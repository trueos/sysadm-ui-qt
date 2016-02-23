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
  //Connect any signals/slots
  connect(ui->createBE, SIGNAL(clicked()), this, SLOT(create_be()) );
  connect(ui->deleteBE, SIGNAL(clicked()), this, SLOT(delete_be()) );
  connect(ui->renameBE, SIGNAL(clicked()), this, SLOT(rename_be()) );
  connect(ui->mountBE, SIGNAL(clicked()), this, SLOT(mount_be()) );
  connect(ui->unmountBE, SIGNAL(clicked()), this, SLOT(unmount_be()) );
  connect(ui->activateBE, SIGNAL(clicked()), this, SLOT(activate_be()) );
	
  connect(ui->tree_BE, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(updateButtons()) );
  //Now run any CORE communications
  updateList();
  
}


// === PRIVATE SLOTS ===
void beadm_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith("beadm_auto")){ return; }
  if(id=="beadm_auto_page_list"){
    //populate the tree widget with the info
	  
  }else{
    updateList();
  }
  
}

void beadm_page::updateButtons(){
  QTreeWidgetItem *curr = ui->tree_BE->currentItem();
	
}

//GUI Buttons
void beadm_page::create_be(){
	
}

void beadm_page::delete_be(){
	
}

void beadm_page::rename_be(){
	
}

void beadm_page::mount_be(){
	
}

void beadm_page::unmount_be(){
	
}

void beadm_page::activate_be(){
	
}

void beadm_page::updateList(){
  QJsonObject obj;
    obj.insert("action","listbes");
  CORE->communicate("beadm_auto_page_list", "sysadm", "beadm", obj);
}

