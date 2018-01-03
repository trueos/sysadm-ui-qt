//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_source_control.h"
#include "ui_page_source_control.h" //auto-generated from the .ui file

#define PAGE_ID QString("page_sourcectl_request_")
#define FETCH_PROCESS_ID QString("system_fetch_ports_tree")

sourcectl_page::sourcectl_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::sourcectl_ui){
  ui->setupUi(this);
}

sourcectl_page::~sourcectl_page(){

}

//Initialize the CORE <-->Page connections
void sourcectl_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );

}

//Page embedded, go ahead and startup any core requests
void sourcectl_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("System Sources") );
  //Now run any CORE communications
  CORE->registerForEvents(sysadm_client::DISPATCHER);

}

void sourcectl_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(PAGE_ID)){ return; }
  if(namesp == "error" || name == "error"){ qDebug() << "Got Error:" << id << args; return; }
  //Now parse the reply
  if(id==PAGE_ID+"fetch_ports"){
    qDebug() << "successful start of ports fetch";
  }

}

void sourcectl_page::ParseEvent(sysadm_client::EVENT_TYPE type, QJsonValue val){
  if( type!=sysadm_client::DISPATCHER ){ return; }
  if(val.toObject().value("process_id").toString() != FETCH_PROCESS_ID ){ return; }
  qDebug() << "Got fetch process:" << val.toObject();
  bool isRunning = val.toObject().value("state").toString() == "running";
  qDebug() << " - is Running:" << isRunning;
}

void sourcectl_page::send_fetch_ports(){
  QJsonObject obj;
    obj.insert("action","fetch_ports");
    //obj.insert("ports_dir","/usr/ports-optional-dir");
  communicate(PAGE_ID+"fetch_ports", "sysadm", "systemmanager",obj);
}
