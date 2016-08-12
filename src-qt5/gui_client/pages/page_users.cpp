//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_users.h"
#include "ui_page_users.h" //auto-generated from the .ui file

#define USERTAG QString("sysadm_client_user_")

users_page::users_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::users_ui){
  ui->setupUi(this);	
  connect(ui->radio_standard, SIGNAL(toggled(bool)), this, SLOT(updateUserList()) );
}

users_page::~users_page(){
  
}

//Initialize the CORE <-->Page connections
void users_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void users_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("User Manager") );
  //Now run any CORE communications
  send_list_users();
}

void users_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(USERTAG)){ return; } //not interested
  bool iserror = (name.toLower()=="error") || (namesp.toLower()=="error");
  if(id==(USERTAG+"list_users")){
    if(!iserror){ userObj = args.toObject(); }
    updateUserList();
  }

}

void users_page::updateUserList(){ //uses the userObj variable
  QString sel;
  if(ui->list_users->currentItem()!=0){ sel = ui->list_users->currentItem()->whatsThis(); }
  ui->list_users->clear();
  QStringList users = userObj.keys();
  bool filter = ui->radio_standard->isChecked();
  //int setdef = -1;
  for(int i=0; i<users.length(); i++){
    if(filter){
      //Verify that the user has a valid shell, homedir, and UID>=1000
      if(userObj.value(users[i]).toObject().value("uid").toString().toInt() < 1000){ continue; }
      if(userObj.value(users[i]).toObject().value("shell").toString().toLower().contains("nologin")){ continue; }
      QString home = userObj.value(users[i]).toObject().value("home_dir").toString();
      if(home.contains("nonexistent") || home.contains("/var/")){ continue; }
    }
    QListWidgetItem *it = new QListWidgetItem();
    it->setWhatsThis(users[i]);
    //if(users[i]==sel){ setdef = ui->list_users->count(); }
    it->setText( userObj.value(users[i]).toObject().value("name").toString()+" ("+userObj.value(users[i]).toObject().value("comment").toString()+")" );
    if(userObj.value(users[i]).toObject().value("personacrypt_enabled").toString() == "true"){
      it->setIcon( QIcon(":/icons/black/lock.svg") );
    }
    ui->list_users->addItem(it);
  }
  /*QCoreApplication::processEvents();
  if(setdef>=0){ setdef = 0; }
  QListWidgetItem *it = ui->list_users->item(setdef);
  if(it!=0){ qDebug() << "previous selection:" << it->text() << sel; }
  ui->list_users->setCurrentItem(it);
  ui->list_users->scrollToItem(it,QAbstractItemView::PositionAtCenter);*/
}

void users_page::updateUserSelection(){ //uses the userObj variable

}

void users_page::checkUidSelection(){ //uses the userObj variable (validate manual UID selection)

}

//Core Request routines
void users_page::send_list_users(){
  QJsonObject obj;
    obj.insert("action","usershow");
  communicate(USERTAG+"list_users", "sysadm", "users",obj);
}

void users_page::send_user_save(){

}

void users_page::send_user_remove(){

}
