//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_FIREWALL_PAGE_H
#define _PCBSD_SYSADM_CLIENT_FIREWALL_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class firewall_ui; //this is the name of the main widget/object in the QtDesigner form
};

class firewall_page : public PageWidget{
	Q_OBJECT
public:
	firewall_page(QWidget *parent, sysadm_client *core);
	~firewall_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "page_firewall"; } //ID is used to identify which type of page this is

public slots:
	void ParseReply(QString, QString, QString, QJsonValue);
	//void ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue);

private:
	Ui::firewall_ui *ui;
	QJsonObject knownPorts;

private slots:
	//Status update requests
	void send_get_status();
	void send_get_knownports();
	void send_get_openports();

	//Basic status changes
	void send_start();
	void send_stop();
	void send_restart();
	void send_enable();
	void send_disable();
	//open/close ports
	void send_open_ports(QStringList ports);
	void send_close_ports(QStringList ports);

	//UI interaction slots
	void selectionChanged(); //open ports selection changed
	void serviceSelected(int index);
	void openPortClicked();
	void closePortClicked();
	void newPortChanged(); //port number/type modified - check validity against current list
  

};
#endif
