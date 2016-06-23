//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "TrayUI.h"

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

QHash<QString,sysadm_client*> CORES; // hostIP / core
QHash<QString, HostMessage> MESSAGES; // "hostIP/message_type", Message Structure

// === PUBLIC ===
sysadm_tray::sysadm_tray() : QSystemTrayIcon(){
  CMAN = 0; SDLG = 0;
  showNotices = false;
  iconreset = true;
  cPriority = 0;
  iconTimer = new QTimer(this);
    iconTimer->setInterval(1500); //1.5 seconds
    connect(iconTimer, SIGNAL(timeout()), this, SLOT(UpdateIcon()) );
  //Load any CORES
  updateCoreList();
  
  //Setup the tray icon
  this->setIcon( QIcon(":/icons/grey/lock.svg") );
  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivated()) );
  //Setup the message menu
  msgMenu = new QMenu(); 
    msgMenu->setIcon( QIcon(":/icons/black/inbox.svg") );
    QAction *act = msgMenu->addAction(QIcon(":/icons/black/trash.svg"), tr("Hide all messages"));
      act->setWhatsThis("clearall");
      QFont fnt = act->font(); fnt.setItalic(true);
      act->setFont(fnt);
    msgMenu->addSeparator();
    connect(msgMenu, SIGNAL(triggered(QAction*)), this, SLOT(MessageTriggered(QAction*)) );
	
  //Setup the menu
  menu = new MenuItem(0,"",msgMenu);
  this->setContextMenu(menu);
  connect(menu, SIGNAL(OpenConnectionManager()), this, SLOT(OpenConnectionManager()) );
  connect(menu, SIGNAL(OpenSettings()), this, SLOT(OpenSettings()) );
  connect(menu, SIGNAL(CloseApplication()),this, SLOT(CloseApplication()) );
  connect(menu, SIGNAL(OpenCore(QString)), this, SLOT(OpenCore(QString)) );
  connect(menu, SIGNAL(ShowMessage(HostMessage)), this, SLOT(ShowMessage(HostMessage)) );
  connect(menu, SIGNAL(ClearMessage(QString, QString)), this, SLOT(ClearMessage(QString, QString)) );
  connect(menu, SIGNAL(UnlockConnections()), this, SLOT(UnlockConnections()) );
  connect(menu, SIGNAL(UpdateTrayIcon()), this, SLOT(UpdateIconPriority()) );
  QTimer::singleShot(10, menu, SLOT(UpdateMenu()) );
  QTimer::singleShot(0,this, SLOT(updateMessageMenu()) );
}

sysadm_tray::~sysadm_tray(){
  if(CMAN!=0){ delete CMAN; }
  if(SDLG!=0){ delete SDLG; }
  delete this->contextMenu(); //Note in docs that the tray does not take ownership of this menu
}

// === PRIVATE ===
sysadm_client* sysadm_tray::getCore(QString host){
  //simplification to ensure that core always exists fot the given host
  if(!CORES.contains(host)){ 
    CORES.insert(host, new sysadm_client()); 
    CORES[host]->registerForEvents(sysadm_client::SYSSTATE);
    #ifdef __FreeBSD__
     //Also load the currently-running user for this process and place that into the UI automatically
      //Note: This will only be valid on FreeBSD systems (since the server is only for FreeBSD)
    if(host==LOCALHOST){ CORES[host]->openConnection(getlogin(),"",LOCALHOST); }
    #endif
  }
  return CORES[host];
}

// === PRIVATE SLOTS ===
void sysadm_tray::trayActivated(){
  qDebug() << "tray activated";
  if(this->contextMenu()!=0){
     this->contextMenu()->popup( this->geometry().center());
  }
}

// - Application-wide setting changed
void sysadm_tray::UpdateWindows(){
  //First check for any of the special windows (skip settings window)
  //if(CMAN!=0){ QTimer::singleShot(0, CMAN, SLOT(UpdateWindow()) ); }
  //Now do all the open client windows
  /*for(int i=0; i<CLIENTS.length(); i++){
    QTimer::singleShot(0, CLIENTS[i], SLOT(UpdateUI()) );
  }*/
}

