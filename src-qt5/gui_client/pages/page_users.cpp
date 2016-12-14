//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_users.h"
#include "ui_page_users.h" //auto-generated from the .ui file
#include <QRegExpValidator>

#define USERTAG QString("sysadm_client_user_")

users_page::users_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::users_ui){
  ui->setupUi(this);
  msgbox = 0;
  connect(ui->radio_standard, SIGNAL(toggled(bool)), this, SLOT(updateUserList()) );
  connect(ui->list_users, SIGNAL(currentRowChanged(int)), this, SLOT(updateUserSelection()) );
  //All the UI elements which need to re-run the check routine
  connect(ui->radio_pc_init, SIGNAL(toggled(bool)), this, SLOT(checkSelectionChanges()) );
  connect(ui->line_user_name, SIGNAL(editingFinished()), this, SLOT(generateUserDefaults()) );
  connect(ui->line_user_password, SIGNAL(textChanged(const QString&)), this, SLOT(checkSelectionChanges()) );
  connect(ui->line_user_comment, SIGNAL(editingFinished()), this, SLOT(checkSelectionChanges()) );
  connect(ui->line_user_home, SIGNAL(textChanged(const QString&)), this, SLOT(checkSelectionChanges()) );
  connect(ui->line_user_shell, SIGNAL(textChanged(const QString&)), this, SLOT(checkSelectionChanges()) );
  connect(ui->line_pc_password, SIGNAL(textChanged(const QString&)), this, SLOT(checkSelectionChanges()) );
  connect(ui->check_user_autouid, SIGNAL(toggled(bool)), this, SLOT(checkSelectionChanges()) );
  connect(ui->spin_user_uid, SIGNAL(valueChanged(int)), this, SLOT(validateUserChanges()) );
  connect(ui->tool_refresh_pcdevs, SIGNAL(clicked()), this, SLOT(send_update_pcdevs()) );
  connect(ui->tool_user_showpassword, SIGNAL(toggled(bool)), this, SLOT(checkSelectionChanges()) );
  connect(ui->tool_pc_showpassword, SIGNAL(toggled(bool)), this, SLOT(checkSelectionChanges()) );
  connect(ui->tool_pc_showpassword_disable, SIGNAL(toggled(bool)), this, SLOT(checkSelectionChanges()) );
  connect(ui->group_pc_enable, SIGNAL(toggled(bool)), this, SLOT(checkSelectionChanges()) );
  ui->tabWidget->setCurrentWidget(ui->tab); //make sure the Users tab is shown initially
  ui->tabWidget_users->setCurrentWidget(ui->tab_user_details); //make sure the user details are shown initially

  connect(ui->radio_group_standard, SIGNAL(toggled(bool)), this, SLOT(updateGroupList()) );
  connect(ui->list_groups, SIGNAL(currentRowChanged(int)), this, SLOT(updateGroupSelection()) );

  //Setup all the validators for the line edits
  ui->line_user_name->setValidator( new QRegExpValidator(QRegExp("[\\w-.]{0,16}"), this) );
  ui->line_user_comment->setValidator( new QRegExpValidator(QRegExp("[\\w-.@, ]{0,80}"), this) );
  //TEMPORARY: Disable the "groups" tab until it is finished
  //ui->tabWidget->setTabEnabled(1,false);
  ui->push_group_new->setVisible(false); //not finished yet
  ui->push_group_remove->setVisible(false); //not finished yet
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
  send_list_groups();
  send_update_pcdevs();
}

