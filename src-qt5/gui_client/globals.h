//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_GLOBAL_HEADERS_H
#define _PCBSD_SYSADM_CLIENT_GLOBAL_HEADERS_H

#include <QApplication>
#include <QString>
#include <QList>
#include <QStringList>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QWidget>
#include <QObject>
#include <QSettings>
#include <QTimer>


#include "../Core/sysadm-client.h"

//Now define all the global variables
// (those each subsystem might need to access independently)
extern sysadm_client *S_CORE;
extern QSettings *settings;

#endif


