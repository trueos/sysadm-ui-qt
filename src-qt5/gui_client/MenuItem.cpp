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
CoreAction::CoreAction(sysadm_client*core, QObject *parent, QString bridge_id) : QAction(parent){
   qDebug() << "New CoreAction:" << core->currentHost() << bridge_id;
  //Load the current core settings into the action
  host = core->currentHost();
  b_id = bridge_id;
  if(b_id.isEmpty()){ this->setWhatsThis("core::"+host); }
  else{ this->setWhatsThis(b_id); }
  if(b_id.isEmpty()){ nickname = settings->value("Hosts/"+host,"").toString(); }
  else{ nickname = b_id.section("::{",0,0); } //(hostname+UUID) Only need hostname for visuals
  if(nickname.isEmpty()){
    if( core->isLocalHost() ){ nickname = tr("Local System"); }
    else{ nickname = host; }
    this->setText(nickname);
  }else{ 
    this->setText(nickname); 
    nickname.append(" ("+host+")"); 
  }
  if(b_id.isEmpty()){
    //Update the icon as needed
    if(core->isActive()){ CoreActive(); }
    else if(core->isConnecting()){ CoreConnecting(); }
    else{ CoreClosed(); }
    //Setup the core connections
    connect(core, SIGNAL(clientAuthorized()), this, SLOT(CoreActive()) );
    connect(core, SIGNAL(clientDisconnected()), this, SLOT(CoreClosed()) );
    connect(core, SIGNAL(clientReconnecting()), this, SLOT(CoreConnecting()) );
    connect(core, SIGNAL(NewEvent(sysadm_client::EVENT_TYPE, QJsonValue)), this, SLOT(CoreEvent(sysadm_client::EVENT_TYPE, QJsonValue)) );
    connect(core, SIGNAL(statePriorityChanged(int)), this, SLOT(priorityChanged(int)) );
  }else{
     //Update the icon as needed
    if(core->isActive()){ CoreActive(); }
    else if(core->isConnecting()){ CoreConnecting(); }
    else{ CoreClosed(); }
    //Setup the core connections
    connect(core, SIGNAL(bridgeAuthorized(QString)), this, SLOT(bridgeAuthorized(QString)) );
    connect(core, SIGNAL(bridgeEvent(QString, sysadm_client::EVENT_TYPE, QJsonValue)), this, SLOT(bridgeEvent(QString, sysadm_client::EVENT_TYPE, QJsonValue)) );
    connect(core, SIGNAL(bridgeStatePriorityChanged(QString, int)), this, SLOT(bridgePriorityChanged(QString, int)) );
  }
  if(b_id.isEmpty()){ connect(core, SIGNAL(clientTypeChanged()), this, SLOT(CoreTypeChanged()) ); }
  //if(core->isBridge() && b_id.isEmpty()){ b_id = "bridge_dummy"; } //just used for a short time while the bridge is disconnected
}

CoreAction::~CoreAction(){
  qDebug() << "CoreAction destructor:" << host << b_id;
  //disconnect(this);
}
void CoreAction::CoreClosed(){
  this->setIcon( QIcon(":/icons/grey/disk.svg") );
  this->setToolTip( tr("Connection Closed") );
  this->setEnabled(true);
  emit UpdateTrayIcon(); //let the main tray icon know it needs to update as needed
  emit ShowMessage( createMessage(host, "connection", QString(tr("%1: Lost Connection")).arg(nickname), ":/icons/grey/off.svg") );
}
void CoreAction::CoreConnecting(){
  this->setIcon( QIcon(":/icons/black/sync.svg") );
  this->setToolTip( tr("Trying to connect....") );
  this->setEnabled(false);
}
void CoreAction::CoreActive(){

  this->setIcon( QIcon(":/icons/black/disk.svg") );
  this->setToolTip( tr("Connection Active") );
  this->setEnabled(true);
  emit ShowMessage( createMessage(host, "connection", QString(tr("%1: Connected")).arg(nickname), ":/icons/black/off.svg") );
  if(b_id=="bridge_dummy"){ qDebug() << " - Rebuild Menu"; emit updateParent(this->whatsThis()); }
}

