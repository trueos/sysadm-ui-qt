//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_LP_PAGE_H
#define _PCBSD_SYSADM_CLIENT_LP_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"

//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class lp_ui; //this is the name of the main widget/object in the QtDesigner form
};

class lp_page : public PageWidget{
	Q_OBJECT
public:
	lp_page(QWidget *parent, sysadm_client *core);
	~lp_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "page_lp"; } //ID is used to identify which type of page this is
	void ParseReply(QString, QString, QString, QJsonValue);

private:
	Ui::lp_ui *ui;
	QStringList zpools; //available zpools
	QStringList avail_datasets; //all available datasets (includes zpools)
	void send_list_zpools();

private slots:

	//CORE Interactions (buttons usually)
      // - snapshots page
	void updateSnapshotPage();
	void sendSnapshotRevert();
	void sendSnapshotRemove();
	void sendSnapshotCreate();
	void snapSelectionChanged();
      // - replication page
	void updateReplicationPage();
	void sendRepCreate();
	void sendRepRemove();
	void sendRepStart();
	void sendRepInit();
 	void openNewRepInfo();
	void openOldRepInfo();
	void closeNewRepInfo();
	void new_rep_freq_changed();
      // - schedule page
	void updateSchedulePage();
	void showNewSchSnapInfo();
	void showOldSchSnapInfo();
	void removeSchSnap();
	void saveSchSnapInfo();
	void hideSchSnapInfo();
	void snap_sch_freq_changed();

	void showNewSchScrubInfo();
	void showOldSchScrubInfo();
	void removeSchScrub();
	void saveSchScrubInfo();
	void hideSchScrubInfo();
	void scrub_sch_freq_changed();

	// - settings page
	void updateSettings();
	void sendSaveSettings();
};
#endif