void users_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(USERTAG)){ return; } //not interested
  bool iserror = (name.toLower()=="error") || (namesp.toLower()=="error");
  QString errtext;
  if(iserror && args.toObject().contains("error")){ errtext = args.toObject().value("error").toString(); }
  if(id==USERTAG+"add_user"){ qDebug() << "Got Message:" << id << iserror << errtext << args; }
  if(id==(USERTAG+"list_users")){
    if(!iserror){ userObj = args.toObject(); }
    updateUserList();

  }else if(id==(USERTAG+"list_groups")){
    if(!iserror){ groupObj = args.toObject(); }
    updateGroupList();

  }else if(id==(USERTAG+"list_pcdevs")){
    QStringList devs = args.toObject().keys();
    ui->combo_pc_device->clear();
    for(int i=0; i<devs.length(); i++){
      ui->combo_pc_device->addItem(args.toObject().value(devs[i]).toString(), devs[i]);
    }
    validateUserChanges();

  }else if(id==(USERTAG+"remove_user") || id==(USERTAG+"add_user") || id==(USERTAG+"modify_user") ){
    if(iserror){
      if(id.endsWith("remove_user")){ ShowError(tr("Could not remove user"), errtext); }
      if(id.endsWith("add_user")){ ShowError(tr("Could not create user"), errtext); }
      if(id.endsWith("modify_user")){ ShowError(tr("Could not modify user"), errtext); }
    }
    send_list_users(); //update the user lists
    send_list_groups(); //update the groups (automatically get changed if users are changed)
  }else if(id==(USERTAG+"modify_group")){
    if(iserror){ ShowError(tr("Could not modify group"), errtext); }
    send_list_groups(); //does not change user info
  }

}

void users_page::ShowError(QString msg, QString details){
  if(msgbox==0){ 
    msgbox = new QMessageBox(this);
    msgbox->setIcon(QMessageBox::Warning);
    msgbox->setWindowTitle( tr("Error") );
    msgbox->addButton( QMessageBox::Ok );
  }
  msgbox->setDetailedText(details);
  msgbox->setText(msg);
  msgbox->show();
}

void users_page::updateUserList(){ //uses the userObj variable
  QString sel;
  usersLoading = true;
  ui->list_group_users->clear();
  ui->list_group_users->addItems( userObj.keys() );
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
  ui->line_user_password->clear();
  ui->line_pc_password->clear();
  ui->line_pc_password_disable->clear();
  ui->push_user_remove->setEnabled( !cuser.isEmpty() && userObj.value(cuser).toObject().value("canremove").toString()!="false" );
  if(!cuser.isEmpty()){
    //Currently-existing user
    // - adjust UI elements
    ui->group_user_newperms->setVisible(false);
    ui->check_user_autouid->setVisible(false); //can't change existing UID
    ui->check_user_autouid->setChecked(false); //can't change existing UID
    ui->spin_user_uid->setVisible(true);
    ui->spin_user_uid->setEnabled(false); //can't change existing UID
    ui->group_pc_disable->setChecked(false);
    ui->group_pc_enable->setChecked(false);
    // - load the data
    QJsonObject uobj = userObj.value(cuser).toObject();
    ui->line_user_name->setText( uobj.value("name").toString() );
    ui->line_user_comment->setText( uobj.value("comment").toString() );
    ui->spin_user_uid->setValue( uobj.value("uid").toString().toInt() );
    ui->line_user_home->setText( uobj.value("home_dir").toString() );
    ui->line_user_shell->setText( uobj.value("shell").toString() );
    bool haspc = uobj.contains("personacrypt_enabled");
    ui->group_pc_enable->setVisible(false); //only allow init on new users (12/14/16)
    ui->group_pc_disable->setVisible(haspc);
  }else{
    //New User
    // - adjust UI elements
    ui->group_user_newperms->setVisible(true);
    ui->check_user_autouid->setVisible(true);
    ui->check_user_autouid->setChecked(true);
    ui->spin_user_uid->setEnabled(true); 
    ui->group_pc_disable->setVisible(false);
    ui->group_pc_disable->setChecked(false);
    ui->group_pc_enable->setVisible(true);
    ui->group_pc_enable->setChecked(false);  
    // - unload the data
    ui->line_user_name->clear();
    ui->line_user_comment->clear();
    ui->line_user_home->clear();
    ui->line_user_shell->clear();
    ui->tabWidget_users->setCurrentWidget(ui->tab_user_details); //show main tab
    ui->line_user_comment->setFocus(); //focus on the first input box

  }
  checkSelectionChanges();
}