void CoreAction::CoreEvent(sysadm_client::EVENT_TYPE type, QJsonValue val){
  if(type!=sysadm_client::SYSSTATE || !val.isObject()){ return; }
  //Update notices
  if(val.toObject().contains("updating")){
    QString stat = val.toObject().value("updating").toObject().value("status").toString();
    if(stat=="noupdates"){ emit ClearMessage(host,"updates"); }
    else{
      QString msg, icon;
      if(stat=="rebootrequired"){ msg = tr("%1: Reboot required to finish updates"); icon = ":/icons/black/sync-circled.svg"; }
      else if(stat=="updaterunning"){ msg = tr("%1: Updates in progress"); icon = ":/icons/grey/sync.svg"; }
      else if(stat=="updatesavailable"){ msg = tr("%1: Updates available"); icon = ":/icons/black/sync.svg"; }
      if(!msg.isEmpty()){ emit ShowMessage( createMessage(host,"updates", msg.arg(nickname), icon) ); }
   }
  }
  //ZFS notices
  if(val.toObject().contains("zpools")){
    //Scan the pool notices and see if any need attention
    QJsonObject zpool = val.toObject().value("zpools").toObject();
    QStringList pools = zpool.keys();
    int priority = -1;
    for(int i=0; i<pools.length(); i++){
      if(zpool.value(pools[i]).toObject().contains("priority") ){
        int pri = zpool.value(pools[i]).toObject().value("priority").toString().section("-",0,0).simplified().toInt();
        if(pri > priority){ priority = pri; }
      }
    }
    if(priority>2){
      QString msg = tr("%1: zpool needs attention");
      //update the message for known issues
      if(priority==6){ msg = tr("%1: zpool running low on disk space"); }
      else if(priority==9){ msg = tr("%1: zpool degraded - possible hardware issue"); }
      emit ShowMessage( createMessage(host, "zfs", msg.arg(nickname), ":/icons/black/disk.svg") );
    }else{
      emit ClearMessage(host, "zfs");
    }
  }
}

void CoreAction::CoreTypeChanged(){
  emit updateParent(this->whatsThis());
}

void CoreAction::priorityChanged(int priority){
  QString icon;
  if(priority <3){ icon = ":/icons/black/disk.svg"; } //Information - do nothing
  else if(priority < 6){  icon = ":/icons/black/exclamationmark.svg"; } //Warning - change icon
  else if(priority < 9){  icon = ":/icons/black/warning.svg"; } //Critical - change icon and popup message
  else{  icon = ":/icons/black/attention.svg"; } //Urgent - change icon and popup client window 
  this->setIcon(QIcon(icon));
  emit UpdateTrayIcon(); //let the main tray icon know it needs to update as needed
}
//Bridged versions of the normal slots
void CoreAction::bridgeAuthorized(QString ID){
  if(b_id==ID){ CoreActive(); }
}
void CoreAction::bridgeEvent(QString ID, sysadm_client::EVENT_TYPE type, QJsonValue val){
  if(b_id==ID){ CoreEvent(type, val); }
}
void CoreAction::bridgePriorityChanged(QString ID, int priority){
  if(b_id==ID){ priorityChanged(priority); }
}

//=================
//    CORE MENU
//=================
CoreMenu::CoreMenu(sysadm_client* core, QWidget *parent) : QMenu(parent){
  //This is a bridge connection - make a menu of all connections availeable through bridge
  qDebug() << "New CoreMenu:" << core->currentHost();
  Core = core; //save pointer for later
  host = core->currentHost();
  this->setWhatsThis("core::"+host);
  nickname = settings->value("Hosts/"+host,"").toString();
  if(nickname.isEmpty()){
    if( core->isLocalHost() ){ nickname = tr("Local System"); }
    else{ nickname = host; }
    this->setTitle(nickname);
  }else{ 
    this->setTitle(nickname); 
    nickname.append(" ("+host+")"); 
  }
  //Update the icon as needed
  if(core->isActive()){ CoreActive(); }
  else if(core->isConnecting()){ CoreConnecting(); }
  else{ CoreClosed(); }
  //Setup connections
  connect(this, SIGNAL(triggered(QAction*)), this, SLOT(menuTriggered(QAction*)) );
  connect(core, SIGNAL(clientAuthorized()), this, SLOT(CoreActive()) );
  connect(core, SIGNAL(clientDisconnected()), this, SLOT(CoreClosed()) );
  connect(core, SIGNAL(clientReconnecting()), this, SLOT(CoreConnecting()) );
  connect(core, SIGNAL(bridgeConnectionsChanged(QStringList)), this, SLOT(BridgeConnectionsChanged(QStringList)) );
  connect(core, SIGNAL(clientTypeChanged()), this, SLOT(CoreTypeChanged()) );
  //Now add any additional menus as needed
  QTimer::singleShot(100, this, SLOT(BridgeConnectionsChanged()) );
}

