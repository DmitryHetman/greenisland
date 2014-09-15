/****************************************************************************
 * This file is part of Hawaii Shell.
 *
 * Copyright (C) 2014 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * Author(s):
 *    Pier Luigi Fiorini
 *
 * $BEGIN_LICENSE:GPL2+$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
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

/*
 * Main procedures
 */

function surfaceMapped(surface) {
    // Get the first view and if it has a role property than this
    // is definitely a shell window
    var firstView = compositor.firstViewOf(surface);
    if (typeof(firstView.role) == "undefined") {
        console.debug("Application surface", surface, "mapped");
        console.debug("\tappId:", surface.className);
        console.debug("\ttitle:", surface.title);
        console.debug("\tsize:", surface.size.width + "x" + surface.size.height);
    } else {
        console.debug("Shell surface", surface, "mapped");
        console.debug("\trole:", firstView.role);
        console.debug("\tsize:", surface.size.width + "x" + surface.size.height);
    }

    // We get mapped/unmapped signals all the time for example when a
    // workspace is selected, for all the surfaces in the previous workspace
    // an unmapped signal is emitted; so we need to figure out if a
    // representation for the surface was already created and exit in that case
    var i;
    for (i = 0; i < surfaceModel.count; i++) {
        var entry = surfaceModel.get(i);

        if (entry.surface === surface)
            return;
    }

    // Call a specialized method to deal with application or
    // shell windows
    if (typeof(firstView.role) == "undefined")
        mapApplicationSurface(surface);
    else
        mapShellSurface(surface, firstView);
}

function surfaceUnmapped(surface) {
    // Get the first view and if it has a role property than this
    // is definitely a shell window
    var firstView = compositor.firstViewOf(surface);
    if (typeof(firstView.role) == "undefined") {
        console.debug("Application surface", surface, "unmapped");
        console.debug("\tappId:", surface.className);
        console.debug("\ttitle:", surface.title);
    } else {
        console.debug("Shell surface", surface, "unmapped");
        console.debug("\trole:", firstView.role);
        console.debug("\tsize:", surface.size.width + "x" + surface.size.height);
    }

    // Call a specialized method to deal with application or
    // shell windows
    if (typeof(firstView.role) == "undefined")
        unmapApplicationSurface(surface);
    else
        unmapShellSurface(surface);
}

function surfaceDestroyed(surface) {
    console.debug("Surface", surface, "destroyed");

    // Remove surface from model
    var i;
    for (i = 0; i < surfaceModel.count; i++) {
        var entry = surfaceModel.get(i);

        if (entry.surface === surface) {
            // Destroy window representation and
            // remove the surface from the model
            if (entry.window.chrome)
                entry.window.chrome.destroy();
            entry.window.destroy();
            surfaceModel.remove(i, 1);
            break;
        }
    }
}

/*
 * Map surfaces
 */

function mapApplicationSurface(surface) {
    // Create surface item
    var component = Qt.createComponent("WaylandClientWindow.qml");
    if (component.status !== Component.Ready) {
        console.error(component.errorString());
        return;
    }

    // Window position
    var pos = Qt.point(0, 0);

    // Request a view for this output (Items cannot be shared between
    // windows so a new one is created on demand)
    var child = compositor.viewForOutput(surface, _greenisland_output);

    // Create and setup window container
    var window = component.createObject(compositorRoot, {"child": child});
    window.child.parent = window;
    window.child.touchEventsEnabled = true;
    window.width = surface.size.width;
    window.height = surface.size.height;

    // Transient parent view
    var transientParentView = null;
    if (surface.windowType === WaylandQuickSurface.Popup ||
            surface.windowType === WaylandQuickSurface.Transient)
        transientParentView = compositor.viewForOutput(surface.transientParent, _greenisland_output);

    // Determine window position
    switch (surface.windowType) {
    case WaylandQuickSurface.Toplevel:
        pos = compositor.calculateInitialPosition(surface);
        surface.globalPosition = pos;
        pos = _greenisland_output.mapToOutput(pos);
        break;
    case WaylandQuickSurface.Popup:
        // Move popups relative to parent window
        pos.x = surface.transientOffset.x;
        pos.y = surface.transientOffset.y;
        surface.globalPosition = Qt.point(transientParentView.surface.globalPosition.x + pos.x,
                                          transientParentView.surface.globalPosition.y + pos.y);
        break;
    case WaylandQuickSurface.Transient:
        // Center transient windows
        pos.x = (transientParentView.width - window.width) / 2;
        pos.y = (transientParentView.height - window.height) / 2;
        surface.globalPosition = Qt.point(transientParentView.surface.globalPosition.x + pos.x,
                                          transientParentView.surface.globalPosition.y + pos.y);
        break;
    default:
        break;
    }

    // Move window
    window.x = pos.x;
    window.y = pos.y;

    // Reparent and give focus
    if (surface.windowType === WaylandQuickSurface.Toplevel)
        window.parent = compositorRoot.screenView.currentWorkspace;
    else
        window.parent = transientParentView;
    window.child.takeFocus();

    // Set transient children so that the parent can be desaturated
    if (surface.windowType === WaylandQuickSurface.Transient)
        transientParentView.parent.transientChildren = window;

    // Log coordinates for debugging purpose
    console.debug("\tposition:", window.x + "," + window.y);
    console.debug("\tscreen:", compositorRoot.screenView.name);

    // Run map animation
    if (typeof(window.runMapAnimation) != "undefined")
        window.runMapAnimation();

    // Add surface to the model
    surfaceModel.append({"surface": surface, "window": window});
}

function mapShellSurface(surface, child) {
    // Shell surfaces have only one view which is passed to us
    // as an argument, check whether it's a view for this output
    // or not
    if (child.output !== _greenisland_output)
        return;

    // Create surface item
    var component = Qt.createComponent("WaylandShellWindow.qml");
    if (component.status !== Component.Ready) {
        console.error(component.errorString());
        return;
    }

    // Create and setup window container
    var window = component.createObject(compositorRoot, {"child": child});
    window.child.parent = window;
    window.child.touchEventsEnabled = true;
    window.width = surface.size.width;
    window.height = surface.size.height;

    // Set initial position
    window.x = window.y = 0;

    // Set appropriate parent
    switch (child.role) {
    case ShellWindowView.DesktopRole:
    case ShellWindowView.DashboardRole:
        window.parent = compositorRoot.screenView.layers.desktop;
        break;
    case ShellWindowView.PanelRole:
    case ShellWindowView.ConfigRole:
        window.parent = compositorRoot.screenView.layers.panels;
        break;
    case ShellWindowView.OverlayRole:
        window.parent = compositorRoot.screenView.layers.overlays;
        break;
    case ShellWindowView.NotificationRole:
        window.parent = compositorRoot.screenView.layers.notifications;
        break;
    case ShellWindowView.LockRole:
        window.parent = compositorRoot.screenView.layers.lock;
        break;
    default:
        window.parent = compositorRoot.screenView.layers.desktop;
        break;
    }

    // Log coordinates for debugging purpose
    console.debug("\tposition:", window.x + "," + window.y);
    console.debug("\tscreen:", compositorRoot.screenView.name);

    // Add surface to the model
    surfaceModel.append({"surface": surface, "window": window});
}

/*
 * Unmap surfaces
 */

function unmapApplicationSurface(surface) {
    // Find window representation
    var i, window = null;
    for (i = 0; i < surfaceModel.count; i++) {
        var entry = surfaceModel.get(i);

        if (entry.surface === surface) {
            window = entry.window;
            break;
        }
    }
    if (!window)
        return;

    // Unset transient children so that the parent can go back to normal
    if (surface.windowType === WaylandQuickSurface.Transient) {
        var transientParentView = compositor.viewForOutput(surface.transientParent, _greenisland_output);
        transientParentView.parent.transientChildren = null;
    }

    // Looks like popup surfaces for Qt applications are never destroyed,
    // this means that the next time the surface is mapped we'll see it
    // in the surface model and don't create a window representation, hence
    // we destroy the surface item when it's unmapped
    if (surface.windowType === WaylandQuickSurface.Popup) {
        if (window.chrome)
            window.chrome.destroy();
        window.destroy();
        surfaceModel.remove(i, 1);
    }
}

function unmapShellSurface(surface) {
}
