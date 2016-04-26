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
	MainUI(sysadm_client *core, QString pageID = "");
	~MainUI();

	sysadm_client* currentCore();


private:
	Ui::MainUI *ui;
	QString currentPage, host;
	void InitializeUI();
	sysadm_client *CORE;
	QShortcut *s_quit;

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
	void resizeEvent(QResizeEvent *ev){
	  //Save the new size to the settings file for later
	  settings->setValue("preferences/MainWindowSize", ev->size());
	  QMainWindow::resizeEvent(ev); //just in case the window needs to see the event too
	}
	
};
#endif
