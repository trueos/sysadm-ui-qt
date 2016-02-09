//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "SAMPLE.h"
#include "ui_SAMPLE.h" //auto-generated from the .ui file

sample_page::sample_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::sample_ui){
  ui->setupUi(this);	
}

sample_page::~sample_page(){
  
}

//Initialize the CORE <-->Page connections
void sample:page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void sample_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Sample") );
  //Now run any CORE communications
  //CORE->communicate("someID", "rpc", "query", QJsonValue("simple-query"));
  
}


// === PRIVATE SLOTS ===
void sample_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
	
}