void users_page::checkSelectionChanges(){ //uses the userObj variable (validate manual UID selection)
  ui->spin_user_uid->setVisible( !ui->check_user_autouid->isChecked() );
  bool initpc = ui->radio_pc_init->isChecked();
  ui->label_6->setVisible(initpc); ui->line_pc_password->setVisible(initpc);  ui->tool_pc_showpassword->setVisible(initpc); //password init options
  ui->label_7->setVisible(initpc); ui->combo_pc_device->setVisible(initpc);  ui->tool_refresh_pcdevs->setVisible(initpc); //device init options
  ui->label_8->setVisible(!initpc); ui->line_pc_key->setVisible(!initpc); ui->tool_pc_findkey->setVisible(!initpc); //key import options
  if(!ui->line_user_comment->text().isEmpty() && ui->line_user_name->text().isEmpty()){
    //Automatically generate a user name based on the real name
    QStringList words = ui->line_user_comment->text().section(",",0,0).split(" ", QString::SkipEmptyParts);
    QString tmp;
    for(int i=0; i<words.length(); i++){
      if(i==words.length()-1){ tmp.append(words[i].toLower()); } //last word - use full thing
      else{ tmp.append( words[i].toLower()[0] ); } //Just first letter from word
    }
    ui->line_user_name->setText(tmp);
  }
  ui->line_user_password->setEchoMode( ui->tool_user_showpassword->isChecked() ? QLineEdit::Normal : QLineEdit::Password);
  ui->line_pc_password->setEchoMode( ui->tool_pc_showpassword->isChecked() ? QLineEdit::Normal : QLineEdit::Password);
  ui->line_pc_password_disable->setEchoMode( ui->tool_pc_showpassword_disable->isChecked() ? QLineEdit::Normal : QLineEdit::Password);
  validateUserChanges();
}

void users_page::validateUserChanges(){
  bool good = true;
  //UID SETTING
  if(ui->spin_user_uid->isEnabled() && !ui->check_user_autouid->isChecked()){
    //Need to validate the current UID
    QStringList users = userObj.keys();
    int cval = ui->spin_user_uid->value();
    for(int i=0; i<users.length(); i++){
      if(userObj.value(users[i]).toObject().value("uid").toString().toInt() == cval){good = false;  break; } //already found a conflict
    }
  }
  ui->spin_user_uid->setStyleSheet( good ? "" : "QSpinBox{ color: red; selection-background-color: red; selection-color: white; }" );
  //PersonaCrypt Settings
  if(ui->group_pc_enable->isChecked()){
    bool pcgood = true;
    if(ui->radio_pc_init->isChecked()){
      if(ui->line_pc_password->text().isEmpty()){
        pcgood = false;
      }
      ui->line_pc_password->setStyleSheet( pcgood ? "" : "QLineEdit{ border: 1px solid red; color: red;  border-radius: 4px; }" );
      bool devgood = !ui->combo_pc_device->currentData().toString().isEmpty();
      ui->combo_pc_device->setStyleSheet( devgood ? "" : "QComboBox{ color: red; border: 1px solid red; border-radius: 4px; }" );
      pcgood = pcgood && devgood;
    }else{
      pcgood = QFile::exists(ui->line_pc_key->text()) && !ui->line_pc_key->text().isEmpty();
      ui->line_pc_key->setStyleSheet( pcgood ? "" : "QLineEdit{ border: 1px solid red; color: red; }" );
    }
    good = (good && pcgood);
  }else{
    ui->line_pc_password->setStyleSheet("");
    ui->combo_pc_device->setStyleSheet("");
    ui->line_pc_key->setStyleSheet("");
  }
  //Other General User Settings
  good = good && !ui->line_user_name->text().isEmpty();
  ui->line_user_name->setStyleSheet( ui->line_user_name->text().isEmpty() ? "QLineEdit{ border: 1px solid red; color: red;  border-radius: 4px; }" : "");
  bool pwgood = (!ui->line_user_password->text().isEmpty() ||  ui->list_users->currentItem()!=0);
  ui->line_user_password->setStyleSheet( pwgood ? "" : "QLineEdit{ border: 1px solid red; color: red;  border-radius: 4px; }");
  good = good && pwgood;
  ui->push_user_save->setEnabled(good);
}

