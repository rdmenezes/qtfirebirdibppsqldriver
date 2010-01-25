/*
* This file is part of QtFirebirdIBPPSQLDriver - Qt SQL driver for Firebird with IBPP library
* Copyright (C) 2006-2010 Alex Wencel
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Contact e-mail: Alex Wencel <alex.wencel@gmail.com>
* Program URL   : http://code.google.com/p/qtfirebirdibppsqldriver
*
*/

#include <qsqldriverplugin.h>
#include <qstringlist.h>
#include "qsql_ibpp.h"

class QFBDriverPlugin : public QSqlDriverPlugin
{
public:
    QFBDriverPlugin();

    QSqlDriver* create(const QString &);
    QStringList keys() const;
};

QFBDriverPlugin::QFBDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QFBDriverPlugin::create(const QString &name)
{
    if (name == QLatin1String("QFIREBIRD")) {
        QFBDriver* driver = new QFBDriver();
        return driver;
    }
    return 0;
}

QStringList QFBDriverPlugin::keys() const
{
    QStringList l;
    l  << QLatin1String("QFIREBIRD");
    return l;
}

Q_EXPORT_STATIC_PLUGIN(QFBDriverPlugin)
Q_EXPORT_PLUGIN2(qsqlfb, QFBDriverPlugin)
