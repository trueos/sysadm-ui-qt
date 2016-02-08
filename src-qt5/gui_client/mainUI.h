//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_MAIN_UI_H
#define _PCBSD_SYSADM_CLIENT_MAIN_UI_H

#include <QMainWindow>
#include <globals.h>

namespace Ui{
	class MainUI;
};

class MainUI : public QMainWindow{
	Q_OBJECT
public:
	MainUI(sysadm_client *core);
	~MainUI();

	sysadm_client* currentCore();

private:
	Ui::MainUI *ui;
	void InitializeUI();
	sysadm_client *CORE;

private slots:
	//UI Signals
	void auth_connect();
	void auth_disconnect();
	void auto_local_auth_changed();

	//Page Management
	void loadPage(QString id = "");
	
	//Temporary test actions
	void on_push_tmp_sendmsg_clicked();
	void NewMessage(QString, QString, QString, QJsonValue);

	//Core Signals
	void NoAuthorization();
	void Authorized();
	void Disconnected();

signals:
	void ClientClosed(MainUI*);

protected:
	void closeEvent(QCloseEvent *ev){
	  emit ClientClosed(this);
	  QMainWindow::closeEvent(ev);
	}
	
};
#endif
