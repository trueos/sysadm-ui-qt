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
	class sample_ui; //this is the name of the main widget/object in the QtDesigner form
};

class sample_page : public PageWidget{
	Q_OBJECT
public:
	sample_page(QWidget *parent, sysadm_client *core);
	~sample_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "sample"; } //ID is used to identify which type of page this is
	
private:
	Ui::sample_ui *ui;

private slots:
	void ParseReply(QString, QString, QString, QJsonValue);
};
#endif