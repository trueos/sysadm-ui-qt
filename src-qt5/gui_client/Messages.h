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
	QString host_id; //used to identify the source of the message
	QString message_id; //used to identify messages which should be unique/replacable
	QString message;
	QString iconfile;
	QDateTime date_time;
        int priority; // 0-9, in blocks of 3 for criticality (0-2: not important, 3-5: warning, 6-8: Critical, 9: URGENT)
	bool viewed;
};

inline HostMessage createMessage(QString host, QString msg_type, QString msg, QString ico, int pri = 0, QDateTime dt = QDateTime::currentDateTime()){
  HostMessage hmsg;
    hmsg.host_id = host;
    hmsg.message_id = msg_type;
    hmsg.message = msg;
    hmsg.iconfile = ico;
    hmsg.date_time = dt;
    hmsg.priority = pri;
    hmsg.viewed = false;
  return hmsg;
}

#endif
