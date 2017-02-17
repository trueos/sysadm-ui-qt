//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_UPDATES_PAGE_H
#define _PCBSD_SYSADM_CLIENT_UPDATES_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class updates_ui; //this is the name of the main widget/object in the QtDesigner form
};

class updates_page : public PageWidget{
	Q_OBJECT
public:
	updates_page(QWidget *parent, sysadm_client *core);
	~updates_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "page_updates"; } //ID is used to identify which type of page this is
	void ParseReply(QString, QString, QString, QJsonValue);
	void ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue);

private:
	Ui::updates_ui *ui;
	QStringList run_updates;

	//UI Update routines
	void updateBranchList(QString active, QStringList avail);

private slots:

	//Core communications
	void send_list_branches();
	void send_change_branch();
	void send_check_updates(bool force = false);
	void check_start_updates();
	void send_start_updates();
	void send_stop_updates();
	void send_list_settings();
	void send_save_settings();
	void send_list_logs();
	void send_read_log();

	//UI slots
	void check_current_branch();
	void check_current_update();
	void check_current_update_item(QTreeWidgetItem*);
	void on_push_check_updates_clicked(){ send_check_updates(true); }
	void on_check_auto_reboot_toggled(bool);

};
#endif