void sysadm_tray::updateCoreList(){
  showNotices = false;
  //First add the localhost to the top of the list (if available)
  if(sysadm_client::localhostAvailable() ){
    getCore(LOCALHOST);
  }
  //Now add any known hosts (including connection status)
  if(!SSL_cfg.isNull()){
    QStringList known = settings->allKeys().filter("Hosts/").filter("/username");
    //syntax: Hosts/<hostIP>/username = <username>
    known.sort(); //sort by name
    //Now add these hosts to the menu
    for(int i=0; i<known.length(); i++){
      QString host = known[i].section("/",1,-1).section("/username",0,0);
      if(!CORES.contains(host)){
          qDebug() << "Connect To Host:" << host;
	  getCore(host);      
	  QString user = settings->value(known[i]).toString();
	  CORES[host]->openConnection(host);
      }
    }
  }
  QTimer::singleShot(1000, this, SLOT(allowPopups()) );
}

void sysadm_tray::ClientClosed(MainUI* client){
  qDebug() << "Client Closed";
  int index = CLIENTS.indexOf(client);
  if(index >=0){ CLIENTS.takeAt(index)->deleteLater(); }
}

//Menu Actions
void sysadm_tray::OpenConnectionManager(){
  if(CMAN==0){ 
    CMAN = new C_Manager(); 
    connect(CMAN, SIGNAL(SettingsChanged()), menu, SLOT(UpdateMenu()) ); 
  }
  CMAN->showNormal();
}

void sysadm_tray::OpenSettings(){
  if(SDLG==0){ 
    SDLG = new SettingsDialog(); 
    connect(SDLG, SIGNAL(updateWindows()), this, SLOT(UpdateWindows()) );
  }
  SDLG->showNormal();	
}

void sysadm_tray::CloseApplication(){
  //perform any cleanup
    // Disconnect any cores
  QStringList cores = CORES.keys();
  for(int i=0; i<cores.length(); i++){
    qDebug() << "Closing Connection:" << CORES[cores[i]]->currentHost();
    CORES[ cores[i] ]->closeConnection();
  }
    // Close any clients
  if(!CLIENTS.isEmpty()){ qDebug() << "Closing open client:" << CLIENTS.length();}
  for(int i=0; i<CLIENTS.length(); i++){
    CLIENTS[i]->close();
  }
    // Delete any cores (should be disconnected by now)
  for(int i=0; i<cores.length(); i++){
    qDebug() << "Deleting Cores...";
    delete CORES.take(cores[i]);
  }
  QCoreApplication::exit(0);	
}

void sysadm_tray::OpenCore(QString host){
  //See if a window for this host is already open and use that
  for(int i=0; i<CLIENTS.length(); i++){
    if(CLIENTS[i]->currentHost()==host){
       if(CLIENTS[i]->currentCore()->isReady()){  CLIENTS[i]->showNormal(); }
      return;
    }
  }
  //Split the host ID into host/bridge if necessary
  QString b_id = host.section("/",1,-1);
  if(!b_id.isEmpty()){ host = host.section("/",0,0); }
  if(getCore(host)->isConnecting()){ return; } //wait - still trying to connect
  else if(!getCore(host)->isReady()){ getCore(host)->openConnection(); return; }
  if(b_id.isEmpty() && getCore(host)->isBridge()){ return; }
  //Open a new window for this host
  sysadm_client *core = getCore(host);
    MainUI *tmp = new MainUI(core,"", b_id);
    if(core->isReady()){  tmp->showNormal(); }
    connect(tmp, SIGNAL(ClientClosed(MainUI*)), this, SLOT(ClientClosed(MainUI*)) );
    CLIENTS << tmp;	
}

void sysadm_tray::UnlockConnections(){
  this->setIcon( QIcon(":/icons/grey/disk2.svg") );
  //Open all the cores
  updateCoreList();  
  //Update the menu
  QTimer::singleShot(0, menu, SLOT(UpdateMenu()) );
  QTimer::singleShot(50, this, SLOT(trayActivated()) );
}

//Popup Notifications
void sysadm_tray::ShowMessage(HostMessage msg){
  bool refreshlist = true;
  //Update the internal database of messages
  if(MESSAGES.contains(msg.host_id+"/"+msg.message_id) ){
    //see if this message is new or not
    HostMessage old = MESSAGES[msg.host_id+"/"+msg.message_id];
    if(old.message==msg.message && old.date_time > msg.date_time){ refreshlist=false; } //same hidden message - don't re-show it
    else{ MESSAGES.insert(msg.host_id+"/"+msg.message_id, msg); }
  }else{
    MESSAGES.insert(msg.host_id+"/"+msg.message_id, msg);
  }
  //Now update the user-viewable menu's
  if(refreshlist){ QTimer::singleShot(0,this, SLOT(updateMessageMenu()) ); }
}

