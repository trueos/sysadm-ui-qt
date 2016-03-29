//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_pkg.h"
#include "ui_page_pkg.h" //auto-generated from the .ui file

#define TAG QString("sysadm_pkg_auto_")

pkg_page::pkg_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::pkg_page_ui){
  ui->setupUi(this);
  local_showall = false;
  local_advmode = false;
  local_viewM = new QMenu(this);
    ui->tool_local_filter->setMenu(local_viewM);
    //Now populate the filter menu
    QAction *tmp = local_viewM->addAction(tr("View All Packages"));
      tmp->setCheckable(true);
      tmp->setChecked(local_showall);
      connect(tmp, SIGNAL(toggled(bool)), this, SLOT(update_local_viewall(bool)) );
    tmp = local_viewM->addAction(tr("View Advanced Options"));
      tmp->setCheckable(true);
      tmp->setChecked(local_showall);
      connect(tmp, SIGNAL(toggled(bool)), this, SLOT(update_local_viewadv(bool)) );
  //Change any flags as needed
  ui->tree_local->setMouseTracking(true); //allow hover-over events (status tip)
  //Setup the GUI connections
  connect(ui->check_local_all, SIGNAL(toggled(bool)), this, SLOT(update_local_pkg_check(bool)) );
  connect(ui->tree_local, SIGNAL(itemSelectionChanged()), this, SLOT(update_local_buttons()) );
  connect(ui->tool_local_rem, SIGNAL(clicked()), this, SLOT(send_local_rmpkgs()) );
  update_local_buttons();
}

pkg_page::~pkg_page(){
  
}

//Initialize the CORE <-->Page connections
void pkg_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
  connect(CORE, SIGNAL(NewEvent(sysadm_client::EVENT_TYPE, QJsonValue)), this, SLOT(ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void pkg_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("AppCafe") );
  //Now run any CORE communications
  CORE->registerForEvents(sysadm_client::DISPATCHER);

  send_local_update();
}

//Core requests
void pkg_page::send_local_update(){
  ui->tab_local->setEnabled(false);
  QJsonObject obj;
    obj.insert("action","pkg_info");
    obj.insert("repo","local");
    obj.insert("result","full");
  CORE->communicate(TAG+"list_local", "sysadm", "pkg",obj);	
}

void pkg_page::send_local_audit(){
  QJsonObject obj;
    obj.insert("action","pkg_audit");
  CORE->communicate(TAG+"list_audit", "sysadm", "pkg",obj);	
}

//Parsing Core Replies
void pkg_page::update_local_list(QJsonObject obj){
  QStringList origins = obj.keys();
  bool sort = ui->tree_local->topLevelItemCount()<1;
  //Quick removal of any items no longer installed
  for(int i=0; i<ui->tree_local->topLevelItemCount(); i++){
    if( !origins.contains(ui->tree_local->topLevelItem(i)->whatsThis(0)) ){
      delete ui->tree_local->takeTopLevelItem(i);
      i--;
    }
  }
  //QStringList fields;
  for(int i=0; i<origins.length(); i++){
    if(origins[i].simplified().isEmpty()){ continue; } //just in case we get any empty keys
    //fields << obj.value(origins[i]).toObject().keys(); fields.removeDuplicates();
    //Find the item for this origin (if one exists)
    QTreeWidgetItem *it = 0;
    for(int j=0; j<ui->tree_local->topLevelItemCount(); j++){
      if(ui->tree_local->topLevelItem(j)->whatsThis(0)==origins[i]){ it = ui->tree_local->topLevelItem(j); break; }
    }
    if(it==0){
      //No Item available - need to create one
      it = new QTreeWidgetItem();
      it->setWhatsThis(0,origins[i]);
      it->setCheckState(0,(ui->check_local_all->isChecked() ? Qt::Checked : Qt::Unchecked) );
    }
    //Remove this item as needed based on viewing options
    if(obj.value(origins[i]).toObject().contains("reverse_dependencies") && !local_showall){
      delete it; //this will also remove it from the tree widget if it already exists there
      continue;
    }
    //Update the info within the item
    it->setText(1, obj.value(origins[i]).toObject().value("name").toString() );
      it->setToolTip(1, obj.value(origins[i]).toObject().value("comment").toString() );
      it->setStatusTip(1, it->toolTip(1));
    it->setText(2, obj.value(origins[i]).toObject().value("version").toString() );
    it->setText(3, BtoHR(obj.value(origins[i]).toObject().value("flatsize").toString().toDouble()) );
    it->setText(4, origins[i].section("/",0,0) ); //category
    //Now the hidden data within each item
    it->setWhatsThis(1, obj.value(origins[i]).toObject().value("repository").toString() ); //which repo the pkg was installed from
    QStringList stat_ico;
    if(obj.value(origins[i]).toObject().value("locked").toString()=="1"){ stat_ico <<"lock"; }
    if(obj.value(origins[i]).toObject().contains("reverse_dependencies")){ stat_ico <<"req"; }
    it->setData(0,Qt::UserRole, stat_ico); //save this for later
    //More room for expansion later
    //Add this item as needed based on viewing options
    if(ui->tree_local->indexOfTopLevelItem(it)<0){
      ui->tree_local->addTopLevelItem(it);
    }
  }
  //qDebug() << "pkg fields:" << fields;
  //Re-sort the items if necessary
  if(sort){
    ui->tree_local->sortItems(1,Qt::AscendingOrder);
    for(int i=0; i<ui->tree_local->columnCount(); i++){ ui->tree_local->resizeColumnToContents(i); }
  }else if(!ui->tree_local->selectedItems().isEmpty()){
    ui->tree_local->scrollToItem(ui->tree_local->selectedItems().first());
  }
  //Now that we have a list of local packages, get the audit of them as well
  send_local_audit();
}

