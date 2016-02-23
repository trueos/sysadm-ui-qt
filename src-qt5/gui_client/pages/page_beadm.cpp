//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_beadm.h"
#include "ui_page_beadm.h" //auto-generated from the .ui file

beadm_page::beadm_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::page_beadm_ui){
  ui->setupUi(this);	
}

beadm_page::~beadm_page(){
  
}

//Initialize the CORE <-->Page connections
void beadm_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void beadm_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("PC-BSD Boot-Up Configuration") );
  //Now run any CORE communications
  //CORE->communicate("someID", "rpc", "query", QJsonValue("simple-query"));
  
}


// === PRIVATE SLOTS ===
void beadm_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
	
}