void sysadm_tray::ClearMessage(QString host, QString msg_id){
  if(MESSAGES.contains(host+"/"+msg_id)){
    MESSAGES.remove(host+"/"+msg_id);
    QTimer::singleShot(0,this, SLOT(updateMessageMenu()) );
  }
}

void sysadm_tray::MessageTriggered(QAction *act){
  if(act->whatsThis()=="clearall"){
    QStringList keys = MESSAGES.keys();
    QDateTime cdt = QDateTime::currentDateTime();
    QDateTime delay = cdt.addDays(1);
    for(int i=0; i<keys.length(); i++){
      if(MESSAGES[keys[i]].date_time < cdt){ MESSAGES[keys[i]].date_time = delay; }
    }
    QTimer::singleShot(0,this, SLOT(updateMessageMenu()) );
  }else if(MESSAGES.contains(act->whatsThis())){
    //Open the designated host and hide this message
    QString host = MESSAGES[act->whatsThis()].host_id;
    MESSAGES[act->whatsThis()].date_time = QDateTime::currentDateTime().addDays(1); //hide for one day if unresolved in the meantime
    QTimer::singleShot(0,this, SLOT(updateMessageMenu()) );
    OpenCore(host);
  }
}

//Function to update the messageMenu
void sysadm_tray::updateMessageMenu(){
  QStringList keys = MESSAGES.keys();
  QList<QAction*> acts = msgMenu->actions();
  //First update the existing actions as needed
  int num = 0; //for the final tally of messages which are visible
  QDateTime cdt = QDateTime::currentDateTime();
  for(int i=0; i<acts.length(); i++){
    if(keys.contains(acts[i]->whatsThis()) && (MESSAGES[acts[i]->whatsThis()].date_time < cdt) ){
      acts[i]->setText( MESSAGES[acts[i]->whatsThis()].message );
      acts[i]->setIcon( QIcon(MESSAGES[acts[i]->whatsThis()].iconfile) );
      num++;
      keys.removeAll(acts[i]->whatsThis()); //already handled
    }else if( acts[i]->whatsThis()!="clearall" && !acts[i]->whatsThis().isEmpty() ) {
      msgMenu->removeAction(acts[i]);
    }
  }
  //Now add in any new messages
  for(int i=0; i<keys.length(); i++){
    if(MESSAGES[keys[i]].date_time < cdt){
      QAction *act = msgMenu->addAction( QIcon(MESSAGES[keys[i]].iconfile),MESSAGES[keys[i]].message );
	act->setWhatsThis(keys[i]);
	num++;
    }
  }
  //Now update the main menu title to account for the new number of messages
  QString title = tr("Messages");
  if(num>0){ title.prepend("("+QString::number(num)+") "); }
  msgMenu->setTitle(title);
  msgMenu->setEnabled(num>0);
}

//Icon Updates
void sysadm_tray::UpdateIconPriority(){
  //A core's priority changed - go through and re-update the current max priority
  QStringList cores = CORES.keys();
  cPriority = 0;
  for(int i=0; i<cores.length(); i++){
    if(CORES[ cores[i] ]->statePriority() > cPriority){ cPriority = CORES[ cores[i] ]->statePriority(); }
  }
  //Update the icon right now
  if(iconTimer->isActive()){ iconTimer->stop(); }
  iconreset = false;
  UpdateIcon();
  //Now setup the automatic flashing 
  if(cPriority >2 && cPriority < 9){ iconTimer->start();  }
}

void sysadm_tray::UpdateIcon(){
  QString icon;
  if(iconreset || cPriority <3){
    if(SSL_cfg.isNull()){ icon = ":/icons/grey/lock.svg"; }
    else{ icon = ":/icons/grey/disk2.svg"; }
  }else if(cPriority < 6){  icon = ":/icons/grey/exclamationmark.svg"; }
  else if(cPriority < 9){  icon = ":/icons/grey/warning.svg"; }
  else{  icon = ":/icons/grey/attention.svg"; }
  this->setIcon(QIcon(icon));
  //Reset the icon flag as needed (for next run)
  iconreset = !iconreset;

}
