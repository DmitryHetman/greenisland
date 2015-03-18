/****************************************************************************
 * This file is part of Green Island.
 *
 * Copyright (C) 2012-2014 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#ifndef COMPOSITORSETTINGS_H
#define COMPOSITORSETTINGS_H

#include <QtCore/QObject>
#include <QtCompositor/QWaylandInputDevice>

#include <GreenIsland/Compositor>

namespace GreenIsland {

class CompositorSettingsPrivate;

class GREENISLAND_EXPORT CompositorSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString keyboardLayout READ keyboardLayout WRITE setKeyboardLayout NOTIFY keyMapChanged)
    Q_PROPERTY(QString keyboardVariant READ keyboardVariant WRITE setKeyboardVariant NOTIFY keyMapChanged)
    Q_PROPERTY(QString keyboardOptions READ keyboardOptions WRITE setKeyboardOptions NOTIFY keyMapChanged)
    Q_PROPERTY(QString keyboardRules READ keyboardRules WRITE setKeyboardRules NOTIFY keyMapChanged)
    Q_PROPERTY(QString keyboardModel READ keyboardModel WRITE setKeyboardModel NOTIFY keyMapChanged)
public:
    CompositorSettings(Compositor *compositor);
    ~CompositorSettings();

    QString keyboardLayout() const;
    void setKeyboardLayout(const QString &layout);

    QString keyboardVariant() const;
    void setKeyboardVariant(const QString &variant);

    QString keyboardOptions() const;
    void setKeyboardOptions(const QString &options);

    QString keyboardRules() const;
    void setKeyboardRules(const QString &rules);

    QString keyboardModel() const;
    void setKeyboardModel(const QString &model);

Q_SIGNALS:
    void keyMapChanged();

private:
    Q_DECLARE_PRIVATE(CompositorSettings)
    CompositorSettingsPrivate *const d_ptr;
};

}

#endif // COMPOSITORSETTINGS_H