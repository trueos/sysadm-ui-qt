//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "MenuItem.h"

extern QHash<QString,sysadm_client*> CORES; // hostIP / core

MenuItem::MenuItem(QWidget *parent, QString path, sysadm_client *core) : QMenu(parent){
  this->setWhatsThis(path);
  this->setTitle(path.section("/",-1));
  //Now setup connections
  connect(this, SIGNAL(triggered(QAction*)), this, SLOT(menuTriggered(QAction*)) );
  if(core!=0){
    //Load the current core settings
    host = core->currentHost();
    if(core->isActive()){ CoreActive(); }
    else{ CoreClosed(); }
    //Setup the core connections
    connect(core, SIGNAL(clientAuthorized()), this, SLOT(CoreActive()) );
    connect(core, SIGNAL(clientDisconnected()), this, SLOT(CoreClosed()) );
    connect(core, SIGNAL(NewEvent(sysadm_client::EVENT_TYPE, QJsonValue)), this, SLOT(CoreEvent(sysadm_client::EVENT_TYPE, QJsonValue)) );
  }
	
}

MenuItem::~MenuItem(){
	
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
  connect(menu, SIGNAL(OpenCoreLogs(QString)), this, SIGNAL(OpenCoreLogs(QString)) );
  connect(menu, SIGNAL(ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)), this, SIGNAL(ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)) );
  QTimer::singleShot(0, menu, SLOT(UpdateMenu()) );
}

// === PUBLIC SLOTS ===
void MenuItem::UpdateMenu(){
  QStringList subdirs = settings->allKeys().filter("C_Groups/"+(this->whatsThis().isEmpty() ? "" : (this->whatsThis()+"/") ) );
  QStringList hosts = settings->value("C_Groups/"+this->whatsThis()).toStringList();
  qDebug() << "Update Menu:" << this->whatsThis() << "Has Core:" << !host.isEmpty();
  qDebug() << "  - subdirs:" << subdirs << "hosts:" << hosts;
  //Now go through and update the menu
  this->clear();
  if(host.isEmpty()){
    //This is a general menu (not associated with a particular core)
	
    //Check for the localhost first
    if(this->whatsThis().isEmpty() && CORES.contains(LOCALHOST) ){
      addSubMenu( new MenuItem(this, tr("Local System"), CORES[LOCALHOST]) );
    }
    
    //Now add any other direct hosts
    if(!hosts.isEmpty()){
      if(!this->isEmpty()){ this->addSeparator(); }
      for(int i=0; i<hosts.length(); i++){
        if(CORES.contains(hosts[i])){
	  addSubMenu( new MenuItem(this, settings->value("Hosts/"+hosts[i], hosts[i]).toString(), CORES[ hosts[i] ]) );
	}
      }
    }
    //Now add any other sub-groups
    if(!subdirs.isEmpty()){
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
      QAction *tmp = this->addAction(QIcon(":/icons/black/globe.svg"),tr("Manage Connections"));
        tmp->setWhatsThis("open_conn_mgmt");
      tmp = this->addAction(QIcon(":/icons/black/preferences.svg"),tr("Settings"));
        tmp->setWhatsThis("open_settings");
      this->addSeparator();
      tmp = this->addAction(QIcon(":/icons/black/off.svg"),tr("Close SysAdm Client"));
        tmp->setWhatsThis("close_app");
    }
    
  }else{
    //Special Menu: Has an associated core file - only list options for accessing it
    QAction *tmp = this->addAction(QIcon(":/icons/black/desktop.svg"), tr("Manage System"));
	tmp->setWhatsThis("open_host");
    tmp = this->addAction(QIcon(":/icons/black/document-text.svg"), tr("View Logs"));
	tmp->setWhatsThis("open_host_logs");
  }
}


// === PRIVATE SLOTS ===
void MenuItem::menuTriggered(QAction *act){
  //Don't handle non-child actions
  if(act->parent()!=this){ return; }
  //Now emit the proper signal for this button
  QString action = act->whatsThis();
  if(action=="open_conn_mgmt"){ emit OpenConnectionManager(); }
  else if(action=="open_settings"){ emit OpenSettings(); }
  else if(action=="close_app"){ emit CloseApplication(); }
  else if(action=="open_host"){ emit OpenCore(host); }
  else if(action=="open_host_logs"){ emit OpenCoreLogs(host); }
}

void MenuItem::CoreClosed(){
  this->setIcon( QIcon(":/icons/grey/disk.svg") );
  this->setToolTip( tr("Connection Closed") );
}

void MenuItem::CoreActive(){
  this->setIcon( QIcon(":/icons/black/disk.svg") );
  this->setToolTip( tr("Connection Active") );
}

void MenuItem::CoreEvent(sysadm_client::EVENT_TYPE type, QJsonValue data){
	
}