CoreMenu::~CoreMenu(){
  qDebug() << "CoreMenu destructor";
  //disconnect(this);
}

void CoreMenu::menuTriggered(QAction *act){
  if(act==0){ return; }
  if(!act->whatsThis().isEmpty()){ emit OpenCore(host+"/"+act->whatsThis()); }
}
void CoreMenu::triggerReconnect(){
  if(CORES.contains(host) && Core!=0){
    qDebug() << "Reconnect to Bridge:" << host;
    Core->openConnection(); //re-use previous setup
  }
}

void CoreMenu::CoreClosed(){
  qDebug() << " - Got Bridge Closed...";
  this->setIcon( QIcon(":/icons/grey/guidepost.svg") );
  this->setToolTip( tr("Connection Closed") );
  BridgeConnectionsChanged(QStringList()); //clear the menu
  emit UpdateTrayIcon(); //let the main tray icon know it needs to update as needed
  emit ShowMessage( createMessage(host, "connection", QString(tr("%1: Lost Connection")).arg(nickname), ":/icons/grey/off.svg") );
}

void CoreMenu::CoreConnecting(){
  qDebug() << " - Got Bridge Connecting...";
  this->setIcon( QIcon(":/icons/black/sync.svg") );
  this->setToolTip( tr("Trying to connect....") );
  this->setEnabled(false);
}

void CoreMenu::CoreActive(){
  qDebug() << " - Got Bridge Connected...";
  this->setIcon( QIcon(":/icons/black/guidepost.svg") );
  this->setToolTip( tr("Connection Active") );
  this->setEnabled(true);
  emit ShowMessage( createMessage(host, "connection", QString(tr("%1: Connected")).arg(nickname), ":/icons/black/off.svg") );
}

void CoreMenu::CoreTypeChanged(){
  emit updateParent(this->whatsThis());
}

void CoreMenu::BridgeConnectionsChanged(QStringList conns){
  qDebug() << " - Got Bridge Connections Changed";
  if(!CORES.contains(host) || Core==0){ return; }
  if(conns.isEmpty() && Core->isActive() ){  conns = Core->bridgeConnections(); }
  this->setEnabled(true);
  conns.sort(); //sort alphabetically
  //Clear all the actions first
  for(int i=0; i<acts.length(); i++){
    this->removeAction(acts[i]); //remove from menu temporarily
    if(!conns.contains(acts[i]->whatsThis()) ){
      emit ShowMessage( createMessage(host, "connection", QString(tr("%1: Lost Connection")).arg(acts[i]->whatsThis()), ":/icons/grey/off.svg") );
      acts.takeAt(i)->deleteLater(); i--;
    }
  }
  //Now go through and add connections as needed (don't use the "acts" list - it is invalid now)
  for(int i=0; i<conns.length(); i++){
    bool found = false;
    for(int j=0; j<acts.length(); j++){
      if(acts[j]->whatsThis()==conns[i]){ found = true; this->addAction(acts[j]); }
    }
    //Need to create a new action
    if(!found){
      CoreAction *act = new CoreAction(Core, this, conns[i]);
      connect(act, SIGNAL(ShowMessage(HostMessage)), this, SIGNAL(ShowMessage(HostMessage)) );
      connect(act, SIGNAL(ClearMessage(QString, QString)), this, SIGNAL(ClearMessage(QString, QString)) );
      connect(act, SIGNAL(UpdateTrayIcon()), this, SIGNAL(UpdateTrayIcon()) );
      acts << act;
      this->addAction(act);
    }
  }
  if(this->isEmpty() && !Core->isActive()){
    this->addAction(QIcon(":/icons/black/sync.svg"), tr("Reconnect Now"), this, SLOT(triggerReconnect()) );
  }
}

