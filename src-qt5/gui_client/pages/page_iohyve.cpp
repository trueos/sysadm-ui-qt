//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_iohyve.h"
#include "ui_page_iohyve.h" //auto-generated from the .ui file

#define PAGETAG QString("client_page_iohyve_")

iohyve_page::iohyve_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::iohyve_ui){
  ui->setupUi(this);	
  //Connect the ui signals/slots
  // - setup page
  connect(ui->push_setup, SIGNAL(clicked()), this, SLOT(request_setup()) );
  connect(ui->combo_zpool, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSetupOptions()));
  connect(ui->combo_nic, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSetupOptions()));
  // - vm page/tab
  connect(ui->tree_vms, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(checkVMSelection()) );
  connect(ui->push_vm_start, SIGNAL(clicked()), this, SLOT(request_vm_start()) );
  connect(ui->push_vm_stop, SIGNAL(clicked()), this, SLOT(request_vm_stop()) );
  // -iso page/tab
  connect(ui->tree_iso, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(checkISOSelection()) );	
  connect(ui->push_iso_fetch, SIGNAL(clicked()), this, SLOT(request_iso_fetch()) );
  connect(ui->push_iso_rename, SIGNAL(clicked()), this, SLOT(request_iso_rename()) );
  connect(ui->push_iso_remove, SIGNAL(clicked()), this, SLOT(request_iso_remove()) );
}

iohyve_page::~iohyve_page(){
  
}

//Initialize the CORE <-->Page connections
void iohyve_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
  connect(CORE, SIGNAL(NewEvent(sysadm_client::EVENT_TYPE, QJsonValue)), this, SLOT(ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue)) );
  CORE->registerForEvents(sysadm_client::DISPATCHER, true);
}

//Page embedded, go ahead and startup any core requests
void iohyve_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("iohyve VMs") );
  //Now run any CORE communications
  request_is_setup();
  
}


// === PRIVATE SLOTS ===
void iohyve_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(PAGETAG) || namesp=="error" || name=="error"){ return; } //not from this page
  qDebug() << "New Reply:" << namesp << name << args;
  if(id==PAGETAG+"setup"){
    request_is_setup(); //make sure it got setup properly
  }else if(id==PAGETAG+"setup_options_zfs"){
    ui->combo_zpool->clear();
    if(args.isObject()){
      QStringList pools = args.toObject().keys(); //don't need all the extra pool info, just the list
      pools.sort();
      ui->combo_zpool->addItems(pools);
    }
    checkSetupOptions();
  }else if(id==PAGETAG+"setup_options_nic"){
    ui->combo_nic->clear();
    if(args.isObject()){
      QStringList nics = args.toObject().keys(); //don't need all the extra pool info, just the list
      nics.sort();
      for(int i=0; i<nics.length(); i++){
	if(args.toObject().value(nics[i]).toObject().value("is_active").toString()=="true"){
	  QString desc = args.toObject().value(nics[i]).toObject().value("description").toString();
          ui->combo_nic->addItem(nics[i]+" ("+desc+")",nics[i]);
	}
      }
    }   
    checkSetupOptions();
  }else if(id==PAGETAG+"is_setup"){
    if(args.isObject() && args.toObject().contains("issetup")){
      if(args.toObject().value("issetup").toObject().contains("setup")){
        bool issetup = ("true"==args.toObject().value("issetup").toObject().value("setup").toString().toLower());
	if(issetup){
	  ui->stackedWidget->setCurrentWidget(ui->page_vms);
	  request_vm_list();
	  request_iso_list();
	}else{
	  ui->stackedWidget->setCurrentWidget(ui->page_setup);
	  request_setup_options();
	}
      }
    }
  }else if(id==PAGETAG+"list_vm"){
    ui->tree_vms->clear();
    if(args.isObject() && args.toObject().contains("listvms")){
      QJsonObject VMO = args.toObject().value("listvms").toObject();
      QStringList vms = VMO.keys();
      vms.sort();
      for(int i=0; i<vms.length(); i++){
	QString vmm, running, rcboot, desc;
	vmm = VMO.value(vms[i]).toObject().value("vmm").toString();
	running = VMO.value(vms[i]).toObject().value("running").toString();
	rcboot = VMO.value(vms[i]).toObject().value("rcboot").toString();
	desc = VMO.value(vms[i]).toObject().value("description").toString();
        ui->tree_vms->addTopLevelItem( new QTreeWidgetItem( QStringList()<< vms[i] << vmm << running << rcboot << desc ) );
      }
      ui->tree_vms->header()->resizeSections(QHeaderView::ResizeToContents);
      ui->tree_vms->sortItems(0, Qt::AscendingOrder);
    }
    checkVMSelection();
  }else if(id==PAGETAG+"list_iso"){
    ui->tree_iso->clear();
    if(args.isObject() && args.toObject().contains("listisos")){
      if(args.toObject().value("listisos").isArray()){
        QJsonArray arr = args.toObject().value("listisos").toArray();
	for(int i=0; i<arr.count(); i++){
	  ui->tree_iso->addTopLevelItem( new QTreeWidgetItem( QStringList() << arr.at(i).toString()) );
	}
      }
    }
    checkISOSelection();
  }else{
    //Other modification command - update the lists
    request_vm_list();
    request_iso_list();
  }
}

