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

//Single-connection actions (also used for individual bridge connections)
class CoreAction : public QAction{
	Q_OBJECT
private:
	QString nickname, host;
	QString b_id; //bridge_id

public:
	CoreAction(sysadm_client*core, QObject *parent=0, QString bridge_id = "");
	~CoreAction();
		
public slots:
	void CoreClosed();
	void CoreConnecting();
	void CoreActive();
	void CoreEvent(sysadm_client::EVENT_TYPE, QJsonValue);
	void CoreTypeChanged();
	void priorityChanged(int);


signals:
	void updateParent(QString); //parent menu needs to update (significant change to CORE)
	//Show a tray message popup
	void ShowMessage(HostMessage);
	void ClearMessage(QString, QString); //host ID, message ID
	void UpdateTrayIcon();
};

//Bridged Connection: Create a menu of core actions
class CoreMenu : public QMenu{
	Q_OBJECT
private:
	QString nickname, host;
	sysadm_client *Core;
	//QList<CoreAction*> acts;

public:
	CoreMenu(sysadm_client* core, QWidget *parent = 0);
	~CoreMenu();

private slots:
	void menuTriggered(QAction*);
	void triggerReconnect();

	void CoreClosed();
	void CoreConnecting();
	void CoreActive();
	void CoreTypeChanged();
	void BridgeConnectionsChanged(QStringList conns = QStringList());

        void bridgeAuthorized(QString);
	void bridgeEvent(QString, sysadm_client::EVENT_TYPE, QJsonValue);
	void bridgePriorityChanged(QString, int);

signals:
	// CORE Actions
	void OpenCore(QString host);

	//Show a tray message popup
	void ShowMessage(HostMessage);
	void ClearMessage(QString, QString);
	void UpdateTrayIcon();
	void updateParent(QString); //parent menu needs to update (significant change to CORE)
};

//Normal Menu (categories)
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

	//QList<QAction*> coreActions;
	//QList<QMenu*> coreMenus;

	void addSubMenu(MenuItem *menu);
	void addCoreAction(QString host);
	
private slots:
	void menuTriggered(QAction*);
	void CoreItemChanged(QString host);

	void PasswordReady();
	void PasswordTyping();

signals:
	//Recursive Signals (will travel up the chain until it gets to the main tray)
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
