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
	bool local_showall, local_advmode, local_hasupdates; //Local tab options
	QMenu *local_viewM;

	//Core requests
	void send_local_update();
	void send_local_audit();
	void send_local_check_upgrade();

	//Parsing Core Replies
	void update_local_list(QJsonObject obj);
	void update_local_audit(QJsonObject obj);
	void update_pending_process(QJsonObject obj);

	//Bytes to human-readable conversion
	QString BtoHR(double bytes);
	//JsonArray to StringList conversion
	QStringList ArrayToStringList(QJsonArray array);
	//Status icon setting routine
	void updateStatusIcon( QTreeWidgetItem *it );
	//Status icon list update
	bool updateStatusList(QStringList *list, QString stat, bool enabled); //returns: changed (true/false);
	
private slots:
	void ParseReply(QString, QString, QString, QJsonValue);
	void ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue);

	//GUI Updates
	// - local tab
	void update_local_buttons();
	void update_local_pkg_check(bool checked);
	void update_local_viewall(bool checked);
	void update_local_viewadv(bool checked);
	void goto_browser_from_local(QTreeWidgetItem *it);
	// - repo tab
	void browser_goto_pkg(QString origin, QString repo);
	// - pending tab
	void pending_show_log(bool);


	//GUI -> Core Requests
	void send_local_rmpkgs();
	void send_local_lockpkgs();
	void send_local_unlockpkgs();
	void send_local_upgradepkgs();
};
#endif