void pkg_page::update_local_audit(QJsonObject obj){
  //update the status icons for items as needed
  
}

//Bytes to human-readable conversion
QString pkg_page::BtoHR(double bytes){
  QStringList units; units << "B" << "K" << "M" << "G" << "T" << "P";
  int unit = 0;
  while(bytes > 1024 && unit <units.length() ){
    unit++;
    bytes = bytes/1024.0;
  }
  //Round off the final number to 2 decimel places
  bytes = qRound(bytes*100)/100.0;
  return ( QString::number(bytes)+units[unit] );
}

// === PRIVATE SLOTS ===
void pkg_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(TAG) || namesp=="error" || name=="error"){ return; }
  qDebug() << "Got Reply:" << id << args.toObject().keys();
  if( id==TAG+"list_local" && args.isObject() ){
    //Got update to the list of locally installed packages
    ui->tab_local->setEnabled(true);
    if(args.toObject().contains("pkg_info")){ update_local_list( args.toObject().value("pkg_info").toObject() ); }
  }else{
    qDebug() << " - arguments:" << args;
  }
}

void pkg_page::ParseEvent(sysadm_client::EVENT_TYPE type, QJsonValue val){
  if( type!=sysadm_client::DISPATCHER ){ return; }
  qDebug() << "Got Dispatcher Event:" << val;
  //Also check that this is a sysadm/pkg event (ignore all others)
  if(!val.toObject().contains("event_system") || val.toObject().value("event_system").toString()!="sysadm/pkg"){ return; }
  //Now go through and update the UI based on the type of action for this event
  QString act = val.toObject().value("action").toString();
  bool finished = (val.toObject().value("state").toString() == "finished");
  if( act=="pkg_remove" || act=="pkg_install" || act=="pkg_lock" || act=="pkg_unlock"){
    if(finished){ 
      //Need to update the list of installed packages    
      send_local_update();
    }else{
      // Need to update the list of pending processes
      // TO-DO
    }
  }else if(act=="pkg_audit" && finished){
    update_local_audit(val.toObject());
  }
}

//GUI Updates
void pkg_page::update_local_buttons(){
  ui->tool_local_lock->setVisible(local_advmode);
  ui->tool_local_unlock->setVisible(local_advmode);
  ui->tool_local_upgrade->setVisible(local_advmode);
  //Now see if there are any items checked and enable/disable buttons as needed	
  bool gotcheck = false;
  for(int i=0; i<ui->tree_local->topLevelItemCount() && !gotcheck; i++){
    gotcheck = (ui->tree_local->topLevelItem(i)->checkState(0)==Qt::Checked);
  }
  ui->tool_local_lock->setEnabled(gotcheck);
  ui->tool_local_unlock->setEnabled(gotcheck);
  ui->tool_local_rem->setEnabled(gotcheck);
}

void pkg_page::update_local_pkg_check(bool checked){
  for(int i=0; i<ui->tree_local->topLevelItemCount(); i++){
    ui->tree_local->topLevelItem(i)->setCheckState(0, (checked ? Qt::Checked : Qt::Unchecked) );
  }
}
void pkg_page::update_local_viewall(bool checked){
  local_showall = checked;
  send_local_update();
}

void pkg_page::update_local_viewadv(bool checked){
  local_advmode = checked;
  update_local_buttons();
}

//GUI -> Core Requests
void pkg_page::send_local_rmpkgs(){
  QStringList pkgs;
  for(int i=0; i<ui->tree_local->topLevelItemCount(); i++){
    if(ui->tree_local->topLevelItem(i)->checkState(0)==Qt::Checked){ pkgs << ui->tree_local->topLevelItem(i)->whatsThis(0); }
  }
  if(pkgs.isEmpty()){ return; } //nothing to do
  QJsonObject obj;
    obj.insert("action","pkg_remove");
    obj.insert("recursive","true"); //cleanup orphaned packages
    obj.insert("pkg_origins", QJsonArray::fromStringList(pkgs) );
  CORE->communicate(TAG+"pkg_remove", "sysadm", "pkg",obj);		
}
