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

public slots:
	void ParseReply(QString, QString, QString, QJsonValue);
	//void ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue);

private:
	Ui::users_ui *ui;
	QMessageBox *msgbox;
        QJsonObject userObj, groupObj; //keep this saved for instant read on selection change as needed
	bool usersLoading, groupsLoading;

	void ShowError(QString msg, QString details);

private slots:
	//Widget update routines
	void updateUserList(); //uses the userObj variable
	void updateUserSelection(); //uses the userObj variable
	void checkSelectionChanges(); //uses the userObj variable (validate manual UID selection)
	void validateUserChanges();

	void updateGroupList(); //uses the groupObj variable
	void updateGroupSelection(); //uses the groupObj variable
	void validateGroupSettings(); //uses the groupObj variable

	//Core Request routines
	void send_list_users();
	void send_user_save();
	void send_user_remove();
	void send_update_pcdevs();

	void send_list_groups();
	void send_group_save();
	void send_group_remove();
	
	//Button routines
	void on_push_user_new_clicked();
	void on_push_user_remove_clicked();
	void on_push_user_save_clicked();
	void on_tool_pc_findkey_clicked();

	void on_push_group_rmuser_clicked();
	void on_push_group_adduser_clicked();
	void on_push_group_new_clicked();
	void on_push_group_remove_clicked();
	void on_push_group_save_clicked();

};
#endif
