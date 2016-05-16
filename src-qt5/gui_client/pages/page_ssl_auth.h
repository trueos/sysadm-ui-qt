//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_SSL_AUTH_PAGE_H
#define _PCBSD_SYSADM_CLIENT_SSL_AUTH_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"

namespace Ui{
	class page_ssl_auth_ui; //this is the name of the main widget/object in the QtDesigner form
};

class ssl_auth_page : public PageWidget{
	Q_OBJECT
public:
	ssl_auth_page(QWidget *parent, sysadm_client *core);
	~ssl_auth_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "page_ssl_auth"; } //ID is used to identify which type of page this is
	void ParseReply(QString, QString, QString, QJsonValue);

private:
	Ui::page_ssl_auth_ui *ui;

	void requestList();

private slots:
	//GUI interactions
	void on_treeWidget_currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *);
	void on_treeWidget_itemActivated(QTreeWidgetItem *, int);
	void on_push_revoke_clicked();
};
#endif
