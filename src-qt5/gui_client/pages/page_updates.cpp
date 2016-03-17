//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_updates.h"
#include "ui_page_updates.h" //auto-generated from the .ui file

updates_page::updates_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::updates_ui){
  ui->setupUi(this);	
}

updates_page::~updates_page(){
  
}

//Initialize the CORE <-->Page connections
void updates_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void updates_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Update Manager") );
  //Now run any CORE communications
  /*
  QJsonObject obj;
    obj.insert("action","checkupdates");
  CORE->communicate("someID", "sysadm", "update",obj);
  */
}


// === PRIVATE SLOTS ===
void updates_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
	
}