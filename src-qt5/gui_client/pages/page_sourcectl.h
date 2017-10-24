//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_SOURCECTL_PAGE_H
#define _PCBSD_SYSADM_CLIENT_SOURCECTL_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
    class sourcectl_ui; //this is the name of the main widget/object in the QtDesigner form
};

class sourcectl_page : public PageWidget{
	Q_OBJECT
public:
    sourcectl_page(QWidget *parent, sysadm_client *core);
    ~sourcectl_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
    QString pageID(){ return "page_sourcectl"; } //ID is used to identify which type of page this is
	void ParseReply(QString, QString, QString, QJsonValue);
	void ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue);

private:
    Ui::sourcectl_ui *ui;


	//UI Update routines


private slots:

	//Core communications

	void send_list_logs();
	void send_read_log();

	//UI slots


    void on_pushButton_DownloadPorts_clicked();
    void on_pushButton_UpdatePorts_clicked();
    void on_pushButton_DeletePorts_clicked();
    void on_pushButton_DownloadSource_clicked();
    void on_pushButton_UpdateSource_clicked();
    void on_pushButton_DeleteSource_clicked();
};
#endif
