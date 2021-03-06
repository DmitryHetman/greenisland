/****************************************************************************
 * This file is part of Hawaii.
 *
 * Copyright (C) 2015-2016 Pier Luigi Fiorini
 *
 * Author(s):
 *    Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * $BEGIN_LICENSE:LGPL$
 *
 * This file may be used under the terms of the GNU Lesser General Public
 * License version 2.1 or later as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPLv21 included in the packaging of
 * this file.  Please review the following information to ensure the
 * GNU Lesser General Public License version 2.1 requirements will be
 * met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 *
 * Alternatively, this file may be used under the terms of the GNU General
 * Public License version 2.0 or later as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPLv2 included in the
 * packaging of this file.  Please review the following information to ensure
 * the GNU General Public License version 2.0 requirements will be
 * met: http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * $END_LICENSE$
 ***************************************************************************/

#ifndef GREENISLAND_GTKSHELL_P_H
#define GREENISLAND_GTKSHELL_P_H

#include <GreenIsland/Server/GtkShell>
#include <GreenIsland/QtWaylandCompositor/private/qwaylandcompositorextension_p.h>

#include <GreenIsland/server/private/qwayland-server-gtk.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Green Island API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

namespace GreenIsland {

namespace Server {

class GREENISLANDSERVER_EXPORT GtkShellPrivate
        : public QWaylandCompositorExtensionPrivate
        , public QtWaylandServer::gtk_shell
{
    Q_DECLARE_PUBLIC(GtkShell)
public:
    GtkShellPrivate();

    static GtkShellPrivate *get(GtkShell *shell) { return shell->d_func(); }

protected:
    void shell_bind_resource(Resource *resource) Q_DECL_OVERRIDE;

    void shell_get_gtk_surface(Resource *resource, uint32_t id,
                               wl_resource *surfaceResource)  Q_DECL_OVERRIDE;
};

class GREENISLANDSERVER_EXPORT GtkSurfacePrivate
        : public QWaylandCompositorExtensionPrivate
        , public QtWaylandServer::gtk_surface
{
    Q_DECLARE_PUBLIC(GtkSurface)
public:
    GtkSurfacePrivate();
    ~GtkSurfacePrivate();

    static GtkSurfacePrivate *get(GtkSurface *surface) { return surface->d_func(); }

protected:
    void surface_destroy_resource(Resource *resource) Q_DECL_OVERRIDE;

    void surface_set_dbus_properties(Resource *resource,
                                     const QString &application_id,
                                     const QString &app_menu_path,
                                     const QString &menubar_path,
                                     const QString &window_object_path,
                                     const QString &application_object_path,
                                     const QString &unique_bus_name) Q_DECL_OVERRIDE;

    void surface_set_modal(Resource *resource) Q_DECL_OVERRIDE;
    void surface_unset_modal(Resource *resource) Q_DECL_OVERRIDE;

private:
    GtkShell *m_shell;
    QWaylandSurface *m_surface;

    QString m_appId;
    QString m_appMenuPath;
    QString m_menuBarPath;
    QString m_windowObjectPath;
    QString m_appObjectPath;
    QString m_uniqueBusName;
};

} // namespace Server

} // namespace GreenIsland

#endif // GREENISLAND_GTKSHELL_P_H

