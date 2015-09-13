/****************************************************************************
 * This file is part of Green Island.
 *
 * Copyright (C) 2015 Pier Luigi Fiorini
 * Copyright (C) 2015 The Qt Company Ltd.
 *
 * Author(s):
 *    Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * $BEGIN_LICENSE:LGPL213$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1, or version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $END_LICENSE$
 ***************************************************************************/

#ifndef GREENISLAND_EGLFSOFFSCREENWINDOW_H
#define GREENISLAND_EGLFSOFFSCREENWINDOW_H

#include <QtGui/qpa/qplatformoffscreensurface.h>

#include <greenislandplatform/greenisland_platform_export.h>

#include <EGL/egl.h>

namespace GreenIsland {

namespace Platform {

class GREENISLANDPLATFORM_EXPORT EglFSOffscreenWindow : public QPlatformOffscreenSurface
{
public:
    EglFSOffscreenWindow(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface);
    ~EglFSOffscreenWindow();

    QSurfaceFormat format() const Q_DECL_OVERRIDE { return m_format; }
    bool isValid() const Q_DECL_OVERRIDE { return m_surface != EGL_NO_SURFACE; }

private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLNativeWindowType m_window;
};

} // namespace Platform

} // namespace GreenIsland

#endif // GREENISLAND_EGLFSOFFSCREENWINDOW_H