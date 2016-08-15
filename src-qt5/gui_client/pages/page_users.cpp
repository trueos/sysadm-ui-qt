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
  connect(ui->list_users, SIGNAL(currentRowChanged(int)), this, SLOT(updateUserSelection()) );
  //All the UI elements which need to re-run the check routine
  connect(ui->radio_pc_init, SIGNAL(toggled(bool)), this, SLOT(checkSelectionChanges()) );
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
  usersLoading = true;
  if(ui->list_users->currentItem()!=0){ sel = ui->list_users->currentItem()->whatsThis(); }
  ui->list_users->clear();
  QStringList users = userObj.keys();
  bool filter = ui->radio_standard->isChecked();
  QListWidgetItem *setdef = 0;
  for(int i=0; i<users.length(); i++){
    if(filter){
      //Verify that the user has a valid shell, homedir, and UID>=1000
      if(userObj.value(users[i]).toObject().value("uid").toString().toInt() < 1000){ continue; }
      if(userObj.value(users[i]).toObject().value("shell").toString().toLower().contains("nologin")){ continue; }
      QString home = userObj.value(users[i]).toObject().value("home_dir").toString();
      if(home.contains("nonexistent") || home.contains("/var/")){ continue; }
    }
    QListWidgetItem *it = new QListWidgetItem(ui->list_users);
    it->setWhatsThis(users[i]);
    if(users[i]==sel || setdef==0){ setdef = it; }
    it->setText( userObj.value(users[i]).toObject().value("name").toString()+" ("+userObj.value(users[i]).toObject().value("comment").toString()+")" );
    if(userObj.value(users[i]).toObject().value("personacrypt_enabled").toString() == "true"){
      it->setIcon( QIcon(":/icons/black/lock.svg") );
    }
  }
  if(setdef!=0){ ui->list_users->setCurrentItem(setdef); }
  QCoreApplication::processEvents(); //throw away the list changed signals so far
  usersLoading = false;
  if(ui->list_users->currentItem()!=0){ ui->list_users->scrollToItem(ui->list_users->currentItem(),QAbstractItemView::PositionAtCenter); }
  updateUserSelection();
}

void users_page::updateUserSelection(){ //uses the userObj variable
  if(usersLoading){ return; } //stray signal from the clear/reload routine
  QString cuser;
  if(ui->list_users->currentItem()!=0){ cuser = ui->list_users->currentItem()->whatsThis(); }
  if(!cuser.isEmpty()){
    //Currently-existing user
    // - adjust UI elements
    ui->check_user_autouid->setVisible(false); //can't change existing UID
    ui->check_user_autouid->setChecked(false); //can't change existing UID
    ui->spin_user_uid->setVisible(true);
    ui->spin_user_uid->setEnabled(false); //can't change existing UID
    ui->check_pc_disable->setChecked(false);
    ui->group_pc_enable->setChecked(false);
    // - load the data
    QJsonObject uobj = userObj.value(cuser).toObject();
    ui->line_user_name->setText( uobj.value("name").toString() );
    ui->line_user_comment->setText( uobj.value("comment").toString() );
    ui->spin_user_uid->setValue( uobj.value("uid").toString().toInt() );
    ui->line_user_home->setText( uobj.value("home_dir").toString() );
    ui->line_user_shell->setText( uobj.value("shell").toString() );
    bool haspc = uobj.contains("personacrypt_enabled");
    ui->group_pc_enable->setVisible(!haspc);
    ui->check_pc_disable->setVisible(haspc);
  }else{
    //New User
    // - adjust UI elements
    ui->check_user_autouid->setVisible(true);
    ui->check_user_autouid->setChecked(true);
    ui->spin_user_uid->setEnabled(true); 
    ui->check_pc_disable->setVisible(false);
    ui->check_pc_disable->setChecked(false);
    ui->group_pc_enable->setVisible(true);
    ui->group_pc_enable->setChecked(false);  
    // - unload the data
    ui->line_user_name->clear();
    ui->line_user_comment->clear();
    ui->line_user_home->clear();
    ui->line_user_shell->clear();
    ui->tabWidget_users->setCurrentWidget(ui->tab_user_details); //show main tab
    ui->line_user_name->setFocus(); //focus on the first input box

  }
  checkSelectionChanges();
}

void users_page::checkSelectionChanges(){ //uses the userObj variable (validate manual UID selection)
  ui->spin_user_uid->setVisible( !ui->check_user_autouid->isChecked() );
  bool initpc = ui->radio_pc_init->isChecked();
  ui->label_6->setVisible(initpc); ui->line_pc_password->setVisible(initpc);  //password init options
  ui->label_7->setVisible(initpc); ui->combo_pc_device->setVisible(initpc);  //device init options
  ui->label_8->setVisible(!initpc); ui->line_pc_key->setVisible(!initpc); ui->tool_pc_findkey->setVisible(!initpc); //key import options
  QString uname = ui->line_user_name->text();
  if(!uname.isEmpty()){
    if(ui->line_user_home->text().isEmpty()){ ui->line_user_home->setText("/home/"+uname); }
    if(ui->line_user_shell->text().isEmpty()){ ui->line_user_shell->setText("/bin/csh"); }
  }
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