void users_page::generateUserDefaults(){
  QString uname = ui->line_user_name->text();
  if(!uname.isEmpty()){
    if(ui->line_user_home->text().isEmpty()){ ui->line_user_home->setText("/home/"+uname); }
    if(ui->line_user_shell->text().isEmpty()){ ui->line_user_shell->setText("/bin/csh"); }
  }
  checkSelectionChanges();
}

void users_page::updateGroupList(){
   //uses the groupObj variable
  groupsLoading = true;
  QString sel;
  if(ui->list_groups->currentItem()!=0){ sel = ui->list_groups->currentItem()->whatsThis(); }
  ui->list_groups->clear();
  QStringList grps = groupObj.keys();
  bool filter = ui->radio_group_standard->isChecked();
  QListWidgetItem *setdef = 0;
  for(int i=0; i<grps.length(); i++){
    if(filter){
      //Only show the main groups (wheel, operator)
      if(grps[i]!="wheel" && grps[i]!="operator" && !grps[i].contains("users") ){ continue; }
    }
    QListWidgetItem *it = new QListWidgetItem(ui->list_groups);
    it->setWhatsThis(grps[i]);
    if(grps[i]==sel || setdef==0){ setdef = it; }
    QJsonArray users = groupObj.value(grps[i]).toObject().value("users").toArray();
    if(users.count()==1 && users[0].toString().isEmpty()){ users = QJsonArray(); } //check for an empty array
    it->setText(grps[i]+" (gid: "+groupObj.value(grps[i]).toObject().value("gid").toString()+", users: "+QString::number( users.count())+")" );
  }
  if(setdef!=0){ ui->list_groups->setCurrentItem(setdef); }
  QCoreApplication::processEvents(); //throw away the list changed signals so far
  groupsLoading = false;
  if(ui->list_groups->currentItem()!=0){ ui->list_users->scrollToItem(ui->list_groups->currentItem(),QAbstractItemView::PositionAtCenter); }
  updateGroupSelection();
}

void users_page::updateGroupSelection(){
  if(groupsLoading){ return; } //in the middle of changing the lists around
  QStringList users;
  if(ui->list_groups->currentItem()!=0){ 
    QJsonArray uarr = groupObj.value( ui->list_groups->currentItem()->whatsThis() ).toObject().value("users").toArray();
    for(int i=0; i<uarr.count(); i++){ 
      if(uarr[i].toString().simplified().isEmpty()){ continue; } //skip empty entries
      users << uarr[i].toString(); 
    }
  }
  ui->list_group_members->clear();
  ui->list_group_members->addItems(users);
  ui->list_group_members->sortItems(Qt::AscendingOrder);
  validateGroupSettings();
}

void users_page::validateGroupSettings(){
  QString cgroup;
  if(ui->list_groups->currentItem()!=0){ cgroup = ui->list_groups->currentItem()->whatsThis(); }
  bool ok = false;
  if(cgroup.isEmpty()){
    //New Group - check different inputs

  }else{
    //Existing Group - check for difference in users
    QStringList cur, members;
    QJsonArray uarr = groupObj.value(cgroup).toObject().value("users").toArray();
    //Read off all the current users in this group from master object
    for(int i=0; i<uarr.count(); i++){ 
      if( !uarr[i].toString().isEmpty() ){ cur << uarr[i].toString(); }
    }
    //Now get all the members listed in the UI
    for(int i=0; i<ui->list_group_members->count(); i++){
      members << ui->list_group_members->item(i)->text();
    }
    //Now see if the two lists are different
    ok = (cur.length()!=members.length()); //different lengths mean changes made
    if(!ok){
      //same length - need to look for differences in items
      cur.sort();
      members.sort();
      for(int i=0; i<cur.length() && !ok; i++){
        ok = (cur[i] != members[i]);
      }
    }
  }
  ui->push_group_save->setEnabled(ok);
}

//Core Request routines
void users_page::send_list_users(){
  QJsonObject obj;
    obj.insert("action","usershow");
  communicate(USERTAG+"list_users", "sysadm", "users",obj);
}

