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

#include "keyboard.h"
#include "keyboard_p.h"
#include "pointer.h"
#include "pointer_p.h"
#include "registry_p.h"
#include "seat.h"
#include "seat_p.h"
#include "touch.h"
#include "touch_p.h"

namespace GreenIsland {

namespace Client {

/*
 * SeatPrivate
 */

SeatPrivate::SeatPrivate()
    : QtWayland::wl_seat()
    , version(0)
    , keyboard(Q_NULLPTR)
    , pointer(Q_NULLPTR)
    , touch(Q_NULLPTR)
{
}

SeatPrivate::~SeatPrivate()
{
    delete keyboard;
    delete pointer;
    delete touch;
}

void SeatPrivate::seat_capabilities(uint32_t capabilities)
{
    Q_Q(Seat);

    if (capabilities & capability_keyboard && !keyboard) {
        keyboard = new Keyboard(q);
        KeyboardPrivate::get(keyboard)->init(get_keyboard());
        Q_EMIT q->keyboardAdded();
    } else if (!(capabilities & capability_keyboard) && keyboard) {
        delete keyboard;
        keyboard = Q_NULLPTR;
        Q_EMIT q->keyboardRemoved();
    }

    if (capabilities & capability_pointer && !pointer) {
        pointer = new Pointer(q);
        PointerPrivate::get(pointer)->init(get_pointer());
        Q_EMIT q->pointerAdded();
    } else if (!(capabilities & capability_pointer) && pointer) {
        delete pointer;
        pointer = Q_NULLPTR;
        Q_EMIT q->pointerRemoved();
    }

    if (capabilities & capability_touch && !touch) {
        touch = new Touch(q);
        TouchPrivate::get(touch)->init(get_touch());
        Q_EMIT q->touchAdded();
    } else if (!(capabilities & capability_touch) && touch) {
        delete touch;
        touch = Q_NULLPTR;
        Q_EMIT q->touchRemoved();
    }
}

void SeatPrivate::seat_name(const QString &name)
{
    Q_Q(Seat);

    if (this->name != name) {
        this->name = name;
        Q_EMIT q->nameChanged();
    }
}

/*
 * Seat
 */

Seat::Seat(QObject *parent)
    : QObject(*new SeatPrivate(), parent)
{
}

QString Seat::name() const
{
    Q_D(const Seat);
    return d->name;
}

quint32 Seat::version() const
{
    Q_D(const Seat);
    return d->version;
}

Keyboard *Seat::keyboard() const
{
    Q_D(const Seat);
    return d->keyboard;
}

Pointer *Seat::pointer() const
{
    Q_D(const Seat);
    return d->pointer;
}

Touch *Seat::touch() const
{
    Q_D(const Seat);
    return d->touch;
}

QByteArray Seat::interfaceName()
{
    return QByteArrayLiteral("wl_seat");
}

} // namespace Client

} // namespace GreenIsland

#include "moc_seat.cpp"
