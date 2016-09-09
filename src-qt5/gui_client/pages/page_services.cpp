//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_services.h"
#include "ui_page_services.h" //auto-generated from the .ui file

#define STAG QString("sysadm_services_")

services_page::services_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::services_ui){
  ui->setupUi(this);
  connect(ui->tool_start, SIGNAL(clicked()), this, SLOT(send_start_services()) );
  connect(ui->tool_stop, SIGNAL(clicked()), this, SLOT(send_stop_services()) );
  connect(ui->tool_restart, SIGNAL(clicked()), this, SLOT(send_restart_services()) );
  connect(ui->tool_enable, SIGNAL(clicked()), this, SLOT(send_enable_services()) );
  connect(ui->tool_disable, SIGNAL(clicked()), this, SLOT(send_disable_services()) );
  connect(ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()) );
}

services_page::~services_page(){
  
}

//Initialize the CORE <-->Page connections
void services_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void services_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Service Manager") );
  //Now run any CORE communications
  send_list_services();
}

void services_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(STAG)){ return; } //not something this page needs
   //qDebug() << "Got services reply:" << id << args;
  bool haserror = (name=="error" || namesp=="error");
  if(id==STAG+"list"){
    ui->label_sync->setVisible(false);
    this->setEnabled(true);
    //keep the current item selection if there is one
    bool firstrun = (ui->treeWidget->topLevelItemCount()<1);
    QStringList list = args.toObject().value("services").toObject().keys();
    qDebug() << "Got list:" << firstrun << list;
    //Update all the current items first
    for(int i=0; i<ui->treeWidget->topLevelItemCount(); i++){
      QTreeWidgetItem *it = ui->treeWidget->topLevelItem(i);
      if(list.contains( it->text(0) ) ){
        //Update Item details
        QJsonObject info = args.toObject().value("services").toObject().value(it->text(0)).toObject();
        it->setText(1, info.value("is_running").toString());
        it->setText(2, info.value("is_enabled").toString());
        it->setText(3, info.value("description").toString());
        it->setToolTip(0, info.value("path").toString());
        it->setToolTip(2, info.value("tag").toString());
        list.removeAll( it->text(0) ); //already handled - remove from list
      }else{
        //Remove item - no longer available
        delete it; i--;
      }
    }
    //Add in any new items
    for(int i=0; i<list.length(); i++){
      QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidget);
        QJsonObject info = args.toObject().value("services").toObject().value(list[i]).toObject();
        it->setText(0, info.value("name").toString());
        it->setText(1, info.value("is_running").toString());
        it->setText(2, info.value("is_enabled").toString());
        it->setText(3, info.value("description").toString());
        it->setToolTip(0, info.value("path").toString());
        it->setToolTip(2, info.value("tag").toString());
      ui->treeWidget->addTopLevelItem(it);
    }
    //Adjust the sorting/display if this is the first run
    if(firstrun){
      ui->treeWidget->sortByColumn(0, Qt::AscendingOrder);
      for(int i=0; i<ui->treeWidget->columnCount(); i++){
        ui->treeWidget->resizeColumnToContents(i);
      }
    }
  }else{
    send_list_services(); //update the list - something changed
  }
}

void services_page::send_list_services(){
  ui->label_sync->setVisible(true);
  this->setEnabled(false);

  QJsonObject obj;
    obj.insert("action","list_services");
  communicate(STAG+"list", "sysadm", "services",obj);
}

void services_page::send_start_services(){
  //Get the currently selected services
  QStringList sel;
  QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
  for(int i=0; i<items.length(); i++){
    if(items[i]->text(1).toLower()!="true"){ sel << items[i]->text(0); }
  }
  if(sel.isEmpty()){ return; } //nothing to do
  QJsonObject obj;
    obj.insert("action","start");
    obj.insert("services", QJsonArray::fromStringList(sel));
  communicate(STAG+"start", "sysadm", "services",obj);
}

void services_page::send_stop_services(){
  //Get the currently selected services
  QStringList sel;
  QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
  for(int i=0; i<items.length(); i++){
    if(items[i]->text(1).toLower()=="true"){ sel << items[i]->text(0); }
  }
  if(sel.isEmpty()){ return; } //nothing to do
  QJsonObject obj;
    obj.insert("action","stop");
    obj.insert("services", QJsonArray::fromStringList(sel));
  communicate(STAG+"stop", "sysadm", "services",obj);
}

void services_page::send_restart_services(){
  //Get the currently selected services
  QStringList sel;
  QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
  for(int i=0; i<items.length(); i++){
    if(items[i]->text(1).toLower()=="true"){ sel << items[i]->text(0); }
  }
  if(sel.isEmpty()){ return; } //nothing to do
  QJsonObject obj;
    obj.insert("action","restart");
    obj.insert("services", QJsonArray::fromStringList(sel));
  communicate(STAG+"restart", "sysadm", "services",obj);
}

void services_page::send_enable_services(){
  //Get the currently selected services
  QStringList sel;
  QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
  for(int i=0; i<items.length(); i++){
    if(items[i]->text(2).toLower()!="true"){ sel << items[i]->text(0); }
  }
  if(sel.isEmpty()){ return; } //nothing to do
  QJsonObject obj;
    obj.insert("action","enable");
    obj.insert("services", QJsonArray::fromStringList(sel));
  communicate(STAG+"enable", "sysadm", "services",obj);
}

void services_page::send_disable_services(){
  //Get the currently selected services
  QStringList sel;
  QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
  for(int i=0; i<items.length(); i++){
    if(items[i]->text(2).toLower()=="true"){ sel << items[i]->text(0); }
  }
  if(sel.isEmpty()){ return; } //nothing to do
  QJsonObject obj;
    obj.insert("action","disable");
    obj.insert("services", QJsonArray::fromStringList(sel));
  communicate(STAG+"disable", "sysadm", "services",obj);
}


void services_page::updateSelection(){

}
