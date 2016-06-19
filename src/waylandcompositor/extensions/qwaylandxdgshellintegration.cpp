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

#include "qwaylandxdgshellintegration_p.h"

#include <GreenIsland/QtWaylandCompositor/QWaylandQuickShellSurfaceItem>
#include <GreenIsland/QtWaylandCompositor/QWaylandCompositor>
#include <GreenIsland/QtWaylandCompositor/QWaylandInput>
#include <GreenIsland/QtWaylandCompositor/private/qwaylandxdgshell_p.h>
#include <QMouseEvent>
#include <QGuiApplication>

QT_BEGIN_NAMESPACE

namespace QtWayland {

XdgShellIntegration::XdgShellIntegration(QWaylandQuickShellSurfaceItem *item)
    : QWaylandQuickShellIntegration(item)
    , m_item(item)
    , m_xdgSurface(qobject_cast<QWaylandXdgSurface *>(item->shellSurface()))
    , grabberState(GrabberState::Default)
{
    m_item->setSurface(m_xdgSurface->surface());
    connect(m_xdgSurface, &QWaylandXdgSurface::startMove, this, &XdgShellIntegration::handleStartMove);
    connect(m_xdgSurface, &QWaylandXdgSurface::startResize, this, &XdgShellIntegration::handleStartResize);
    connect(m_xdgSurface, &QWaylandXdgSurface::setMaximized, this, &XdgShellIntegration::handleSetMaximized);
    connect(m_xdgSurface, &QWaylandXdgSurface::unsetMaximized, this, &XdgShellIntegration::handleUnsetMaximized);
    connect(m_xdgSurface, &QWaylandXdgSurface::maximizedChanged, this, &XdgShellIntegration::handleMaximizedChanged);
    connect(m_xdgSurface, &QWaylandXdgSurface::activatedChanged, this, &XdgShellIntegration::handleActivatedChanged);
    connect(m_xdgSurface->surface(), &QWaylandSurface::sizeChanged, this, &XdgShellIntegration::handleSurfaceSizeChanged);
}

bool XdgShellIntegration::mouseMoveEvent(QMouseEvent *event)
{
    if (grabberState == GrabberState::Resize) {
        Q_ASSERT(resizeState.inputDevice == m_item->compositor()->inputDeviceFor(event));
        if (!resizeState.initialized) {
            resizeState.initialMousePos = event->windowPos();
            resizeState.initialized = true;
            return true;
        }
        float scaleFactor = m_item->view()->output()->scaleFactor();
        QPointF delta = (event->windowPos() - resizeState.initialMousePos) / scaleFactor;
        QSize newSize = m_xdgSurface->sizeForResize(resizeState.initialWindowSize, delta, resizeState.resizeEdges);
        m_xdgSurface->sendResizing(newSize);
    } else if (grabberState == GrabberState::Move) {
        Q_ASSERT(moveState.inputDevice == m_item->compositor()->inputDeviceFor(event));
        QQuickItem *moveItem = m_item->moveItem();
        if (!moveState.initialized) {
            moveState.initialOffset = moveItem->mapFromItem(nullptr, event->windowPos());
            moveState.initialized = true;
            return true;
        }
        if (!moveItem->parentItem())
            return true;
        QPointF parentPos = moveItem->parentItem()->mapFromItem(nullptr, event->windowPos());
        moveItem->setPosition(parentPos - moveState.initialOffset);
    }
    return false;
}

bool XdgShellIntegration::mouseReleaseEvent(QMouseEvent *event)
{
    if (grabberState == GrabberState::Resize) {
        float scaleFactor = m_item->view()->output()->scaleFactor();
        QPointF delta = (event->windowPos() - resizeState.initialMousePos) / scaleFactor;
        QSize newSize = m_xdgSurface->sizeForResize(resizeState.initialWindowSize, delta, resizeState.resizeEdges);
        m_xdgSurface->sendUnMaximized(newSize);
        grabberState = GrabberState::Default;
        return true;
    } else if (grabberState == GrabberState::Move) {
        grabberState = GrabberState::Default;
        return true;
    }
    return false;
}

void XdgShellIntegration::handleStartMove(QWaylandInputDevice *inputDevice)
{
    grabberState = GrabberState::Move;
    moveState.inputDevice = inputDevice;
    moveState.initialized = false;
}

void XdgShellIntegration::handleStartResize(QWaylandInputDevice *inputDevice, QWaylandXdgSurface::ResizeEdge edges)
{
    grabberState = GrabberState::Resize;
    resizeState.inputDevice = inputDevice;
    resizeState.resizeEdges = edges;
    resizeState.initialWindowSize = m_xdgSurface->windowGeometry().size();
    resizeState.initialPosition = m_item->moveItem()->position();
    resizeState.initialSurfaceSize = m_item->surface()->size();
    resizeState.initialized = false;
}

void XdgShellIntegration::handleSetMaximized()
{
    maximizeState.initialWindowSize = m_xdgSurface->windowGeometry().size();
    maximizeState.initialPosition = m_item->moveItem()->position();

    m_xdgSurface->sendMaximized(m_item->view()->output()->availableGeometry().size() / m_item->view()->output()->scaleFactor());
}

void XdgShellIntegration::handleUnsetMaximized()
{
    m_xdgSurface->sendUnMaximized(maximizeState.initialWindowSize);
}

void XdgShellIntegration::handleMaximizedChanged()
{
    if (m_xdgSurface->maximized()) {
        m_item->moveItem()->setPosition(m_item->view()->output()->position() + m_item->view()->output()->availableGeometry().topLeft());
    } else {
        m_item->moveItem()->setPosition(maximizeState.initialPosition);
    }
}

void XdgShellIntegration::handleActivatedChanged()
{
    if (m_xdgSurface->activated())
        m_item->raise();
}

void XdgShellIntegration::handleSurfaceSizeChanged()
{
    if (grabberState == GrabberState::Resize) {
        qreal x = resizeState.initialPosition.x();
        qreal y = resizeState.initialPosition.y();
        if (resizeState.resizeEdges & QWaylandXdgSurface::ResizeEdge::TopEdge)
            y += resizeState.initialSurfaceSize.height() - m_item->surface()->size().height();

        if (resizeState.resizeEdges & QWaylandXdgSurface::ResizeEdge::LeftEdge)
            x += resizeState.initialSurfaceSize.width() - m_item->surface()->size().width();
        m_item->moveItem()->setPosition(QPointF(x, y));
    }
}

XdgPopupIntegration::XdgPopupIntegration(QWaylandQuickShellSurfaceItem *item)
    : QWaylandQuickShellIntegration (item)
    , m_xdgPopup(qobject_cast<QWaylandXdgPopup *>(item->shellSurface()))
    , m_xdgShell(QWaylandXdgPopupPrivate::get(m_xdgPopup)->m_xdgShell)
{
    item->setSurface(m_xdgPopup->surface());
    item->moveItem()->setPosition(QPointF(m_xdgPopup->position() * item->view()->output()->scaleFactor()));

    QWaylandClient *client = m_xdgPopup->surface()->client();
    QWaylandQuickShellEventFilter::startFilter(client, [&]() { m_xdgShell->closeAllPopups(); });

    connect(m_xdgPopup, &QWaylandXdgPopup::destroyed, this, &XdgPopupIntegration::handlePopupDestroyed);
}

void XdgPopupIntegration::handlePopupDestroyed()
{
    QWaylandXdgShellPrivate *shellPrivate = QWaylandXdgShellPrivate::get(m_xdgShell);
    auto popups = shellPrivate->m_xdgPopups;
    if (popups.isEmpty())
        QWaylandQuickShellEventFilter::cancelFilter();
}

}

QT_END_NAMESPACE
