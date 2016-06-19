/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWAYLANDXDGSHELLINTEGRATION_H
#define QWAYLANDXDGSHELLINTEGRATION_H

#include <GreenIsland/QtWaylandCompositor/private/qwaylandquickshellsurfaceitem_p.h>
#include <GreenIsland/QtWaylandCompositor/QWaylandXdgSurface>

QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

namespace QtWayland {

class XdgShellIntegration : public QWaylandQuickShellIntegration
{
    Q_OBJECT
public:
    XdgShellIntegration(QWaylandQuickShellSurfaceItem *item);
    bool mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    bool mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void handleStartMove(QWaylandInputDevice *inputDevice);
    void handleStartResize(QWaylandInputDevice *inputDevice, QWaylandXdgSurface::ResizeEdge edges);
    void handleSetMaximized();
    void handleUnsetMaximized();
    void handleMaximizedChanged();
    void handleActivatedChanged();
    void handleSurfaceSizeChanged();

private:
    enum class GrabberState {
        Default,
        Resize,
        Move
    };
    QWaylandQuickShellSurfaceItem *m_item;
    QWaylandXdgSurface *m_xdgSurface;

    GrabberState grabberState;
    struct {
        QWaylandInputDevice *inputDevice;
        QPointF initialOffset;
        bool initialized;
    } moveState;

    struct {
        QWaylandInputDevice *inputDevice;
        QWaylandXdgSurface::ResizeEdge resizeEdges;
        QSizeF initialWindowSize;
        QPointF initialMousePos;
        QPointF initialPosition;
        QSize initialSurfaceSize;
        bool initialized;
    } resizeState;

    struct {
        QSize initialWindowSize;
        QPointF initialPosition;
    } maximizeState;
};

class XdgPopupIntegration : public QWaylandQuickShellIntegration
{
    Q_OBJECT
public:
    XdgPopupIntegration(QWaylandQuickShellSurfaceItem *item);

private Q_SLOTS:
    void handlePopupDestroyed();

private:
    QWaylandXdgPopup *m_xdgPopup;
    QWaylandXdgShell *m_xdgShell;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDXDGSHELLINTEGRATION_H