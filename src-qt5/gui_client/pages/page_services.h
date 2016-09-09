//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_SERVICES_PAGE_H
#define _PCBSD_SYSADM_CLIENT_SERVICES_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class services_ui; //this is the name of the main widget/object in the QtDesigner form
};

class services_page : public PageWidget{
	Q_OBJECT
public:
	services_page(QWidget *parent, sysadm_client *core);
	~services_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "page_services"; } //ID is used to identify which type of page this is

public slots:
	void ParseReply(QString, QString, QString, QJsonValue);
	//void ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue);

private:
	Ui::services_ui *ui;

private slots:
	void send_list_services();
	void send_start_services();
	void send_stop_services();
	void send_restart_services();
	void send_enable_services();
	void send_disable_services();

	void updateSelection();

};
#endif
