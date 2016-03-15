//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_TASK_PAGE_H
#define _PCBSD_SYSADM_CLIENT_TASK_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class taskmanager_ui; //this is the name of the main widget/object in the QtDesigner form
};

class taskmanager_page : public PageWidget{
	Q_OBJECT
public:
	taskmanager_page(QWidget *parent, sysadm_client *core);
	~taskmanager_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "taskmanager"; } //ID is used to identify which type of page this is
	
private:
	Ui::taskmanager_ui *ui;

	void parsePIDS(QJsonObject);
	void ShowMemInfo(int active, int cache, int freeM, int inactive, int wired);
	void ShowCPUInfo(int tot, QList<int> percs); //all values are 0-100
	void ShowCPUTempInfo(QStringList temps); //Temperature info
	QTimer *proctimer;

private slots:
	void ParseReply(QString, QString, QString, QJsonValue);
	void slotRequestProcInfo();
	void slotRequestMemInfo();
	void slotRequestCPUInfo();
	void slotRequestCPUTempInfo();

	void slot_kill_proc();

};
#endif
