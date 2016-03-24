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
#include "Messages.h"

class CoreAction : public QAction{
	Q_OBJECT
private:
	QString nickname, host;

public:
	CoreAction(sysadm_client*core, QObject *parent=0);
	~CoreAction();
		
private slots:
	void CoreClosed();
	void CoreConnecting();
	void CoreActive();
	void CoreEvent(sysadm_client::EVENT_TYPE, QJsonValue);
	void priorityChanged(int);

signals:
	//Show a tray message popup
	void ShowMessage(HostMessage);
	void ClearMessage(QString, QString); //host ID, message ID
	void UpdateTrayIcon();
};

class MenuItem : public QMenu{
	Q_OBJECT
public:
	MenuItem(QWidget *parent = 0, QString path="", QMenu *msgmenu = 0);
	~MenuItem();

public slots:
	void UpdateMenu();

private:
	QLineEdit *line_pass;
	QWidgetAction *lineA;
	QMenu *msgMenu;

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
	void ShowMessage(HostMessage);
	void ClearMessage(QString, QString);
	void UpdateTrayIcon();

};

#endif