void iohyve_page::ParseEvent(sysadm_client::EVENT_TYPE evtype, QJsonValue val){
  if(evtype==sysadm_client::DISPATCHER){
    qDebug() << "Got Dispatcher Event:" << val;
  }
}

// -- core Requests
void iohyve_page::request_is_setup(){
  QJsonObject obj;
    obj.insert("action","issetup");
  CORE->communicate(PAGETAG+"is_setup","sysadm","iohyve", obj);  
}

void iohyve_page::request_setup(){
  QJsonObject obj;
    obj.insert("action","setup");
    obj.insert("pool",ui->combo_zpool->currentText()); //zpool
    obj.insert("nic",ui->combo_nic->currentData().toString()); //network interface
  CORE->communicate(PAGETAG+"setup","sysadm","iohyve", obj);  
}

void iohyve_page::request_setup_options(){
  //Request all the info necessary to setup iohyve
  QJsonObject obj;
    obj.insert("action","list_pools");
  CORE->communicate(PAGETAG+"setup_options_zfs","sysadm","zfs", obj);
  QJsonObject obj2;
    obj2.insert("action","list-devices");
  CORE->communicate(PAGETAG+"setup_options_nic","sysadm","network", obj2);
	
}

void iohyve_page::request_vm_list(){
  QJsonObject obj;
    obj.insert("action","listvms");
  CORE->communicate(PAGETAG+"list_vm","sysadm","iohyve", obj);
}

void iohyve_page::request_vm_start(){
  if(ui->tree_vms->currentItem()==0){ return; }
  QString vm = ui->tree_vms->currentItem()->text(0); //name column
   QJsonObject obj;
    obj.insert("action","start");
    obj.insert("name",vm);
  CORE->communicate(PAGETAG+"vm_start","sysadm","iohyve", obj); 
}

void iohyve_page::request_vm_stop(){
  if(ui->tree_vms->currentItem()==0){ return; }
  QString vm = ui->tree_vms->currentItem()->text(0); //name column
   QJsonObject obj;
    obj.insert("action","stop");
    obj.insert("name",vm);
    //obj.insert("force","true");
  CORE->communicate(PAGETAG+"vm_stop","sysadm","iohyve", obj); 	
}

// - ISO page
void iohyve_page::request_iso_list(){
  QJsonObject obj;
    obj.insert("action","listisos");
  CORE->communicate(PAGETAG+"list_iso","sysadm","iohyve", obj);
}

void iohyve_page::request_iso_fetch(){
  QString url = QInputDialog::getText(this, tr("ISO Fetch:"), tr("URL:"));
  if(url.isEmpty()){ return; } //cancelled
  //Validate the URL
  if(!QUrl(url).isValid()){
    QMessageBox::warning(this, tr("Invalid URL"), tr("Invalid ISO URL") );
    return;
  }
   QJsonObject obj;
    obj.insert("action","fetchiso");
    obj.insert("url",url);
  CORE->communicate(PAGETAG+"iso_fetch","sysadm","iohyve", obj); 
}

void iohyve_page::request_iso_rename(){
  if(ui->tree_iso->currentItem()==0){ return; }
  QString iso = ui->tree_iso->currentItem()->text(0); //name column
  //Now request the new name
  QString newname = QInputDialog::getText(this, tr("Rename ISO"), tr("New Name:"));
  //Make sure the new name does not already exist
  if(!ui->tree_iso->findItems(newname, Qt::MatchExactly).isEmpty()){
    //Name already in use - ask again
    request_iso_rename();
    return;
  }
  //Now send the request
   QJsonObject obj;
    obj.insert("action","renameiso");
    obj.insert("source",iso);
    obj.insert("target",newname);
  CORE->communicate(PAGETAG+"iso_rename","sysadm","iohyve", obj); 	
}

void iohyve_page::request_iso_remove(){
  if(ui->tree_iso->currentItem()==0){ return; }
  QString iso = ui->tree_iso->currentItem()->text(0); //name column
   QJsonObject obj;
    obj.insert("action","rmiso");
    obj.insert("target",iso);
  CORE->communicate(PAGETAG+"iso_remove","sysadm","iohyve", obj); 
}

// -- ui slots
void iohyve_page::checkSetupOptions(){
  ui->push_setup->setEnabled( !ui->combo_zpool->currentText().isEmpty() && !ui->combo_nic->currentData().toString().isEmpty() );
}

void iohyve_page::checkVMSelection(){
  QTreeWidgetItem *it = ui->tree_vms->currentItem();
  ui->push_vm_start->setEnabled(it!=0);
  ui->push_vm_stop->setEnabled(it!=0);
}

void iohyve_page::checkISOSelection(){
  QTreeWidgetItem *it = ui->tree_iso->currentItem();
  ui->push_iso_rename->setEnabled(it!=0);
  ui->push_iso_remove->setEnabled(it!=0);
}
