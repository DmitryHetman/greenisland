/****************************************************************************
 * This file is part of Green Island.
 *
 * Copyright (C) 2012-2013 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * Author(s):
 *    Pier Luigi Fiorini
 *
 * $BEGIN_LICENSE:GPL3+$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <QDebug>
#include <QDir>
#include <QScreen>
#include <QStringList>
#include <QWindow>

#include <VCompositor>

#include "greenisland.h"
#include "gitsha1.h"
#include "config.h"

#include <sys/utsname.h>
#include <unistd.h>
#include <stdio.h>

static int convertPermission(const QFileInfo &fileInfo)
{
    int p = 0;

    if (fileInfo.permission(QFile::ReadUser))
        p += 400;
    if (fileInfo.permission(QFile::WriteUser))
        p += 200;
    if (fileInfo.permission(QFile::ExeUser))
        p += 100;
    if (fileInfo.permission(QFile::ReadGroup))
        p += 40;
    if (fileInfo.permission(QFile::WriteGroup))
        p += 20;
    if (fileInfo.permission(QFile::ExeGroup))
        p += 10;
    if (fileInfo.permission(QFile::ReadOther))
        p += 4;
    if (fileInfo.permission(QFile::WriteOther))
        p += 2;
    if (fileInfo.permission(QFile::ExeOther))
        p += 1;

    return p;
}

static void verifyXdgRuntimeDir()
{
    QByteArray dirName = qgetenv("XDG_RUNTIME_DIR");

    if (dirName.isEmpty()) {
        QString msg = QObject::tr(
                    "The XDG_RUNTIME_DIR environment variable is not set.\n"
                    "Refer to your distribution on how to get it, or read\n"
                    "http://www.freedesktop.org/wiki/Specifications/basedir-spec\n"
                    "on how to implement it.\n");
        qFatal(msg.toUtf8());
    }

    QFileInfo fileInfo(dirName);

    if (!fileInfo.exists()) {
        QString msg = QObject::tr(
                    "The XDG_RUNTIME_DIR environment variable is set to "
                    "\"%1\", which doesn't exist.\n").arg(dirName.constData());
        qFatal(msg.toUtf8());
    }

    if (convertPermission(fileInfo) != 700 || fileInfo.ownerId() != getuid()) {
        QString msg = QObject::tr(
                    "XDG_RUNTIME_DIR is set to \"%1\" and is not configured correctly.\n"
                    "Unix access mode must be 0700, but is 0%2.\n"
                    "It must also be owned by the current user (UID %3), "
                    "but is owned by UID %4 (\"%5\").\n")
                .arg(dirName.constData())
                .arg(convertPermission(fileInfo))
                .arg(getuid())
                .arg(fileInfo.ownerId())
                .arg(fileInfo.owner());
        qFatal(msg.toUtf8());
    }
}

int main(int argc, char *argv[])
{
    // Assume the xcb platform if the DISPLAY environment variable is defined,
    // otherwise go for kms
    if (!qgetenv("DISPLAY").isEmpty())
        setenv("QT_QPA_PLATFORM", "xcb", 0);
    else {
        setenv("QT_QPA_PLATFORM", "kms", 0);
        setenv("QT_QPA_GENERIC_PLUGINS", "evdevmouse,evdevkeyboard,evdevtouch", 0);
        setenv("QT_KMS_TTYKBD", "1", 0);
    }

    GreenIsland app(argc, argv);

    // Print the banner
    qDebug() << qPrintable(QString("Green Island v%1\n").arg(GREENISLAND_VERSION))
             << "              http://www.maui-project.org\n"
             << "              Bug reports to: https://github.com/hawaii-desktop/greenisland/issues\n"
             << qPrintable(QString("              Build: %1-%2")
                           .arg(GREENISLAND_VERSION).arg(GIT_REV));

    // Print the current system
    struct utsname uts;
    if (uname(&uts) != -1)
        qDebug() << qPrintable(QString("OS: %1, %2, %3, %4").arg(uts.sysname)
                               .arg(uts.release).arg(uts.version).arg(uts.machine));
    else
        qDebug() << "OS: Unknown";

    // Command line arguments
    QStringList arguments = QCoreApplication::instance()->arguments();

    // Usage instructions
    if (arguments.contains(QLatin1String("-h")) || arguments.contains(QLatin1String("--help"))) {
        printf("Usage: greenisland [options]\n");
        printf("Arguments are:\n");
        printf("\t--fullscreen\t\trun in fullscreen mode\n");
        printf("\t--synthesize-touch\tsynthesize touch for unhandled mouse events\n");
        return 0;
    }

    // Check whether XDG_RUNTIME_DIR is ok or not
    verifyXdgRuntimeDir();

    // Synthesize touch for unhandled mouse events
    if (arguments.contains(QLatin1String("--synthesize-touch")))
        app.setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);

    // Window geometry
    QRect geometry;
    if (arguments.contains(QLatin1String("--fullscreen")))
        geometry = QGuiApplication::primaryScreen()->geometry();
    else
        geometry = QGuiApplication::primaryScreen()->availableGeometry();

    return app.exec();
}
