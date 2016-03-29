//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_SAMPLE_PAGE_H
#define _PCBSD_SYSADM_CLIENT_SAMPLE_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class pkg_page_ui; //this is the name of the main widget/object in the QtDesigner form
};

class pkg_page : public PageWidget{
	Q_OBJECT
public:
	pkg_page(QWidget *parent, sysadm_client *core);
	~pkg_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "page_pkg"; } //ID is used to identify which type of page this is
	
private:
	Ui::pkg_page_ui *ui;

	//Internal flags
	bool local_showall, local_advmode; //Local tab options
	QMenu *local_viewM;

	//Core requests
	void send_local_update();
	void send_local_audit();

	//Parsing Core Replies
	void update_local_list(QJsonObject obj);
	void update_local_audit(QJsonObject obj);

	//Bytes to human-readable conversion
	QString BtoHR(double bytes);

private slots:
	void ParseReply(QString, QString, QString, QJsonValue);
	void ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue);

	//GUI Updates
	void update_local_buttons();
	void update_local_pkg_check(bool checked);
	void update_local_viewall(bool checked);
	void update_local_viewadv(bool checked);

	//GUI -> Core Requests
	void send_local_rmpkgs();
};
#endif