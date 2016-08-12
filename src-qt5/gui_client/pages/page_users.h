//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_USERS_PAGE_H
#define _PCBSD_SYSADM_CLIENT_USERS_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class users_ui; //this is the name of the main widget/object in the QtDesigner form
};

class users_page : public PageWidget{
	Q_OBJECT
public:
	users_page(QWidget *parent, sysadm_client *core);
	~users_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "users"; } //ID is used to identify which type of page this is
	void ParseReply(QString, QString, QString, QJsonValue);
	//void ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue);

private:
	Ui::users_ui *ui;
        QJsonObject userObj; //keep this saved for instant read on selection change as needed

private slots:
	//Widget update routines
	void updateUserList(); //uses the userObj variable
	void updateUserSelection(); //uses the userObj variable
	void checkUidSelection(); //uses the userObj variable (validate manual UID selection)

	//Core Request routines

	//Button routines
};
#endif
