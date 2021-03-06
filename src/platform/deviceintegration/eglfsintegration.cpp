/****************************************************************************
 * This file is part of Hawaii.
 *
 * Copyright (C) 2015-2016 Pier Luigi Fiorini
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * Author(s):
 *    Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * $BEGIN_LICENSE:QTLGPL$
 *
 * GNU Lesser General Public License Usage
 * This file may be used under the terms of the GNU Lesser General
 * Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.LGPLv3 included in the
 * packaging of this file. Please review the following information to
 * ensure the GNU Lesser General Public License version 3 requirements
 * will be met: https://www.gnu.org/licenses/lgpl.html.
 *
 * GNU General Public License Usage
 * Alternatively, this file may be used under the terms of the GNU
 * General Public License version 2.0 or (at your option) the GNU General
 * Public license version 3 or any later version approved by the KDE Free
 * Qt Foundation. The licenses are as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPLv2 and LICENSE.GPLv3
 * included in the packaging of this file. Please review the following
 * information to ensure the GNU General Public License requirements will
 * be met: https://www.gnu.org/licenses/gpl-2.0.html and
 * https://www.gnu.org/licenses/gpl-3.0.html.
 *
 * $END_LICENSE$
 ***************************************************************************/

#include <QtGui/QGuiApplication>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>
#include <QtGui/QScreen>
#include <QtGui/QSurfaceFormat>

#include <QtGui/qpa/qplatforminputcontextfactory_p.h>
#include <QtGui/qpa/qwindowsysteminterface.h>

#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>

#include <QtPlatformHeaders/QEGLNativeContext>

#include "eglconvenience/eglconvenience.h"
#include "eglconvenience/eglplatformcontext.h"
#include "eglconvenience/eglpbuffer.h"
#include "deviceintegration/egldeviceintegration.h"
#include "deviceintegration/eglfscontext.h"
#include "deviceintegration/eglfscursor.h"
#include "deviceintegration/eglfsintegration.h"
#include "deviceintegration/eglfsoffscreenwindow.h"
#include "deviceintegration/eglfswindow.h"
#include "logind/vthandler.h"
#include "libinput/libinputmanager_p.h"
#include "libinput/libinputhandler.h"
#include "platformcompositor/openglcompositorbackingstore.h"

static void initResources()
{
#ifndef QT_NO_CURSOR
    Q_INIT_RESOURCE(cursor);
#endif
}

