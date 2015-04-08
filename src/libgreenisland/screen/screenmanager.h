/****************************************************************************
 * This file is part of Green Island.
 *
 * Copyright (C) 2014-2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#ifndef SCREENMANAGER_H
#define SCREENMANAGER_H

#include <QtCore/QObject>

namespace GreenIsland {

class Compositor;
class Output;
class ScreenBackend;

class ScreenManager : public QObject
{
    Q_OBJECT
public:
    ScreenManager(Compositor *compositor);

    void acquireConfiguration(const QString &fileName = QString());

Q_SIGNALS:
    void configurationAcquired();

private:
    Compositor *m_compositor;
    ScreenBackend *m_backend;
    int m_outputsDone;

private Q_SLOTS:
    void outputAdded(Output *output);
    void outputRemoved(Output *output);
    void primaryOutputChanged(Output *output);
};

}

#endif // SCREENMANAGER_H
