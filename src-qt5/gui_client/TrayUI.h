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

class sysadm_tray : public QSystemTrayIcon{
	Q_OBJECT
public:
	sysadm_tray();
	~sysadm_tray();

private:
	//Lists of open cores/windows
	QHash<QString,sysadm_client*> CORES; // hostIP / core
	QList<MainUI*> CLIENTS; //currently open windows

	//Menu's attached to the tray
	QMenu *M_opengui; //menu for launching the main config UI on a particular server
	
	sysadm_client* getCore(QString host);

private slots:
	//Tray activated
	void trayActivated(){
	  this->contextMenu()->popup( QCursor::pos() );
	}
	
	//Update function for when a core is added/removed
	void updateCoreList();
	void ClientClosed(MainUI*);
	
	//void showEvent(sysadm_client::EVENT_TYPE, QJsonValue);
	//Menu Actions
	void open_gui(QAction*);
	void open_config();
	void close_tray();

};

#endif