namespace GreenIsland {

namespace Platform {

EglFSIntegration::EglFSIntegration()
    : m_display(EGL_NO_DISPLAY)
    , m_inputContext(Q_NULLPTR)
    , m_fontDatabase(new QGenericUnixFontDatabase)
    , m_services(new QGenericUnixServices)
    , m_liHandler(Q_NULLPTR)
{
    initResources();
}

EGLDisplay EglFSIntegration::display() const
{
    return m_display;
}

QPlatformInputContext *EglFSIntegration::inputContext() const
{
    return m_inputContext;
}

QPlatformFontDatabase *EglFSIntegration::fontDatabase() const
{
    return m_fontDatabase.data();
}

QPlatformServices *EglFSIntegration::services() const
{
    return m_services.data();
}

QPlatformNativeInterface *EglFSIntegration::nativeInterface() const
{
    return const_cast<EglFSIntegration *>(this);
}

VtHandler *EglFSIntegration::vtHandler() const
{
    return m_vtHandler.data();
}

void EglFSIntegration::initialize()
{
    if (!egl_device_integration()->configurationFileName().isEmpty())
        egl_device_integration()->loadConfiguration(egl_device_integration()->configurationFileName());

    egl_device_integration()->platformInit();

    m_display = eglGetDisplay(nativeDisplay());
    if (Q_UNLIKELY(m_display == EGL_NO_DISPLAY)) {
        qFatal("Failed to open EGL display");
        return;
    }

    EGLint major, minor;
    if (Q_UNLIKELY(!eglInitialize(m_display, &major, &minor))) {
        qFatal("Failed to initialize EGL display");
        return;
    }

    QString icStr = QPlatformInputContextFactory::requested();
    if (icStr.isNull())
        icStr = QLatin1String("compose");
    m_inputContext = QPlatformInputContextFactory::create(icStr);

    if (egl_device_integration()->usesVtHandler())
        m_vtHandler.reset(new VtHandler);

    if (!egl_device_integration()->handlesInput())
        m_liHandler = new Platform::LibInputManager(this);

    // Switch vt
    if (!m_vtHandler.isNull() && m_liHandler) {
        connect(m_liHandler->handler(), &LibInputHandler::keyReleased, this, [this](const LibInputKeyEvent &e) {
            if (e.modifiers.testFlag(Qt::ControlModifier) &&
                    e.modifiers.testFlag(Qt::AltModifier) &&
                    e.key >= Qt::Key_F1 && e.key <= Qt::Key_F35) {
                quint32 vt = e.key - Qt::Key_F1 + 1;
                m_vtHandler->activate(vt);
            }
        });
    }

    if (egl_device_integration()->usesDefaultScreen())
        addScreen(new EglFSScreen(display()));
    else
        egl_device_integration()->screenInit();

    // Set the first screen as primary
    QScreen *firstScreen = QGuiApplication::screens().at(0);
    if (firstScreen && firstScreen->handle())
        setPrimaryScreen(firstScreen->handle());
}

void EglFSIntegration::destroy()
{
    if (!egl_device_integration()->configurationFileName().isEmpty())
        egl_device_integration()->saveConfiguration(egl_device_integration()->configurationFileName());

    Q_FOREACH (QWindow *window, QGuiApplication::topLevelWindows())
        window->destroy();

    egl_device_integration()->screenDestroy();

    if (m_display != EGL_NO_DISPLAY)
        eglTerminate(m_display);

    egl_device_integration()->platformDestroy();
}

QAbstractEventDispatcher *EglFSIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformWindow *EglFSIntegration::createPlatformWindow(QWindow *window) const
{
    QWindowSystemInterface::flushWindowSystemEvents();

    QPlatformWindow *w = egl_device_integration()->createPlatformWindow(window);
    if (!w) {
        w = new EglFSWindow(window);
        static_cast<EglFSWindow *>(w)->create();
    }

    // Activate only the compositor window for the primary screen in order to
    // make keyboard input work
    if (window->type() != Qt::ToolTip && window->screen() == QGuiApplication::primaryScreen())
        w->requestActivateWindow();

    return w;
}

QPlatformBackingStore *EglFSIntegration::createPlatformBackingStore(QWindow *window) const
{
    QPlatformBackingStore *pbs = egl_device_integration()->createPlatformBackingStore(window);
    if (pbs)
        return pbs;

    OpenGLCompositorBackingStore *bs = new OpenGLCompositorBackingStore(window);
    if (!window->handle())
        window->create();
    static_cast<EglFSWindow *>(window->handle())->setBackingStore(bs);
    return bs;
}

QPlatformOpenGLContext *EglFSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QPlatformOpenGLContext *pctx = egl_device_integration()->createPlatformOpenGLContext(context);
    if (pctx)
        return pctx;

    EGLDisplay dpy = context->screen() ? static_cast<EglFSScreen *>(context->screen()->handle())->display() : display();
    QPlatformOpenGLContext *share = context->shareHandle();
    QVariant nativeHandle = context->nativeHandle();

    EglFSContext *ctx;
    QSurfaceFormat adjustedFormat = egl_device_integration()->surfaceFormatFor(context->format());
    if (nativeHandle.isNull()) {
        EGLConfig config = EglFSIntegration::chooseConfig(dpy, adjustedFormat);
        ctx = new EglFSContext(adjustedFormat, share, dpy, &config, QVariant());
    } else {
        ctx = new EglFSContext(adjustedFormat, share, dpy, 0, nativeHandle);
    }
    nativeHandle = QVariant::fromValue<QEGLNativeContext>(QEGLNativeContext(ctx->eglContext(), dpy));

    context->setNativeHandle(nativeHandle);
    return ctx;
}

QPlatformOffscreenSurface *EglFSIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    EGLDisplay dpy = surface->screen() ? static_cast<EglFSScreen *>(surface->screen()->handle())->display() : display();
    QSurfaceFormat fmt = egl_device_integration()->surfaceFormatFor(surface->requestedFormat());
    if (egl_device_integration()->supportsPBuffers())
        return new EGLPbuffer(dpy, fmt, surface);
    else
        return new EglFSOffscreenWindow(dpy, fmt, surface);
    // Never return null. Multiple QWindows are not supported by this plugin.
}