//=================
//       MENU ITEM
//=================
MenuItem::MenuItem(QWidget *parent, QString path, QMenu *msgmenu) : QMenu(parent){
  line_pass = 0;
  lineA = 0;
  msgMenu = msgmenu;
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
  connect(menu, SIGNAL(UpdateTrayIcon()), this, SIGNAL(UpdateTrayIcon()) );
  connect(menu, SIGNAL(ShowMessage(HostMessage)), this, SIGNAL(ShowMessage(HostMessage)) );
  connect(menu, SIGNAL(ClearMessage(QString, QString)), this, SIGNAL(ClearMessage(QString, QString)) );
  QTimer::singleShot(0, menu, SLOT(UpdateMenu()) );
}

void MenuItem::addCoreAction(QString host){
  //Find the core associated with the host
  if(!CORES.contains(host)){ return; }
  sysadm_client *core = CORES[host];
  connect(core, SIGNAL(clientTypeChanged()), this, SLOT(UpdateMenu()) );
  if(core->isBridge()){
    CoreMenu *bmen = new CoreMenu(core, this);
    this->addMenu(bmen);
    connect(bmen, SIGNAL(OpenCore(QString)), this, SIGNAL(OpenCore(QString)) );
    connect(bmen, SIGNAL(ShowMessage(HostMessage)), this, SIGNAL(ShowMessage(HostMessage)) );
    connect(bmen, SIGNAL(ClearMessage(QString, QString)), this, SIGNAL(ClearMessage(QString, QString)) );
    connect(bmen, SIGNAL(UpdateTrayIcon()), this, SIGNAL(UpdateTrayIcon()) );
    connect(bmen, SIGNAL(updateParent(QString)), this, SLOT(CoreItemChanged(QString)) );
    coreMenus << bmen;
  }else{
    CoreAction *act = new CoreAction(core, this);
    this->addAction(act);
    connect(act, SIGNAL(ShowMessage(HostMessage)), this, SIGNAL(ShowMessage(HostMessage)) );
    connect(act, SIGNAL(ClearMessage(QString, QString)), this, SIGNAL(ClearMessage(QString, QString)) );
    connect(act, SIGNAL(UpdateTrayIcon()), this, SIGNAL(UpdateTrayIcon()) );
    connect(act, SIGNAL(updateParent(QString)), this, SLOT(CoreItemChanged(QString)) );
    coreActions << act;
  }
}

// === PUBLIC SLOTS ===
void MenuItem::UpdateMenu(){
  //qDebug() << "Update Menu";
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
  //qDebug() << "Update Menu:" << this->whatsThis() << "Has Core:" << !hosts.isEmpty();
  //qDebug() << "  - subdirs:" << subdirs << "hosts:" << hosts;
  //Now go through and update the menu

  //Clean up any obsolete core actions/menus (need special care)
  for(int i=0; i<coreMenus.length(); i++){ 
      //coreMenus[i]->disconnect(); 
      this->removeAction(coreMenus[i]->menuAction()); 
      coreMenus.takeAt(i)->deleteLater(); 
      i--;
  }
  for(int i=0; i<coreActions.length(); i++){     
      //coreActions[i]->disconnect(); 
      this->removeAction(coreActions[i]); 
      coreActions.takeAt(i)->deleteLater(); 
      i--;
  }
  this->clear(); //clear out anything else
	
    //Check for the localhost first
    if(this->whatsThis().isEmpty() ){
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
	addSubMenu( new MenuItem(this, subdirs[i],msgMenu) );
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
	  connect(line_pass, SIGNAL(textEdited(const QString&)), this, SLOT(PasswordTyping()) );
	}
	line_pass->setText("");
	lineA->setDefaultWidget(line_pass);
	this->addAction(lineA);
	this->setDefaultAction(lineA);  
	this->setActiveAction(lineA);
	line_pass->setFocus(); //give this widget keyboard focus by default
      }else{
        QAction *tmp = this->addAction(QIcon(":/icons/black/globe.svg"),tr("Manage Connections"));
        tmp->setWhatsThis("open_conn_mgmt");
      }
      QAction *tmp = this->addAction(QIcon(":/icons/black/preferences.svg"),tr("Settings"));
        tmp->setWhatsThis("open_settings");
      this->addSeparator();
      if(msgMenu!=0){
	//qDebug() << "Inserting MessageMenu";
        this->addMenu(msgMenu);
	this->addSeparator();
      }
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

void MenuItem::CoreItemChanged(QString ID){
  //Find the Core item associated with this ID and recreate it as needed
  qDebug() << "Core Item Changed:" << ID;
  UpdateMenu();
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

void MenuItem::PasswordTyping(){
  this->setActiveAction(lineA);
}
