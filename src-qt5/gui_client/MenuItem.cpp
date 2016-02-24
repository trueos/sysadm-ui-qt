//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "MenuItem.h"

extern QHash<QString,sysadm_client*> CORES; // hostIP / core

//=================
//      CORE ACTION
//=================
CoreAction::CoreAction(sysadm_client*core, QObject *parent) : QAction(parent){
 
  //Load the current core settings into the action
  this->setWhatsThis("core::"+core->currentHost());
  nickname = settings->value("Hosts/"+core->currentHost(),"").toString();
  if(nickname.isEmpty()){
    if( core->isLocalHost() ){ nickname = tr("Local System"); }
    else{ nickname = core->currentHost(); }
    this->setText(nickname);
  }else{ 
    this->setText(nickname); 
    nickname.append(" ("+core->currentHost()+")"); 
  }
  
  //Update the icon as needed
  if(core->isActive()){ CoreActive(); }
  else{ CoreClosed(); }
  //Setup the core connections
  connect(core, SIGNAL(clientAuthorized()), this, SLOT(CoreActive()) );
  connect(core, SIGNAL(clientDisconnected()), this, SLOT(CoreClosed()) );
  connect(core, SIGNAL(NewEvent(sysadm_client::EVENT_TYPE, QJsonValue)), this, SLOT(CoreEvent(sysadm_client::EVENT_TYPE, QJsonValue)) );
}
CoreAction::~CoreAction(){
	
}
void CoreAction::CoreClosed(){
  this->setIcon( QIcon(":/icons/grey/disk.svg") );
  this->setToolTip( tr("Connection Closed") );
  emit ShowMessage(tr("Disconnected"), QString(tr("%1: Lost Connection")).arg(nickname), QSystemTrayIcon::Warning, 1500);
}
void CoreAction::CoreActive(){
  this->setIcon( QIcon(":/icons/black/disk.svg") );
  this->setToolTip( tr("Connection Active") );
  emit ShowMessage(tr("Connected"), QString(tr("%1: Connected")).arg(nickname), QSystemTrayIcon::Information, 1500);	
}
void CoreAction::CoreEvent(sysadm_client::EVENT_TYPE type, QJsonValue data){
	
}

//=================
//       MENU ITEM
//=================
MenuItem::MenuItem(QWidget *parent, QString path) : QMenu(parent){
  line_pass = 0;
  lineA = 0;
  this->setWhatsThis(path);
  this->setTitle(path.section("/",-1));
  //Now setup connections
  connect(this, SIGNAL(triggered(QAction*)), this, SLOT(menuTriggered(QAction*)) );
	
}

MenuItem::~MenuItem(){
  lineA->deleteLater();
}

// === PRIVATE ===
void MenuItem::addSubMenu(MenuItem *menu){
  //Add the submenu to this one
  this->addMenu(menu);
  //setup all the signal forwarding
  connect(menu, SIGNAL(OpenConnectionManager()), this, SIGNAL(OpenConnectionManager()) );
  connect(menu, SIGNAL(OpenSettings()), this, SIGNAL(OpenSettings()) );
  connect(menu, SIGNAL(CloseApplication()),this, SIGNAL(CloseApplication()) );
  connect(menu, SIGNAL(OpenCore(QString)), this, SIGNAL(OpenCore(QString)) );
  connect(menu, SIGNAL(ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)), this, SIGNAL(ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)) );
  QTimer::singleShot(0, menu, SLOT(UpdateMenu()) );
}

void MenuItem::addCoreAction(QString host){
  //Find the core associated with the host
  if(!CORES.contains(host)){ return; }
  CoreAction *act = new CoreAction(CORES[host], this);
  this->addAction(act);
  connect(act, SIGNAL(ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)), this, SIGNAL(ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)) );
}