void users_page::send_user_save(){
  if(ui->list_users->currentItem()==0){
    //New User
    QJsonObject obj;
    obj.insert("action","useradd");
    obj.insert("name", ui->line_user_name->text() );
    obj.insert("password", ui->line_user_password->text());
    if(!ui->line_user_comment->text().isEmpty()){ obj.insert("comment", ui->line_user_comment->text()); }
    if(!ui->check_user_autouid->isChecked()){ obj.insert("uid", QString::number(ui->spin_user_uid->value()) ); }
    QString home = ui->line_user_home->placeholderText();
    if(!ui->line_user_home->text().isEmpty()){ home = ui->line_user_home->text(); }
    obj.insert("home_dir", home);
     QString shell = ui->line_user_shell->placeholderText();
    if(!ui->line_user_shell->text().isEmpty()){ shell = ui->line_user_shell->text(); }
    obj.insert("shell", shell);   
    QStringList groups;
    if(ui->check_user_wheelpriv->isChecked()){ groups << "wheel"; }
    if(ui->check_user_operatorpriv->isChecked()){ groups << "operator"; }
    if(!groups.isEmpty()){ obj.insert("other_groups", QJsonArray::fromStringList(groups)); }
    //PersonaCrypt Options
    if(ui->group_pc_enable->isChecked()){
      if(ui->radio_pc_init->isChecked()){
        //Initialize new device
        obj.insert("personacrypt_init",ui->combo_pc_device->currentData().toString());
        obj.insert("personacrypt_password",ui->line_pc_password->text() );
      }else{
        //import key from file
        QFile keyfile(ui->line_pc_key->text());
        if(keyfile.open(QIODevice::ReadOnly)){
          QByteArray key = keyfile.readAll().toBase64();
          keyfile.close();
           obj.insert("personacrypt_import", QString(key) );
        }
      }
    }
    communicate(USERTAG+"add_user", "sysadm", "users",obj);
  }else{
    //Modify Existing User
    QJsonObject obj;
    QString cuser = ui->list_users->currentItem()->whatsThis();
    obj.insert("action","usermod");
    obj.insert("name", cuser );
    // - username
    if(ui->line_user_name->text()!=cuser){
      obj.insert("newname", ui->line_user_name->text());
    }
    // - password
    if(!ui->line_user_password->text().isEmpty()){
      obj.insert("password", ui->line_user_password->text());
    }
    // - comment
    if(ui->line_user_comment->text()!=userObj.value(cuser).toObject().value("comment").toString()){ obj.insert("comment", ui->line_user_comment->text()); }
    // - home_dir
    QString home = ui->line_user_home->placeholderText();
    if(!ui->line_user_home->text().isEmpty()){ home = ui->line_user_home->text(); }
    if(home!=userObj.value(cuser).toObject().value("home_dir").toString()){ obj.insert("home_dir", home); }
     QString shell = ui->line_user_shell->placeholderText();
    if(!ui->line_user_shell->text().isEmpty()){ shell = ui->line_user_shell->text(); }
    if(shell!=userObj.value(cuser).toObject().value("shell").toString()){ obj.insert("shell", shell); }
    //PersonaCrypt Options
    if(ui->group_pc_enable->isChecked()){
      if(ui->radio_pc_init->isChecked()){
        //Initialize new device
        obj.insert("personacrypt_init",ui->combo_pc_device->currentData().toString());
        obj.insert("personacrypt_password",ui->line_pc_password->text() );
      }else{
        //import key from file
        QFile keyfile(ui->line_pc_key->text());
        if(keyfile.open(QIODevice::ReadOnly)){
          QByteArray key = keyfile.readAll().toBase64();
          keyfile.close();
           obj.insert("personacrypt_import", QString(key) );
        }
      }
    }else if(ui->group_pc_disable->isChecked()){
      QString password = ui->line_pc_password_disable->text(); //could be empty
      obj.insert("personacrypt_disable", password);
    }
    qDebug() << "Send add_user:" << obj;
    communicate(USERTAG+"add_user", "sysadm", "users",obj);
  }
}

