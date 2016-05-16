//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_CONTROL_PANEL_PAGE_H
#define _PCBSD_SYSADM_CLIENT_CONTROL_PANEL_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"

class control_panel : public PageWidget{
	Q_OBJECT
public:
	control_panel(QWidget *parent, sysadm_client *core);
	~control_panel();

	void setupCore();
	void startPage();
	QString pageID(){ return ""; }
	void ParseReply(QString, QString, QString, QJsonValue);

private:
	QTreeWidget *tree;
	QList<PAGEINFO> pages;

	void setupCategoryButton(QString cat, QTreeWidgetItem *item);
	void setupPageButton(QString id, QTreeWidgetItem *item);

private slots:
	//GUI userevent handling
	void ItemClicked(QTreeWidgetItem*, int);

};
#endif
