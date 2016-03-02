//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_IOHYVE_PAGE_H
#define _PCBSD_SYSADM_CLIENT_IOHYVE_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class iohyve_ui; //this is the name of the main widget/object in the QtDesigner form
};

class iohyve_page : public PageWidget{
	Q_OBJECT
public:
	iohyve_page(QWidget *parent, sysadm_client *core);
	~iohyve_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "page_iohyve"; } //ID is used to identify which type of page this is
	
private:
	Ui::iohyve_ui *ui;

private slots:
	void ParseReply(QString, QString, QString, QJsonValue);
	void ParseEvent(sysadm_client::EVENT_TYPE evtype, QJsonValue val);

	//Core requests
	// - Setup detection/page
	void request_is_setup();
	void request_setup();
	void request_setup_options();
	// - VM page
	void request_vm_list();
	void request_vm_start();
	void request_vm_stop();

	// - ISO page
	void request_iso_list();
	void request_iso_fetch();
	void request_iso_rename();
	void request_iso_remove();

	//UI slots
	void checkSetupOptions();
	void checkVMSelection();
	void checkISOSelection();
};
#endif