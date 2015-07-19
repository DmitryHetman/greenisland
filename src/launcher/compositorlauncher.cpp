/****************************************************************************
 * This file is part of Hawaii.
 *
 * Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * Author(s):
 *    Pier Luigi Fiorini
 *
 * $BEGIN_LICENSE:GPL3-HAWAII$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 or any later version accepted
 * by Pier Luigi Fiorini, which shall act as a proxy defined in Section 14
 * of version 3 of the license.
 *
 * Any modifications to this file must keep this entire header intact.
 *
 * The interactive user interfaces in modified source and object code
 * versions of this program must display Appropriate Legal Notices,
 * as required under Section 5 of the GNU General Public License version 3.
 *
 * In accordance with Section 7(b) of the GNU General Public License
 * version 3, these Appropriate Legal Notices must retain the display of the
 * "Powered by Hawaii" logo.  If the display of the logo is not reasonably
 * feasible for technical reasons, the Appropriate Legal Notices must display
 * the words "Powered by Hawaii".
 *
 * In accordance with Section 7(c) of the GNU General Public License
 * version 3, modified source and object code versions of this program
 * must be marked in reasonable ways as different from the original version.
 *
 * In accordance with Section 7(d) of the GNU General Public License
 * version 3, neither the "Hawaii" name, nor the name of any project that is
 * related to it, nor the names of its contributors may be used to endorse or
 * promote products derived from this software without specific prior written
 * permission.
 *
 * In accordance with Section 7(e) of the GNU General Public License
 * version 3, this license does not grant any license or rights to use the
 * "Hawaii" name or logo, nor any other trademark.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $END_LICENSE$
 ***************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>

#include "cmakedirs.h"
#include "compositorlauncher.h"

#include <unistd.h>

Q_LOGGING_CATEGORY(COMPOSITOR, "greenisland.launcher")

CompositorLauncher::CompositorLauncher(const QString &program, const QStringList &arguments,
                                       QObject *parent)
    : QObject(parent)
    , m_program(program)
    , m_arguments(arguments)
    , m_mode(UnknownMode)
    , m_hardware(UnknownHardware)
    , m_hasLibInputPlugin(false)
    , m_weston(Q_NULLPTR)
    , m_compositor(Q_NULLPTR)
{
    // Detect seat or fallback to seat0
    m_seat = qEnvironmentVariableIsSet("XDG_SEAT")
            ? qgetenv("XDG_SEAT")
            : QStringLiteral("seat0");
}

CompositorLauncher::Mode CompositorLauncher::mode() const
{
    return m_mode;
}

void CompositorLauncher::setMode(const Mode &mode)
{
    m_mode = mode;
}

void CompositorLauncher::start()
{
    // Try to detect mode and hardware
    detectHardware();
    detectMode();
    if (m_mode == UnknownMode) {
        qCWarning(COMPOSITOR) << "No mode detected, please manually specify one!";
        QCoreApplication::quit();
        return;
    }

    // Detect whether we have libinput
    detectLibInput();

    // Setup the sockets
    if (m_mode == NestedMode) {
        // Weston
        m_weston = new CompositorProcess(this);
        m_weston->setSocketName(QStringLiteral("hawaii-master-") + m_seat);
        m_weston->setProgram(QStringLiteral("weston"));
        m_weston->setArguments(QStringList()
                               << QStringLiteral("--shell=fullscreen-shell.so")
                               << QStringLiteral("--socket=%1").arg(m_weston->socketName()));

        // Compositor
        m_compositor = new CompositorProcess(this);
        m_compositor->setSocketName(QStringLiteral("hawaii-slave-") + m_seat);
        m_compositor->setProgram(m_program);
        m_compositor->setArguments(compositorArgs());
        m_compositor->setEnvironment(compositorEnv());

        // Starts the compositor as soon as Weston is started
        connect(m_weston, &CompositorProcess::started,
                m_compositor, &CompositorProcess::start);
    } else {
        //m_socketName = QStringLiteral("hawaii");
        //m_compositor->setSocketName(m_socketName);
    }

    // Summary
    printSummary();

    // Start the process
    if (m_mode == NestedMode)
        m_weston->start();
    else
        spawnCompositor();

    // Set environment so that applications will inherit it
    setupEnvironment();
}

void CompositorLauncher::stop()
{
    m_compositor->stop();
    if (m_mode == NestedMode)
        m_weston->stop();
}

void CompositorLauncher::detectMode()
{
    // Can't detect mode when it was forced by arguments
    if (m_mode != UnknownMode)
        return;

    // Detect Wayland
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
        m_mode = WaylandMode;
        return;
    }

    // Detect X11
    if (qEnvironmentVariableIsSet("DISPLAY")) {
        m_mode = X11Mode;
        return;
    }

    // Detect eglfs since Qt 5.5
    if (qEnvironmentVariableIsSet("QT_QPA_EGLFS_INTEGRATION")) {
        m_mode = EglFSMode;
        return;
    }

    // Use eglfs mode if we detected a particular hardware except
    // for drm with Qt < 5.5 because eglfs_kms was not available
    if (m_hardware != UnknownHardware) {
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
        if (m_hardware == DrmHardware) {
            m_mode = NestedMode;
            return;
        }
#endif
        m_mode = EglFSMode;
        return;
    }

    // TODO: Detect hwcomposer

    // Default to nested mode
    m_mode = NestedMode;
}

void CompositorLauncher::detectHardware()
{
    // Detect Broadcom
    bool found = deviceModel().startsWith(QStringLiteral("Raspberry"));
    if (!found) {
        QStringList paths = QStringList()
                << QStringLiteral("/usr/bin/vcgencmd")
                << QStringLiteral("/opt/vc/bin/vcgencmd")
                << QStringLiteral("/proc/vc-cma");
        found = pathsExist(paths);
    }
    if (found) {
        m_hardware = BroadcomHardware;
        return;
    }

    // TODO: Detect Mali
    // TODO: Detect Vivante

    // Detect DRM
    if (QDir(QStringLiteral("/sys/class/drm")).exists()) {
        m_hardware = DrmHardware;
        return;
    }

    // Unknown hardware
    m_hardware = UnknownHardware;
}

QString CompositorLauncher::deviceModel() const
{
    QFile file(QStringLiteral("/proc/device-tree/model"));
    if (file.open(QIODevice::ReadOnly)) {
        QString data = QString::fromUtf8(file.readAll());
        file.close();
        return data;
    }

    return QString();
}

bool CompositorLauncher::pathsExist(const QStringList &paths) const
{
    Q_FOREACH (const QString &path, paths) {
        if (QFile::exists(path))
            return true;
    }

    return false;
}

void CompositorLauncher::detectLibInput()
{
    // Do we have the libinput plugin?
    Q_FOREACH (const QString &path, QCoreApplication::libraryPaths()) {
        QDir pluginsDir(path + QStringLiteral("/generic"));
        Q_FOREACH (const QString &fileName, pluginsDir.entryList(QDir::Files)) {
            if (fileName == QStringLiteral("libqlibinputplugin.so")) {
                m_hasLibInputPlugin = true;
                break;
            }
        }
    }
}

QStringList CompositorLauncher::compositorArgs() const
{
    QStringList args = m_arguments;

    // Specific arguments
    switch (m_mode) {
    case EglFSMode:
    case HwComposerMode:
        if (m_hasLibInputPlugin)
            args << QStringLiteral("-plugin") << QStringLiteral("libinput");
        else
            args << QStringLiteral("-plugin") << QStringLiteral("EvdevMouse")
                 << QStringLiteral("-plugin") << QStringLiteral("EvdevKeyboard");
        break;
    case NestedMode:
        args << QStringLiteral("--socket=%1").arg(m_compositor->socketName());
        break;
    default:
        break;
    }

    return args;
}

QProcessEnvironment CompositorLauncher::compositorEnv() const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // Standard environment
    env.insert(QStringLiteral("QT_QPA_PLATFORMTHEME"), QStringLiteral("Hawaii"));
    env.insert(QStringLiteral("QT_QUICK_CONTROLS_STYLE"), QStringLiteral("Wind"));
    env.insert(QStringLiteral("XCURSOR_THEME"), QStringLiteral("hawaii"));
    env.insert(QStringLiteral("XCURSOR_SIZE"), QStringLiteral("16"));
    env.insert(QStringLiteral("QSG_RENDER_LOOP"), QStringLiteral("windows"));

    // Specific environment
    switch (m_mode) {
    case NestedMode:
        env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("wayland"));
        env.insert(QStringLiteral("WAYLAND_DISPLAY"), m_weston->socketName());

        if (m_hardware == BroadcomHardware) {
            env.insert(QStringLiteral("QT_WAYLAND_HARDWARE_INTEGRATION"), QStringLiteral("brcm-egl"));
            env.insert(QStringLiteral("QT_WAYLAND_CLIENT_BUFFER_INTEGRATION"), QStringLiteral("brcm-egl"));
        }
        break;
    case EglFSMode:
        // General purpose distributions do not have the proper eglfs
        // integration and may want to build it out of tree, let them
        // specify a QPA plugin with an environment variable
        if (qEnvironmentVariableIsSet("HAWAII_QPA_PLATFORM"))
            env.insert(QStringLiteral("QT_QPA_PLATFORM"), qgetenv("HAWAII_QPA_PLATFORM"));
        else
            env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("eglfs"));

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
        switch (m_hardware) {
        case BroadcomHardware:
            env.insert(QStringLiteral("QT_QPA_EGLFS_INTEGRATION"), QStringLiteral("eglfs_brcm"));
            break;
        default:
            env.insert(QStringLiteral("QT_QPA_EGLFS_INTEGRATION"), QStringLiteral("eglfs_kms"));
            env.insert(QStringLiteral("QT_QPA_EGLFS_KMS_CONFIG"),
                       QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                              QStringLiteral("hawaii/compositor/eglfs/eglfs_kms.json")));
            break;
        }
#endif

        if (m_hasLibInputPlugin)
            env.insert(QStringLiteral("QT_QPA_EGLFS_DISABLE_INPUT"), QStringLiteral("1"));
        env.insert(QStringLiteral("QT_QPA_ENABLE_TERMINAL_KEYBOARD"), QStringLiteral("0"));
        break;
    case HwComposerMode:
        env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("hwcomposer"));
        env.insert(QStringLiteral("EGL_PLATFORM"), QStringLiteral("hwcomposer"));
        //env.insert(QStringLiteral("QT_COMPOSITOR_NEGATE_INVERTED_Y"), QStringLiteral("0"));
        //env.insert(QStringLiteral("QT_QPA_EGLFS_DEPTH"), QStringLiteral("32"));
        if (m_hasLibInputPlugin)
            env.insert(QStringLiteral("QT_QPA_EGLFS_DISABLE_INPUT"), QStringLiteral("1"));
        env.insert(QStringLiteral("QT_QPA_ENABLE_TERMINAL_KEYBOARD"), QStringLiteral("0"));
        break;
    case X11Mode:
        env.insert(QStringLiteral("QT_XCB_GL_INTEGRATION"), QStringLiteral("xcb_egl"));
        break;
    default:
        break;
    }

    return env;
}

void CompositorLauncher::printSummary()
{
    auto modeToString = [this] {
        switch (m_mode) {
        case EglFSMode:
            return QStringLiteral("eglfs");
        case HwComposerMode:
            return QStringLiteral("hwcomposer");
        case NestedMode:
            return QStringLiteral("nested");
        case X11Mode:
            return QStringLiteral("x11");
        case WaylandMode:
            return QStringLiteral("wayland");
        default:
            return QStringLiteral("unknown");
        }
    };

    auto hwToString = [this] {
        switch (m_hardware) {
        case DrmHardware:
            return QStringLiteral("drm");
        case BroadcomHardware:
            return QStringLiteral("broadcom");
        default:
            return QStringLiteral("unknown");
        }
    };

    qCInfo(COMPOSITOR) << "Mode:" << modeToString();
    qCInfo(COMPOSITOR) << "Hardware:" << hwToString();
    qCInfo(COMPOSITOR) << "libinput:" << m_hasLibInputPlugin;
    if (m_mode == NestedMode) {
        qCInfo(COMPOSITOR) << "Weston socket:" << m_weston->socketName();
        qCInfo(COMPOSITOR) << "Compositor socket:" << m_compositor->socketName();
    } else {
        qCInfo(COMPOSITOR) << "Environment variables:";
        QProcessEnvironment env = compositorEnv();
        QStringList sortedKeys = env.keys();
        qSort(sortedKeys);
        Q_FOREACH (const QString &key, sortedKeys) {
            if (key.startsWith(QStringLiteral("QT_")) ||
                    key.startsWith(QStringLiteral("QML2_")) ||
                    key.startsWith(QStringLiteral("XDG_")))
                qCInfo(COMPOSITOR, "\t%s=%s",
                       qPrintable(key),
                       qPrintable(env.value(key)));
        }
    }
    if (m_mode == X11Mode)
        qCInfo(COMPOSITOR) << "X11 display:" << qgetenv("DISPLAY");
}

void CompositorLauncher::setupEnvironment()
{
    // Make clients run on Wayland
    if (m_hardware == BroadcomHardware) {
        qputenv("QT_QPA_PLATFORM", QByteArray("wayland-brcm"));
        qputenv("QT_WAYLAND_HARDWARE_INTEGRATION", QByteArray("brcm-egl"));
        qputenv("QT_WAYLAND_CLIENT_BUFFER_INTEGRATION", QByteArray("brcm-egl"));
    } else {
        qputenv("QT_QPA_PLATFORM", QByteArray("wayland"));
    }
    qputenv("GDK_BACKEND", QByteArray("wayland"));

    // Set WAYLAND_DISPLAY only when nested, otherwise we don't need to do
    // it because applications can detect the socket themselves
    if (m_mode == NestedMode)
        qputenv("WAYLAND_DISPLAY", m_compositor->socketName().toLatin1());
    else
        qunsetenv("WAYLAND_DISPLAY");
}

void CompositorLauncher::spawnCompositor()
{
    // This is the child process, setup the arguments
    QStringList args = compositorArgs();
    char **const argv = new char *[args.count() + 2];
    char **p = argv;

    *p = strdup(qPrintable(m_program));
    ++p;

    Q_FOREACH (const QString &arg, args) {
        *p = new char[arg.length() + 1];
        ::strcpy(*p, qPrintable(arg));
        ++p;
    }

    *p = 0;

    // And the environment
    QProcessEnvironment env = compositorEnv();
    char **const envp = new char *[env.keys().count() + 1];
    char **e = envp;

    Q_FOREACH (const QString &key, env.keys()) {
        QString item = QString("%1=%2").arg(key).arg(env.value(key));
        *e = new char[item.length() + 1];
        ::strcpy(*e, qPrintable(item));
        ++e;
    }

    *e = 0;

    // Execute
    qCInfo(COMPOSITOR, "Running: %s %s", qPrintable(m_program),
           qPrintable(args.join(' ')));
    if (::execvpe(argv[0], argv, envp) == -1) {
        qCCritical(COMPOSITOR, "Failed to execute %s: %s",
                   argv[0], strerror(errno));

        p = argv;
        while (*p != 0)
            delete [] *p++;
        delete [] argv;

        e = envp;
        while (*e != 0)
            delete [] *e++;
        delete [] envp;

        ::exit(EXIT_FAILURE);
    }
}

#include "moc_compositorlauncher.cpp"
