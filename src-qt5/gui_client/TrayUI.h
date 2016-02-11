//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _SYSADM_CLIENT_TRAY_UI_H
#define _SYSADM_CLIENT_TRAY_UI_H

#include "globals.h"
#include "mainUI.h"
#include "MenuItem.h"
#include "C_Manager.h"

class sysadm_tray : public QSystemTrayIcon{
	Q_OBJECT
public:
	sysadm_tray();
	~sysadm_tray();

private:
	//Lists of open cores/windows
	//QHash<QString,sysadm_client*> CORES; // hostIP / core
	QList<MainUI*> CLIENTS; //currently open windows
	C_Manager *CMAN; //current Connection manager window

	//Menu's attached to the tray
	MenuItem *menu; //the main menu
	
	sysadm_client* getCore(QString host);

private slots:
	//Tray activated
	void trayActivated(){
	  this->contextMenu()->popup( QCursor::pos() );
	}
	
	//Update function for when a core is added/removed
	void updateCoreList();
	void ClientClosed(MainUI*);
	
	//Menu Actions
	void OpenConnectionManager();
	void OpenSettings();
	void CloseApplication();
	void OpenCore(QString);
	void OpenCoreLogs(QString);

};

#endif