bool EglFSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    // We assume that devices will have more and not less capabilities
    if (egl_device_integration()->hasCapability(cap))
        return true;

    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    case WindowManagement: return false;
    case RasterGLSurface: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

enum ResourceType {
    EglDisplay,
    EglWindow,
    EglContext,
    EglConfig,
    NativeDisplay,
    XlibDisplay,
    WaylandDisplay
};

static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match ResourceType
                                        QByteArrayLiteral("egldisplay"),
                                        QByteArrayLiteral("eglwindow"),
                                        QByteArrayLiteral("eglcontext"),
                                        QByteArrayLiteral("eglconfig"),
                                        QByteArrayLiteral("nativedisplay"),
                                        QByteArrayLiteral("display"),
                                        QByteArrayLiteral("server_wl_display")
                                      };
    const QByteArray *end = names + sizeof(names) / sizeof(names[0]);
    const QByteArray *result = std::find(names, end, key);
    if (result == end)
        result = std::find(names, end, key.toLower());
    return int(result - names);
}

void *EglFSIntegration::nativeResourceForIntegration(const QByteArray &resource)
{
    void *result = 0;

    switch (resourceType(resource)) {
    case EglDisplay:
        result = display();
        break;
    case NativeDisplay:
        result = reinterpret_cast<void*>(nativeDisplay());
        break;
    case WaylandDisplay:
        result = egl_device_integration()->wlDisplay();
        break;
    default:
        break;
    }

    return result;
}

void *EglFSIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *)
{
    void *result = 0;

    switch (resourceType(resource)) {
    case XlibDisplay:
        // Play nice when using the x11 hooks: Be compatible with xcb that allows querying
        // the X Display pointer, which is nothing but our native display.
        result = reinterpret_cast<void*>(nativeDisplay());
        break;
    default:
        break;
    }

    return result;
}

void *EglFSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    void *result = 0;

    switch (resourceType(resource)) {
    case EglDisplay:
        if (window && window->handle())
            result = static_cast<EglFSScreen *>(window->handle()->screen())->display();
        else
            result = display();
        break;
    case EglWindow:
        if (window && window->handle())
            result = reinterpret_cast<void*>(static_cast<EglFSWindow *>(window->handle())->eglWindow());
        break;
    default:
        break;
    }

    return result;
}

void *EglFSIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    void *result = 0;

    switch (resourceType(resource)) {
    case EglContext:
        if (context->handle())
            result = static_cast<EglFSContext *>(context->handle())->eglContext();
        break;
    case EglConfig:
        if (context->handle())
            result = static_cast<EglFSContext *>(context->handle())->eglConfig();
        break;
    case EglDisplay:
        if (context->handle())
            result = static_cast<EglFSContext *>(context->handle())->eglDisplay();
        break;
    default:
        break;
    }

    return result;
}

static void *eglContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);

    EglFSContext *handle = static_cast<EglFSContext *>(context->handle());
    if (!handle)
        return 0;

    return handle->eglContext();
}

QPlatformNativeInterface::NativeResourceForContextFunction EglFSIntegration::nativeResourceFunctionForContext(const QByteArray &resource)
{
    QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "get_egl_context")
        return NativeResourceForContextFunction(eglContextForContext);

    return 0;
}

QFunctionPointer EglFSIntegration::platformFunction(const QByteArray &function) const
{
    Q_UNUSED(function);
    return 0;
}

void EglFSIntegration::addScreen(QPlatformScreen *screen)
{
    screenAdded(screen);
}

void EglFSIntegration::removeScreen(QPlatformScreen *screen)
{
    destroyScreen(screen);
}

EGLConfig EglFSIntegration::chooseConfig(EGLDisplay display, const QSurfaceFormat &format)
{
    class Chooser : public EglConfigChooser {
    public:
        Chooser(EGLDisplay display)
            : EglConfigChooser(display) { }
        bool filterConfig(EGLConfig config) const Q_DECL_OVERRIDE {
            return egl_device_integration()->filterConfig(display(), config)
                    && EglConfigChooser::filterConfig(config);
        }
    };

    Chooser chooser(display);
    chooser.setSurfaceFormat(format);
    return chooser.chooseConfig();
}

EGLNativeDisplayType EglFSIntegration::nativeDisplay() const
{
    return egl_device_integration()->platformDisplay();
}

} // namespace Platform

} // namespace GreenIsland