// === PUBLIC SLOTS ===
void MenuItem::UpdateMenu(){
  QString pathkey = this->whatsThis();
  if(!pathkey.startsWith("C_Groups/") && !pathkey.isEmpty()){pathkey.prepend("C_Groups/"); }
  else if(pathkey.isEmpty()){ pathkey = "C_Groups"; }
  QStringList subdirs = settings->allKeys().filter(pathkey);
    subdirs.removeAll(pathkey); //don't allow a duplicate of this menu
    if(!pathkey.endsWith("/")){ pathkey.append("/"); } //for consistency later
    for(int i=0; i<subdirs.length(); i++){
      //Remove any non-direct children dirs
      if(subdirs[i].section(pathkey,0,0,QString::SectionSkipEmpty).contains("/")){ 
	subdirs.removeAt(i); i--; 
      }
    }
  QStringList hosts = settings->value(pathkey).toStringList();
  //qDebug() << "Update Menu:" << this->whatsThis() << "Has Core:" << !host.isEmpty();
  //qDebug() << "  - subdirs:" << subdirs << "hosts:" << hosts;
  //Now go through and update the menu
  this->clear();
	
    //Check for the localhost first
    if(this->whatsThis().isEmpty()){
      addCoreAction(LOCALHOST); //will only add if the localhost is available
    }
    
    //Now add any other direct hosts
    if(!hosts.isEmpty() && !SSL_cfg.isNull() ){
      if(!this->isEmpty()){ this->addSeparator(); }
      for(int i=0; i<hosts.length(); i++){
	addCoreAction(hosts[i]);
      }
    }
    //Now add any other sub-groups
    if(!subdirs.isEmpty() && !SSL_cfg.isNull() ){
      if(!this->isEmpty()){ this->addSeparator(); }
      for(int i=0; i<subdirs.length(); i++){
	//skip any non-direct child subdirs
        if(subdirs[i].section(this->whatsThis(), 1,1, QString::SectionSkipEmpty).contains("/")){ continue; }
	//Load the subdir
	addSubMenu( new MenuItem(this, subdirs[i]) );
      }
    }
    
    //Now add any more top-level items
    if(this->whatsThis().isEmpty()){
      //top-level menu - add the main tray options at the bottom
      if(!this->isEmpty()){ this->addSeparator(); }
      if(SSL_cfg.isNull() && QFile::exists(SSLFile()) ){
	if(lineA==0){ 
	  lineA = new QWidgetAction(this); 
	  lineA->setWhatsThis("password entry");
	}
	if(line_pass==0){
	  line_pass = new QLineEdit(this);
	  line_pass->setEchoMode(QLineEdit::Password);
	  line_pass->setPlaceholderText( tr("Unlock Connections") );
	  connect(line_pass, SIGNAL(editingFinished()), this, SLOT(PasswordReady()) );
	}
	line_pass->setText("");
	lineA->setDefaultWidget(line_pass);
	this->addAction(lineA);
	line_pass->setFocus();
      }else{
        QAction *tmp = this->addAction(QIcon(":/icons/black/globe.svg"),tr("Manage Connections"));
        tmp->setWhatsThis("open_conn_mgmt");
      }
      QAction *tmp = this->addAction(QIcon(":/icons/black/preferences.svg"),tr("Settings"));
        tmp->setWhatsThis("open_settings");
      this->addSeparator();
      tmp = this->addAction(QIcon(":/icons/black/off.svg"),tr("Close SysAdm Client"));
        tmp->setWhatsThis("close_app");
    }
    
}


// === PRIVATE SLOTS ===
void MenuItem::menuTriggered(QAction *act){
  //Don't handle non-child actions
  if(act->parent()!=this){ return; }
  //Now emit the proper signal for this button
  QString action = act->whatsThis();
  if(action.startsWith("core::")){ emit OpenCore(action.section("core::",0,-1,QString::SectionSkipEmpty)); }
  else if(action=="open_conn_mgmt"){ emit OpenConnectionManager(); }
  else if(action=="open_settings"){ emit OpenSettings(); }
  else if(action=="close_app"){ emit CloseApplication(); }
  else if(action=="unlock_conns"){ emit UnlockConnections(); }
}

void MenuItem::PasswordReady(){
  if(line_pass==0){ return; }
  QString pass = line_pass->text();
  if(pass.isEmpty()){ return; }
  line_pass->setText("");
  if(LoadSSLFile(pass)){
    this->hide();
    emit UnlockConnections();
  }
}