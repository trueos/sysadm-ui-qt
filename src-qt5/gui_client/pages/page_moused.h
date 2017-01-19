//===========================================
//  TrueOS source code
//  Copyright (c) 2017, TrueOS Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _SYSADM_CLIENT_MOUSED_PAGE_H
#define _SYSADM_CLIENT_MOUSED_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class page_moused; //this is the name of the main widget/object in the QtDesigner form
};

class moused_page : public PageWidget{
	Q_OBJECT
public:
	moused_page(QWidget *parent, sysadm_client *core);
	~moused_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "page_moused"; } //ID is used to identify which type of page this is

public slots:
	void ParseReply(QString, QString, QString, QJsonValue);

private slots:
	void send_list_devices();
	void send_toggle_device_active();
	void send_list_device_settings();
	void send_save_device_settings();

	//UI slots
	void settingChanged(); //update whether the apply button is available
	void slider_acc_exp_changed();
	void slider_acc_linear_changed();
	void slider_drift_changed();

private:
	Ui::page_moused *ui;

	void updateCurrentSettings(QJsonObject); //routine to parse/show the current settings

};
#endif