void users_page::send_user_remove(){
  if(ui->list_users->currentItem()==0){ return; } //nothing selected
  QString user = ui->list_users->currentItem()->whatsThis();
  QJsonObject obj;
    obj.insert("action","userdelete");
    obj.insert("name",user);
    if( QMessageBox::Yes == QMessageBox::question(this, tr("Clean Home Directory?"), tr("Also remove the home directory for this user?")+"\n\n"+user, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) ){
      obj.insert("clean_home","true");
    }else{
      obj.insert("clean_home","false");
    }
  communicate(USERTAG+"remove_user", "sysadm", "users",obj);
}

void users_page::send_update_pcdevs(){
  QJsonObject obj;
    obj.insert("action","personacrypt_listdevs");
  communicate(USERTAG+"list_pcdevs", "sysadm", "users",obj);
}

//GROUP API calls
void users_page::send_list_groups(){
QJsonObject obj;
    obj.insert("action","groupshow");
  communicate(USERTAG+"list_groups", "sysadm", "users",obj);
}

void users_page::send_group_save(){
  if(ui->list_groups->currentItem()==0){
    //New Group

  }else{
    //Modify Existing Group
    QString name = ui->list_groups->currentItem()->whatsThis(); //currently-selected group
    QStringList users;
    for(int i=0; i<ui->list_group_members->count(); i++){
      users << ui->list_group_members->item(i)->text();
    }
    qDebug() << "Modify Users in Group:" << name << users;
    QJsonObject obj;
    obj.insert("action","groupmod");
    obj.insert("name",name);
    if(users.isEmpty()){
      //Need to clear all the current users
      obj.insert("remove_users", groupObj.value(name).toObject().value("users") );
    }else{
      //Need to change the list of users over to this new list
      obj.insert("users", QJsonArray::fromStringList(users));
    }
    communicate(USERTAG+"modify_group", "sysadm","users", obj);
  }
}

void users_page::send_group_remove(){

}

//USER Button routines
void users_page::on_push_user_new_clicked(){
  ui->list_users->setCurrentItem(0); //clear the current item
}

void users_page::on_push_user_remove_clicked(){
  //Prompt for verification about the removal
  if(ui->list_users->currentItem()==0){ return; } //nothing selected
  QString user = ui->list_users->currentItem()->whatsThis();
  if( QMessageBox::Yes == QMessageBox::question(this, tr("Remove User?"), tr("Are you sure you wish to remove this user account?")+"\n\n"+user, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
    send_user_remove();
  }
}

void users_page::on_push_user_save_clicked(){
  send_user_save();
}

void users_page::on_tool_pc_findkey_clicked(){
  QString keyfile = QFileDialog::getOpenFileName(this, tr("Select PersonaCrypt Key File"), QDir::homePath(), tr("PersonaCrypt Key (*)") );
  if(keyfile.isEmpty()){ return; }
  ui->line_pc_key->setText(keyfile);
  validateUserChanges();
}
 // GROUP buttons
void users_page::on_push_group_rmuser_clicked(){
  QList<QListWidgetItem*> sel = ui->list_group_members->selectedItems();
  for(int i=0; i<sel.length(); i++){ delete sel[i]; }
  validateGroupSettings();
}

void users_page::on_push_group_adduser_clicked(){
  //Note: Never change the items in the list_group_users widget - those are auto-populated as needed
  QList<QListWidgetItem*> sel = ui->list_group_users->selectedItems();
  for(int i=0; i<sel.length(); i++){ 
    //Make sure this is not a duplicate
    if(ui->list_group_members->findItems(sel[i]->text(), Qt::MatchExactly).isEmpty()){
      ui->list_group_members->addItem(sel[i]->text());
    }
  }
  //Now re-sort the items
  ui->list_group_members->sortItems(Qt::AscendingOrder);
  validateGroupSettings();
}

void users_page::on_push_group_new_clicked(){

}

void users_page::on_push_group_remove_clicked(){

}

void users_page::on_push_group_save_clicked(){
  send_group_save();
}
