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
#include "SettingsDialog.h"
#include "Messages.h"

class sysadm_tray : public QSystemTrayIcon{
	Q_OBJECT
public:
	sysadm_tray();
	~sysadm_tray();

private:
	//Flag for determining whether popups are allowed
	bool showNotices;

	//Lists of open windows
	QList<MainUI*> CLIENTS; //currently open windows
	C_Manager *CMAN; //current Connection manager window
	SettingsDialog *SDLG; //current settings dialog

	//Menu's attached to the tray
	MenuItem *menu; //the main menu
	QMenu *msgMenu; //The submenu for showing current messages
	QIcon generateMsgIcon(QString iconfile, int priority);

	//Timers/flags to control the icon "flash" frequency
	QTimer *iconTimer, *msgTimer;
	bool iconreset;
	int cPriority;

	//Function to create/retrieve a core
	sysadm_client* getCore(QString host);
	
private slots:
	//Tray activated
	void trayActivated();

	//Allow popups
	void allowPopups(){
	  showNotices = true;
	}
	//Application-wide setting changed
	void UpdateWindows();
	
	//Update function for when a core is added/removed
	void updateCoreList();
	void ClientClosed(MainUI*);
	
	//Menu Actions
	void OpenConnectionManager();
	void OpenSettings();
	void CloseApplication();
	void OpenCore(QString, QString page="");
	void UnlockConnections();
	//Message Notifications
	void ShowMessage(HostMessage);
	void ClearMessage(QString,QString);
	void MessageTriggered(QAction*);
	void updateMessageMenu();
	void MessagesViewed();
	//Icon Updates
	void UpdateIconPriority();
	void UpdateIcon();

};

#endif
