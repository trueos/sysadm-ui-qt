//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
// This class is a recursive menu of items for creating long chains as needed
//===========================================
#ifndef _SYSADM_CLIENT_TRAY_MENU_ITEM_H
#define _SYSADM_CLIENT_TRAY_MENU_ITEM_H

#include "globals.h"

class MenuItem : public QMenu{
	Q_OBJECT
public:
	MenuItem(QWidget *parent = 0, QString path="", sysadm_client *core = 0);
	~MenuItem();

public slots:
	void UpdateMenu();

private:
	QString host;
	QLineEdit *line_pass;
	QWidgetAction *lineA;

	void addSubMenu(MenuItem *menu);

private slots:
	void menuTriggered(QAction*);

	void CoreClosed();
	void CoreActive();
	void CoreEvent(sysadm_client::EVENT_TYPE type, QJsonValue data);

	void PasswordReady();

signals:
	//Recursive Signals (will travel up the chain until it gets to the main tray)
	// Main Tray Actions
	void OpenConnectionManager();
	void OpenSettings();
	void CloseApplication();
	void UnlockConnections();

	// CORE Actions
	void OpenCore(QString host);
	void OpenCoreLogs(QString host);

	//Show a tray message popup
	void ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int);
	

	
	
	
};

#endif