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
  connect(ui->tree_BE, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(updateButtons()) );
}

beadm_page::~beadm_page(){
  
}

//Initialize the CORE <-->Page connections
void beadm_page::setupCore(){

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

// === PRIVATE ===
void beadm_page::startingRequest(QString notice){
  this->setEnabled(false);
  ui->label_status->setText(notice);
  ui->label_status->setVisible(true);
}

// === PRIVATE SLOTS ===
void beadm_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  qDebug() << "New Reply:" << id << namesp << name << args;
  if(!id.startsWith("beadm_auto")){ return; }
  if(id=="beadm_auto_page_list" && name!="error"){
    //populate the tree widget with the info
    if(args.isObject() && args.toObject().contains("listbes")){
      ui->tree_BE->clear();
      QJsonObject LO = args.toObject().value("listbes").toObject(); //List Object
      QStringList names = LO.keys();
      names.sort();
      for(int n=0; n<names.length(); n++){
	if(!LO.value(names[n]).isObject()){ continue; }
        QString active, date, mount, nick, space;
	QJsonObject NO = LO.value(names[n]).toObject(); //Name Object
	if(NO.contains("active")){ active = NO.value("active").toString(); }
	if(NO.contains("date")){ date = NO.value("date").toString(); }
	if(NO.contains("mount")){ mount = NO.value("mount").toString(); }
	if(NO.contains("nick")){ nick = NO.value("nick").toString(); }
	if(NO.contains("space")){ space = NO.value("space").toString(); }
	QTreeWidgetItem *it = new QTreeWidgetItem(QStringList() << names[n] <<nick<<active<<space<<mount<<date );
	ui->tree_BE->addTopLevelItem(it);
      }
      //Now update all the column sizes
      ui->tree_BE->header()->resizeSections(QHeaderView::ResizeToContents);
      ui->tree_BE->sortItems(5, Qt::DescendingOrder); //sort by date (newest first)
    }
    ui->label_status->setVisible(false);
    this->setEnabled(true);
    updateButtons();
  }else{
    updateList();
  }
  
}

void beadm_page::updateButtons(){
  QTreeWidgetItem *curr = ui->tree_BE->currentItem();
  ui->deleteBE->setEnabled(curr!=0 && (curr->text(2)=="-") );
  ui->cloneBE->setEnabled(curr!=0);
  ui->renameBE->setEnabled(curr!=0 && (curr->text(2)=="-"));
  ui->mountBE->setEnabled(curr!=0 && (curr->text(2)=="-") && (curr->text(4)=="-") );
  ui->unmountBE->setEnabled(curr!=0 && (curr->text(2)=="-") && (curr->text(4)!="-"));
  ui->activateBE->setEnabled(curr!=0 && !curr->text(2).contains("R") );
}

//GUI Buttons
void beadm_page::create_be(){
  QString newname = QInputDialog::getText(this, tr("New Boot Environment"), tr("Name:"));
  if(newname.isEmpty()){ return; }
  //Also verify that the name is not already used
  if( !ui->tree_BE->findItems(newname, Qt::MatchExactly, 0).isEmpty() ){ create_be(); return;}
  //Send the request
  QJsonObject obj;
    obj.insert("action","createbe");
    obj.insert("newbe",newname);
  communicate("beadm_auto_page_create", "sysadm", "beadm", obj);
  startingRequest(tr("Creating Boot Environment...") );
}

void beadm_page::clone_be(){
  if(ui->tree_BE->currentItem()==0){ return; } //nothing selected
  QString selbe = ui->tree_BE->currentItem()->text(0); //currently selected BE
  //Get the new name from the user
  QString newname = QInputDialog::getText(this, tr("New Boot Environment"), tr("Name:"));
  if(newname.isEmpty()){ return; }
  //Also verify that the name is not already used
  if( !ui->tree_BE->findItems(newname, Qt::MatchExactly, 0).isEmpty() ){ create_be(); return;}
  QJsonObject obj;
    obj.insert("action","createbe");
    obj.insert("newbe",newname);
    obj.insert("clonefrom",selbe);
  communicate("beadm_auto_page_clone", "sysadm", "beadm", obj);
  startingRequest(tr("Cloning Boot Environment...") );
}

void beadm_page::delete_be(){
  if(ui->tree_BE->currentItem()==0){ return; } //nothing selected
  QString selbe = ui->tree_BE->currentItem()->text(0); //currently selected BE
  QJsonObject obj;
    obj.insert("action","destroybe");
    obj.insert("target",selbe);
  communicate("beadm_auto_page_delete", "sysadm", "beadm", obj);
  startingRequest(tr("Deleting Boot Environment...") );
}

void beadm_page::rename_be(){
  if(ui->tree_BE->currentItem()==0){ return; } //nothing selected
  QString selbe = ui->tree_BE->currentItem()->text(0); //currently selected BE
  //Get the new name from the user
  QString newname = QInputDialog::getText(this, tr("Rename Boot Environment"), tr("Name:"), QLineEdit::Normal, selbe);
  if(newname.isEmpty() || newname==selbe){ return; }
  //Also verify that the name is not already used
  if( !ui->tree_BE->findItems(newname, Qt::MatchExactly, 0).isEmpty() ){ rename_be(); return;}
  QJsonObject obj;
    obj.insert("action","renamebe");
    obj.insert("source",selbe);
    obj.insert("target",newname);
  communicate("beadm_auto_page_rename", "sysadm", "beadm", obj);
  startingRequest(tr("Renaming Boot Environment...") );	
}

void beadm_page::mount_be(){
  if(ui->tree_BE->currentItem()==0){ return; } //nothing selected
  QString selbe = ui->tree_BE->currentItem()->text(0); //currently selected BE
  QJsonObject obj;
    obj.insert("action","mountbe");
    obj.insert("be",selbe);
    //obj.insert("mountpoint",""); 
  communicate("beadm_auto_page_mount", "sysadm", "beadm", obj);
  startingRequest(tr("Mounting Boot Environment...") );
}

void beadm_page::unmount_be(){
  if(ui->tree_BE->currentItem()==0){ return; } //nothing selected
  QString selbe = ui->tree_BE->currentItem()->text(0); //currently selected BE
  QJsonObject obj;
    obj.insert("action","unmountbe");
    obj.insert("be",selbe);
  communicate("beadm_auto_page_unmount", "sysadm", "beadm", obj);
  startingRequest(tr("Unmounting Boot Environment...") );
}

void beadm_page::activate_be(){
  if(ui->tree_BE->currentItem()==0){ return; } //nothing selected
  QString selbe = ui->tree_BE->currentItem()->text(0); //currently selected BE
  QJsonObject obj;
    obj.insert("action","activatebe");
    obj.insert("target",selbe);
  communicate("beadm_auto_page_activate", "sysadm", "beadm", obj);
  qDebug() << "Send Request:" << obj;
  startingRequest(tr("Activating Boot Environment...") );	
}

void beadm_page::updateList(){
  QJsonObject obj;
    obj.insert("action","listbes");
  communicate("beadm_auto_page_list", "sysadm", "beadm", obj);
  startingRequest(tr("Retrieving List...") );
}
