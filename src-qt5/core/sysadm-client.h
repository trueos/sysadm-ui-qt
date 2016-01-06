//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_CLASS_H
#define _PCBSD_SYSADM_CLIENT_CLASS_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QWebSocket>
#include <QObject>
#include <QSettings>

class sysadm_client : public QObject{
	Q_OBJECT
public:
	sysadm_client();
	~sysadm_client();

	// Overall Connection functions (start/stop)
	bool openConnection(QString user, QString pass, QString hostIP);
	bool openConnection(QString authkey, QString hostIP);
	void closeConnection();

	// Connection Hosts Database Access
	QStringList knownHosts(); //Returns: <IP>::::<Name>
	bool saveHost(QString IP, QString name);
	bool removeHost(QString IP);

	

private:
	QSettings *settings;
	QWebSocket *SOCKET;
	QString chost, cport, cauthkey; //current host/port/authkey

public slots:
	
private slots:
	
signals:

};

#endif