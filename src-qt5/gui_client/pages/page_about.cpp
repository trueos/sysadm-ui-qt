//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_about.h"
#include "ui_page_about.h" //auto-generated from the .ui file

about_page::about_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::page_about_ui){
  ui->setupUi(this);	
  connect(ui->tree_BE, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(updateButtons()) );
}

about_page::~about_page(){
  
}

//Initialize the CORE <-->Page connections
void about_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void about_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("About PC-BSD") );
  //Connect any signals/slots
	
  connect(ui->tree_BE, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(updateButtons()) );
  //Now run any CORE communications
  updateList();
}

// === PRIVATE ===
void about_page::startingRequest(QString notice){
  this->setEnabled(false);
  ui->label_status->setText(notice);
  ui->label_status->setVisible(true);
}

// === PRIVATE SLOTS ===

//GUI Buttons

