//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_MESSAGE_SYSTEM_H
#define _PCBSD_SYSADM_CLIENT_MESSAGE_SYSTEM_H

#include <QString>
#include <QDateTime>

struct HostMessage{
	QString host_id;
	QString message;
	QString iconfile;
	QDateTime date_time;
};

inline HostMessage createMessage(QString host, QString msg, QString ico, QDateTime dt = QDateTime::currentDateTime()){
  HostMessage hmsg;
    hmsg.host_id = host;
    hmsg.message = msg;
    hmsg.iconfile = ico;
    hmsg.date_time = dt;
  return hmsg;
}

#endif