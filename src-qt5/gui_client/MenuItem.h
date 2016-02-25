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

class CoreAction : public QAction{
	Q_OBJECT
private:
	QString nickname;

public:
	CoreAction(sysadm_client*core, QObject *parent=0);
	~CoreAction();
		
private slots:
	void CoreClosed();
	void CoreActive();
	void CoreEvent(sysadm_client::EVENT_TYPE type, QJsonValue data);

signals:
	//Show a tray message popup
	void ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int);
};

class MenuItem : public QMenu{
	Q_OBJECT
public:
	MenuItem(QWidget *parent = 0, QString path="");
	~MenuItem();

public slots:
	void UpdateMenu();

private:
	QLineEdit *line_pass;
	QWidgetAction *lineA;

	void addSubMenu(MenuItem *menu);
	void addCoreAction(QString host);

private slots:
	void menuTriggered(QAction*);

	void PasswordReady();
	void PasswordTyping();

signals:
	//Recursive Signals (will travel up the chain until it gets to the main tray)
	// Main Tray Actions
	void OpenConnectionManager();
	void OpenSettings();
	void CloseApplication();
	void UnlockConnections();

	// CORE Actions
	void OpenCore(QString host);

	//Show a tray message popup
	void ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int);
	

	
	
	
};

#endif