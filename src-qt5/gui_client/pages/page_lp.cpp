//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_lp.h"
#include "ui_page_lp.h" //auto-generated from the .ui file

lp_page::lp_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::lp_ui){
  ui->setupUi(this);	
}

lp_page::~lp_page(){
  
}

//Initialize the CORE <-->Page connections
void lp_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void lp_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Life Preserver") );
  //Now run any CORE communications
  /*
  QJsonObject obj;
    obj.insert("sampleVariable","sampleValue");
  CORE->communicate("someID", "rpc", "query",obj);
  */
}


// === PRIVATE SLOTS ===
void lp_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
	
}