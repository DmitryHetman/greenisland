/****************************************************************************
**
** Copyright (C) 2013 Klarälvdalens Datakonsult AB (KDAB).
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

#include "qwaylandtextinput.h"
#include "qwaylandtextinput_p.h"

#include <GreenIsland/QtWaylandCompositor/QWaylandCompositor>
#include <GreenIsland/QtWaylandCompositor/private/qwaylandinput_p.h>

#include "qwaylandsurface.h"
#include "qwaylandview.h"
#include "qwaylandxkb.h"

#include <QGuiApplication>
#include <QInputMethodEvent>

QT_BEGIN_NAMESPACE

static int indexFromUtf8Index(const QString &str, int utf8Index, int baseIndex = 0) {
    if (utf8Index == 0)
        return baseIndex;

    if (utf8Index < 0) {
        const QByteArray &utf8 = str.leftRef(baseIndex).toUtf8();
        return QString::fromUtf8(utf8.left(qMax(utf8.length() + utf8Index, 0))).length();
    } else {
        const QByteArray &utf8 = str.midRef(baseIndex).toUtf8();
        return QString::fromUtf8(utf8.left(utf8Index)).length() + baseIndex;
    }
}

QWaylandTextInputClientState::QWaylandTextInputClientState()
    : hints(0)
    , cursorRectangle()
    , surroundingText()
    , cursorPosition(0)
    , anchorPosition(0)
    , preferredLanguage()
    , changedState()
{
}

Qt::InputMethodQueries QWaylandTextInputClientState::updatedQueries(const QWaylandTextInputClientState &other) const
{
    Qt::InputMethodQueries queries;

    if (hints != other.hints)
        queries |= Qt::ImHints;
    if (cursorRectangle != other.cursorRectangle)
        queries |= Qt::ImCursorRectangle;
    if (surroundingText != other.surroundingText)
        queries |= Qt::ImSurroundingText | Qt::ImCurrentSelection;
    if (cursorPosition != other.cursorPosition)
        queries |= Qt::ImCursorPosition | Qt::ImCurrentSelection;
    if (anchorPosition != other.anchorPosition)
        queries |= Qt::ImAnchorPosition | Qt::ImCurrentSelection;
    if (preferredLanguage != other.preferredLanguage)
        queries |= Qt::ImPreferredLanguage;

    return queries;
}

Qt::InputMethodQueries QWaylandTextInputClientState::mergeChanged(const QWaylandTextInputClientState &other) {
    Qt::InputMethodQueries queries;

    if ((other.changedState & Qt::ImHints) && hints != other.hints) {
        hints = other.hints;
        queries |= Qt::ImHints;
    }

    if ((other.changedState & Qt::ImCursorRectangle) && cursorRectangle != other.cursorRectangle) {
        cursorRectangle = other.cursorRectangle;
        queries |= Qt::ImCursorRectangle;
    }

    if ((other.changedState & Qt::ImSurroundingText) && surroundingText != other.surroundingText) {
        surroundingText = other.surroundingText;
        queries |= Qt::ImSurroundingText | Qt::ImCurrentSelection;
    }

    if ((other.changedState & Qt::ImCursorPosition) && cursorPosition != other.cursorPosition) {
        cursorPosition = other.cursorPosition;
        queries |= Qt::ImCursorPosition | Qt::ImCurrentSelection;
    }

    if ((other.changedState & Qt::ImAnchorPosition) && anchorPosition != other.anchorPosition) {
        anchorPosition = other.anchorPosition;
        queries |= Qt::ImAnchorPosition | Qt::ImCurrentSelection;
    }

    if ((other.changedState & Qt::ImPreferredLanguage) && preferredLanguage != other.preferredLanguage) {
        preferredLanguage = other.preferredLanguage;
        queries |= Qt::ImPreferredLanguage;
    }

    return queries;
}

QWaylandTextInputPrivate::QWaylandTextInputPrivate(QWaylandCompositor *compositor)
    : QWaylandExtensionTemplatePrivate()
    , QtWaylandServer::zwp_text_input_v2()
    , compositor(compositor)
    , focus(nullptr)
    , focusResource(nullptr)
    , focusDestroyListener()
    , inputPanelVisible(false)
    , currentState(new QWaylandTextInputClientState)
    , pendingState(new QWaylandTextInputClientState)
    , serial(0)
    , enabledSurfaces()
{
}

void QWaylandTextInputPrivate::sendInputMethodEvent(QInputMethodEvent *event)
{
    if (!focusResource || !focusResource->handle)
        return;

    if (event->replacementLength() > 0) {
        int start = currentState->surroundingText.leftRef(event->replacementStart()).toUtf8().size();
        int end = currentState->surroundingText.leftRef(event->replacementStart() + event->replacementLength()).toUtf8().size();
        send_delete_surrounding_text(focusResource->handle, start, end - start);
    }
    foreach (const QInputMethodEvent::Attribute &attribute, event->attributes()) {
        if (attribute.type == QInputMethodEvent::Selection) {
            int start = event->commitString().leftRef(attribute.start).toUtf8().size();
            int end = event->commitString().leftRef(attribute.start + attribute.length).toUtf8().size();
            send_cursor_position(focusResource->handle, start, end - start);
        }
    }
    send_commit_string(focusResource->handle, serial, event->commitString());
    foreach (const QInputMethodEvent::Attribute &attribute, event->attributes()) {
        if (attribute.type == QInputMethodEvent::Cursor) {
            int index = event->preeditString().leftRef(attribute.start).toUtf8().size();
            send_preedit_cursor(focusResource->handle, index);
        } else if (attribute.type == QInputMethodEvent::TextFormat) {
            int start = currentState->surroundingText.leftRef(attribute.start).toUtf8().size();
            int end = currentState->surroundingText.leftRef(attribute.start + attribute.length).toUtf8().size();
            // TODO add support for different stylesQWaylandTextInput
            send_preedit_styling(focusResource->handle, start, end - start, preedit_style_default);
        }
    }
    send_preedit_string(focusResource->handle, serial, event->preeditString(), event->preeditString());
}

void QWaylandTextInputPrivate::sendKeyEvent(QKeyEvent *event)
{
    if (!focusResource || !focusResource->handle)
        return;

    // TODO add support for modifiers

    foreach (xkb_keysym_t keysym, QWaylandXkb::toKeysym(event)) {
        send_keysym(focusResource->handle, serial, event->timestamp(), keysym,
                    event->type() == QEvent::KeyPress ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED,
                    0);
    }
}

void QWaylandTextInputPrivate::sendInputPanelState()
{
    if (!focusResource || !focusResource->handle)
        return;

    QInputMethod *inputMethod = qApp->inputMethod();
    const QRectF& keyboardRect = inputMethod->keyboardRectangle();
    const QRectF& sceneInputRect = inputMethod->inputItemTransform().mapRect(inputMethod->inputItemRectangle());
    const QRectF& localRect = sceneInputRect.intersected(keyboardRect).translated(-sceneInputRect.topLeft());

    send_input_panel_state(focusResource->handle,
                           inputMethod->isVisible() ? input_panel_visibility_visible : input_panel_visibility_hidden,
                           localRect.x(), localRect.y(), localRect.width(), localRect.height());
}

void QWaylandTextInputPrivate::sendTextDirection()
{
    if (!focusResource || !focusResource->handle)
        return;

    const Qt::LayoutDirection direction = qApp->inputMethod()->inputDirection();
    send_text_direction(focusResource->handle, compositor->nextSerial(),
                        (direction == Qt::LeftToRight) ? text_direction_ltr :
                                                         (direction == Qt::RightToLeft) ? text_direction_rtl : text_direction_auto);
}

void QWaylandTextInputPrivate::sendLocale()
{
    if (!focusResource || !focusResource->handle)
        return;

    const QLocale locale = qApp->inputMethod()->locale();
    send_language(focusResource->handle, compositor->nextSerial(), locale.bcp47Name());
}

QVariant QWaylandTextInputPrivate::inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const
{
    switch (property) {
    case Qt::ImHints:
        return QVariant(static_cast<int>(currentState->hints));
    case Qt::ImCursorRectangle:
        return currentState->cursorRectangle;
    case Qt::ImFont:
        // Not supported
        return QVariant();
    case Qt::ImCursorPosition:
        return currentState->cursorPosition;
    case Qt::ImSurroundingText:
        return currentState->surroundingText;
    case Qt::ImCurrentSelection:
        return currentState->surroundingText.mid(qMin(currentState->cursorPosition, currentState->anchorPosition),
                                                 qAbs(currentState->anchorPosition - currentState->cursorPosition));
    case Qt::ImMaximumTextLength:
        // Not supported
        return QVariant();
    case Qt::ImAnchorPosition:
        return currentState->anchorPosition;
    case Qt::ImAbsolutePosition:
        // Not supported
        return QVariant();
    case Qt::ImTextAfterCursor:
        if (argument.isValid())
            return currentState->surroundingText.mid(currentState->cursorPosition, argument.toInt());
        return currentState->surroundingText.mid(currentState->cursorPosition);
    case Qt::ImTextBeforeCursor:
        if (argument.isValid())
            return currentState->surroundingText.left(currentState->cursorPosition).right(argument.toInt());
        return currentState->surroundingText.left(currentState->cursorPosition);
    case Qt::ImPreferredLanguage:
        return currentState->preferredLanguage;

    default:
        return QVariant();
    }
}

void QWaylandTextInputPrivate::setFocus(QWaylandView *view)
{
    Q_Q(QWaylandTextInput);

    QWaylandSurface *surface = view ? view->surface() : nullptr;
    QWaylandSurface *focusSurface = focus ? focus->surface() : nullptr;
    if (focusResource && focusSurface != surface) {
        uint32_t serial = compositor->nextSerial();
        send_leave(focusResource->handle, serial, focusSurface->resource());
        focusDestroyListener.reset();
    }

    Resource *resource = surface ? resourceMap().value(surface->waylandClient()) : 0;

    if (resource && (focusSurface != surface || focusResource != resource)) {
        uint32_t serial = compositor->nextSerial();
        currentState.reset(new QWaylandTextInputClientState);
        pendingState.reset(new QWaylandTextInputClientState);
        send_enter(resource->handle, serial, surface->resource());
        focusResource = resource;
        sendInputPanelState();
        sendLocale();
        sendTextDirection();
        focusDestroyListener.listenForDestruction(surface->resource());
        if (inputPanelVisible && q->isSurfaceEnabled(surface))
            qApp->inputMethod()->show();
    }

    focusResource = resource;
    focus = view;
}

void QWaylandTextInputPrivate::zwp_text_input_v2_bind_resource(Resource *resource)
{
    send_modifiers_map(resource->handle, QByteArray(""));
}

void QWaylandTextInputPrivate::zwp_text_input_v2_destroy_resource(Resource *resource)
{
    if (focusResource == resource)
        focusResource = 0;
}

void QWaylandTextInputPrivate::zwp_text_input_v2_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void QWaylandTextInputPrivate::zwp_text_input_v2_enable(Resource *resource, wl_resource *surface)
{
    Q_Q(QWaylandTextInput);

    QWaylandSurface *s = QWaylandSurface::fromResource(surface);
    enabledSurfaces.insert(resource, s);
    emit q->surfaceEnabled(s);
}

void QWaylandTextInputPrivate::zwp_text_input_v2_disable(QtWaylandServer::zwp_text_input_v2::Resource *resource, wl_resource *)
{
    Q_Q(QWaylandTextInput);

    QWaylandSurface *s = enabledSurfaces.take(resource);
    emit q->surfaceDisabled(s);
}

void QWaylandTextInputPrivate::zwp_text_input_v2_show_input_panel(Resource *)
{
    inputPanelVisible = true;

    qApp->inputMethod()->show();
}

void QWaylandTextInputPrivate::zwp_text_input_v2_hide_input_panel(Resource *)
{
    inputPanelVisible = false;

    qApp->inputMethod()->hide();
}

void QWaylandTextInputPrivate::zwp_text_input_v2_set_cursor_rectangle(Resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (resource != focusResource)
        return;

    pendingState->cursorRectangle = QRect(x, y, width, height);

    pendingState->changedState |= Qt::ImCursorRectangle;
}

void QWaylandTextInputPrivate::zwp_text_input_v2_update_state(Resource *resource, uint32_t serial, uint32_t flags)
{
    Q_Q(QWaylandTextInput);

    qCDebug(qLcCompositorInputMethods) << "update_state" << serial << flags;

    if (resource != focusResource)
        return;

    if (flags == update_state_reset || flags == update_state_enter) {
        qCDebug(qLcCompositorInputMethods) << "QInputMethod::reset()";
        qApp->inputMethod()->reset();
    }

    this->serial = serial;

    Qt::InputMethodQueries queries;
    if (flags == update_state_change) {
        queries = currentState->mergeChanged(*pendingState.data());
    } else {
        queries = pendingState->updatedQueries(*currentState.data());
        currentState.swap(pendingState);
    }

    pendingState.reset(new QWaylandTextInputClientState);

    if (queries) {
        qCDebug(qLcCompositorInputMethods) << "QInputMethod::update()" << queries;

        emit q->updateInputMethod(queries);
    }
}

void QWaylandTextInputPrivate::zwp_text_input_v2_set_content_type(Resource *resource, uint32_t hint, uint32_t purpose)
{
    if (resource != focusResource)
        return;

    pendingState->hints = 0;

    if ((hint & content_hint_auto_completion) == 0
        && (hint & content_hint_auto_correction) == 0)
        pendingState->hints |= Qt::ImhNoPredictiveText;
    if ((hint & content_hint_auto_capitalization) == 0)
        pendingState->hints |= Qt::ImhNoAutoUppercase;
    if ((hint & content_hint_lowercase) != 0)
        pendingState->hints |= Qt::ImhPreferLowercase;
    if ((hint & content_hint_uppercase) != 0)
        pendingState->hints |= Qt::ImhPreferUppercase;
    if ((hint & content_hint_hidden_text) != 0)
        pendingState->hints |= Qt::ImhHiddenText;
    if ((hint & content_hint_sensitive_data) != 0)
        pendingState->hints |= Qt::ImhSensitiveData;
    if ((hint & content_hint_latin) != 0)
        pendingState->hints |= Qt::ImhLatinOnly;
    if ((hint & content_hint_multiline) != 0)
        pendingState->hints |= Qt::ImhMultiLine;

    switch (purpose) {
    case content_purpose_normal:
        break;
    case content_purpose_alpha:
        pendingState->hints |= Qt::ImhUppercaseOnly | Qt::ImhLowercaseOnly;
        break;
    case content_purpose_digits:
        pendingState->hints |= Qt::ImhDigitsOnly;
        break;
    case content_purpose_number:
        pendingState->hints |= Qt::ImhFormattedNumbersOnly;
        break;
    case content_purpose_phone:
        pendingState->hints |= Qt::ImhDialableCharactersOnly;
        break;
    case content_purpose_url:
        pendingState->hints |= Qt::ImhUrlCharactersOnly;
        break;
    case content_purpose_email:
        pendingState->hints |= Qt::ImhEmailCharactersOnly;
        break;
    case content_purpose_name:
    case content_purpose_password:
        break;
    case content_purpose_date:
        pendingState->hints |= Qt::ImhDate;
        break;
    case content_purpose_time:
        pendingState->hints |= Qt::ImhTime;
        break;
    case content_purpose_datetime:
        pendingState->hints |= Qt::ImhDate | Qt::ImhTime;
        break;
    case content_purpose_terminal:
    default:
        break;
    }

    pendingState->changedState |= Qt::ImHints;
}

void QWaylandTextInputPrivate::zwp_text_input_v2_set_preferred_language(Resource *resource, const QString &language)
{
    if (resource != focusResource)
        return;

    pendingState->preferredLanguage = language;

    pendingState->changedState |= Qt::ImPreferredLanguage;
}

void QWaylandTextInputPrivate::zwp_text_input_v2_set_surrounding_text(Resource *resource, const QString &text, int32_t cursor, int32_t anchor)
{
    if (resource != focusResource)
        return;

    pendingState->surroundingText = text;
    pendingState->cursorPosition = indexFromUtf8Index(text, cursor);
    pendingState->anchorPosition = indexFromUtf8Index(text, anchor);

    pendingState->changedState |= Qt::ImSurroundingText | Qt::ImCursorPosition | Qt::ImAnchorPosition;
}

QWaylandTextInput::QWaylandTextInput(QWaylandObject *container, QWaylandCompositor *compositor)
    : QWaylandExtensionTemplate(container, *new QWaylandTextInputPrivate(compositor))
{
    connect(&d_func()->focusDestroyListener, &QWaylandDestroyListener::fired,
            this, &QWaylandTextInput::focusSurfaceDestroyed);

    connect(qApp->inputMethod(), &QInputMethod::visibleChanged,
            this, &QWaylandTextInput::sendInputPanelState);
    connect(qApp->inputMethod(), &QInputMethod::keyboardRectangleChanged,
            this, &QWaylandTextInput::sendInputPanelState);
    connect(qApp->inputMethod(), &QInputMethod::inputDirectionChanged,
            this, &QWaylandTextInput::sendTextDirection);
    connect(qApp->inputMethod(), &QInputMethod::localeChanged,
            this, &QWaylandTextInput::sendLocale);
}

QWaylandTextInput::~QWaylandTextInput()
{
}

void QWaylandTextInput::sendInputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QWaylandTextInput);

    d->sendInputMethodEvent(event);
}

void QWaylandTextInput::sendKeyEvent(QKeyEvent *event)
{
    Q_D(QWaylandTextInput);

    d->sendKeyEvent(event);
}

void QWaylandTextInput::sendInputPanelState()
{
    Q_D(QWaylandTextInput);

    d->sendInputPanelState();
}

void QWaylandTextInput::sendTextDirection()
{
    Q_D(QWaylandTextInput);

    d->sendTextDirection();
}

void QWaylandTextInput::sendLocale()
{
    Q_D(QWaylandTextInput);

    d->sendLocale();
}

QVariant QWaylandTextInput::inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const
{
    const Q_D(QWaylandTextInput);

    return d->inputMethodQuery(property, argument);
}

QWaylandView *QWaylandTextInput::focus() const
{
    const Q_D(QWaylandTextInput);

    return d->focus;
}

void QWaylandTextInput::setFocus(QWaylandView *view)
{
    Q_D(QWaylandTextInput);

    d->setFocus(view);
}

void QWaylandTextInput::focusSurfaceDestroyed(void *)
{
    Q_D(QWaylandTextInput);

    d->focusDestroyListener.reset();

    d->focus = nullptr;
    d->focusResource = nullptr;
}

bool QWaylandTextInput::isSurfaceEnabled(QWaylandSurface *surface) const
{
    const Q_D(QWaylandTextInput);

    return d->enabledSurfaces.values().contains(surface);
}

void QWaylandTextInput::add(::wl_client *client, uint32_t id, int version)
{
    Q_D(QWaylandTextInput);

    d->add(client, id, version);
}

const wl_interface *QWaylandTextInput::interface()
{
    return QWaylandTextInputPrivate::interface();
}

QByteArray QWaylandTextInput::interfaceName()
{
    return QWaylandTextInputPrivate::interfaceName();
}

QT_END_NAMESPACE
