//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_MAIN_UI_H
#define _PCBSD_SYSADM_CLIENT_MAIN_UI_H

#include <globals.h>

namespace Ui{
	class MainUI;
};

class MainUI : public QMainWindow{
	Q_OBJECT
public:
	MainUI(sysadm_client *core, QString pageID = "", QString bridgeID = "");
	~MainUI();

	sysadm_client* currentCore();
	QString currentHost();

private:
	Ui::MainUI *ui;
	QString currentPage, host, b_id, nickname;
	void InitializeUI();
	sysadm_client *CORE;
	QShortcut *s_quit, *s_closewin;

private slots:
	//Simple UI slots
	void ShowPowerPopup();
	void ServerDisconnect();
	void ServerReboot();
	void ServerShutdown();

	//Page Management
	void loadPage(QString id = "");
	void ShowPageTitle(QString);
	void ShowSaveButton();
	void SavePage();
	void send_message(QJsonObject msg);

	//Core Signals
	void NoAuthorization();
	void Authorized();
	void Disconnected();
	//Main message signals from core
	void newReply(QString,QString,QString,QJsonValue);
	void bridgeReply(QString,QString,QString,QString,QJsonValue);
	void newEvent(sysadm_client::EVENT_TYPE, QJsonValue);
	void bridgeEvent(QString, sysadm_client::EVENT_TYPE, QJsonValue);

signals:
	void ClientClosed(MainUI*);
	void send_client_message(QString, QJsonObject);

protected:
	void closeEvent(QCloseEvent *ev){
	  emit ClientClosed(this);
	  QMainWindow::closeEvent(ev);
	}
	void resizeEvent(QResizeEvent *ev){
	  //Save the new size to the settings file for later
	  settings->setValue("preferences/MainWindowSize", ev->size());
	  QMainWindow::resizeEvent(ev); //just in case the window needs to see the event too
	}
	
};
#endif
