//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_taskmanager.h"
#include "ui_page_taskmanager.h" //auto-generated from the .ui file

taskmanager_page::taskmanager_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::taskmanager_ui){
  ui->setupUi(this);	
}

taskmanager_page::~taskmanager_page(){
  
}

//Initialize the CORE <-->Page connections
void taskmanager_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void taskmanager_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Task Manager") );
  //Now run any CORE communications
  //CORE->communicate("someID", "rpc", "query", QJsonValue("simple-query"));
  
}


// === PRIVATE SLOTS ===
void taskmanager_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
	
}
