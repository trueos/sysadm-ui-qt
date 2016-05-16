//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_SYSTEM_PAGE_H
#define _PCBSD_SYSADM_CLIENT_SYSTEM_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class page_system_ui; //this is the name of the main widget/object in the QtDesigner form
};

class system_page : public PageWidget{
	Q_OBJECT
public:
	system_page(QWidget *parent, sysadm_client *core);
	~system_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "System Manager"; } //ID is used to identify which type of page this is
	void ParseReply(QString, QString, QString, QJsonValue);

private:
	Ui::page_system_ui *ui;

};
#endif
