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
  UpdateIcon();
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
  if(CMAN!=0){ CMAN->deleteLater(); }
  if(SDLG!=0){ SDLG->deleteLater(); }
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
    QTimer::singleShot(0, CORES[ cores[i] ], SLOT(closeConnection()));
  }
    // Close any clients
  if(!CLIENTS.isEmpty()){ qDebug() << "Closing open client:" << CLIENTS.length();}
  for(int i=0; i<CLIENTS.length(); i++){
    CLIENTS[i]->deleteLater();
  }
  QApplication::processEvents();
    // Delete any cores (should be disconnected by now)
  for(int i=0; i<cores.length(); i++){
    QApplication::processEvents();
    qDebug() << "Deleting Cores...";
    delete CORES.take(cores[i]);
  }
  QCoreApplication::exit(0);	
}

void sysadm_tray::OpenCore(QString host, QString page){
  //See if a window for this host is already open and use that
  qDebug() << "Open Host Window:" << host;
  for(int i=0; i<CLIENTS.length(); i++){
    if(CLIENTS[i]->currentHost()==host){
       //if(CLIENTS[i]->currentCore()->isReady()){  
         CLIENTS[i]->showNormal();
      //}
      return;
    }
  }
  //Split the host ID into host/bridge if necessary
  QString b_id = host.section("/",1,-1);
  if(!b_id.isEmpty()){ host = host.section("/",0,0); }
  if(getCore(host)->isConnecting()){ return; } //wait - still trying to connect
  else if(!getCore(host)->isReady()){
    if(getCore(host)->needsBaseAuth()){
      //Need to use username/password to re-connect
      QString user = settings->value("Hosts/"+host+"/username").toString(); //Have username already in settings
      QString pass = QInputDialog::getText(0, host + ": "+tr("Password Required"), QString(tr("Password for %1")).arg(user), QLineEdit::Password);
      if(!pass.isEmpty()){ getCore(host)->openConnection(user, pass, host); }
    }else{
      getCore(host)->openConnection();
    }
    return; 
  }
  if(b_id.isEmpty() && getCore(host)->isBridge()){ return; }
  //Open a new window for this host
  sysadm_client *core = getCore(host);
    MainUI *tmp = new MainUI(core, page, b_id);
    if(core->isReady()){  tmp->showNormal(); }
    connect(tmp, SIGNAL(ClientClosed(MainUI*)), this, SLOT(ClientClosed(MainUI*)) );
    CLIENTS << tmp;	
}

void sysadm_tray::UnlockConnections(){
  UpdateIcon();
  //Open all the cores
  updateCoreList();  
  //Update the menu
  QTimer::singleShot(0, menu, SLOT(UpdateMenu()) );
  QTimer::singleShot(50, this, SLOT(trayActivated()) );
}

//Popup Notifications
void sysadm_tray::ShowMessage(HostMessage msg){
  //qDebug() << "Got Show Message";
  bool refreshlist = true;
  //Update the internal database of messages
  if(MESSAGES.contains(msg.host_id+"/"+msg.message_id) ){
    //see if this message is new or not
    HostMessage old = MESSAGES[msg.host_id+"/"+msg.message_id];
    if(old.message==msg.message /*&& old.date_time > msg.date_time*/){ refreshlist=false; } //same hidden message - don't re-show it
    else{ MESSAGES.insert(msg.host_id+"/"+msg.message_id, msg); }
  }else{
    MESSAGES.insert(msg.host_id+"/"+msg.message_id, msg);
  }
  //Now update the user-viewable menu's
  if(refreshlist){ 
    QTimer::singleShot(10,this, SLOT(updateMessageMenu()) ); 
  }
}

void sysadm_tray::ClearMessage(QString host, QString msg_id){
  //qDebug() << "Clear Message:" << host << msg_id;
  if(MESSAGES.contains(host+"/"+msg_id)){
    MESSAGES.remove(host+"/"+msg_id);
    QTimer::singleShot(10,this, SLOT(updateMessageMenu()) );
  }
}

void sysadm_tray::MessageTriggered(QAction *act){
  if(act->whatsThis()=="clearall"){
    QStringList keys = MESSAGES.keys();
    QDateTime cdt = QDateTime::currentDateTime();
    QDateTime delay = cdt.addDays(1);
    //qDebug() << "Clear all messages:" << cdt << " -to-" << delay;
    for(int i=0; i<keys.length(); i++){
      if(MESSAGES[keys[i]].date_time.secsTo(cdt)>1 ){ 
        HostMessage msg = MESSAGES[keys[i]];
          msg.date_time = delay;
        MESSAGES.insert(keys[i],msg);
      }
    }
    QTimer::singleShot(10,this, SLOT(updateMessageMenu()) );
  }else if(MESSAGES.contains(act->whatsThis())){
    //Open the designated host and hide this message
    HostMessage msg = MESSAGES[act->whatsThis()];
      //Lower the priority down to stop the tray notifications
      if(msg.priority>2){ msg.priority=2; }
      //msg.date_time = QDateTime::currentDateTime().addDays(1); //hide for one day if unresolved in the meantime;
    MESSAGES.insert(act->whatsThis(),msg);
    QTimer::singleShot(10,this, SLOT(updateMessageMenu()) );
    if(act->whatsThis().section("/",-1)=="updates"){ OpenCore(msg.host_id, "page_updates"); }
    else{ OpenCore(msg.host_id); }
  }
}

//Function to update the messageMenu
void sysadm_tray::updateMessageMenu(){
  //qDebug() << "Update Message Menu:";
  QStringList keys = MESSAGES.keys();
  QList<QAction*> acts = msgMenu->actions();
  //First update the existing actions as needed
  int num = 0; //for the final tally of messages which are visible
  QDateTime cdt = QDateTime::currentDateTime();
  //qDebug() << "Current DT/keys" << cdt << keys;
  uint cdt_t = cdt.toTime_t();
  for(int i=0; i<acts.length(); i++){
    //qDebug() << " - Check Act:" << acts[i]->whatsThis();
    if(keys.contains(acts[i]->whatsThis()) && (MESSAGES[acts[i]->whatsThis()].date_time.toTime_t() < cdt_t) ){
      //qDebug() << " - Update action" << MESSAGES[acts[i]->whatsThis()].date_time;
      acts[i]->setText( MESSAGES[acts[i]->whatsThis()].message );
      acts[i]->setIcon( QIcon(MESSAGES[acts[i]->whatsThis()].iconfile) );
      num++;
      keys.removeAll(acts[i]->whatsThis()); //already handled
    }else if( acts[i]->whatsThis()!="clearall" && !acts[i]->whatsThis().isEmpty() ) {
      //qDebug() << " - Remove Action";
      msgMenu->removeAction(acts[i]);
    }
  }
  //Now add in any new messages
  for(int i=0; i<keys.length(); i++){
    if(MESSAGES[keys[i]].date_time.secsTo(cdt)>-1){
      //qDebug() << " Add new action:" << keys[i];
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
  UpdateIconPriority();
}

//Icon Updates
void sysadm_tray::UpdateIconPriority(){
  int pri = 0;
  QStringList keys = MESSAGES.keys();
  QDateTime cdt = QDateTime::currentDateTime();
  //qDebug() << "Update Priority:" << cdt;
  for(int i=0; i<keys.length(); i++){
    //qDebug() << "Check Key:" << keys[i] << MESSAGES[keys[i]].priority << MESSAGES[keys[i]].date_time;
    if(MESSAGES[keys[i]].date_time.secsTo(cdt) <-1 ){ continue; } //hidden message - ignore it for priorities
    if(MESSAGES[keys[i]].priority > pri){ pri = MESSAGES[keys[i]].priority; }
  }
  cPriority = pri; //save for use
  //Update the icon right now
  if(iconTimer->isActive()){ iconTimer->stop(); }
  iconreset = false;
  UpdateIcon();
  //Now setup the automatic flashing 
  if(cPriority >2 && cPriority < 9){ iconTimer->start();  }
}

void sysadm_tray::UpdateIcon(){
  //qDebug() << "Update Icon:" << cPriority << QDateTime::currentDateTime();
  QString icon = ":/icons/custom/sysadm_circle.svg";
  if(iconreset || cPriority <3){
    if(SSL_cfg.isNull()){ icon = ":/icons/custom/sysadm_circle_grey.png"; }
  }else if(cPriority < 6){  icon = ":/icons/custom/sysadm_circle_yellow.png"; }
  else if(cPriority < 9){  icon = ":/icons/custom/sysadm_circle_orange.png"; }
  else if(cPriority==9){  icon = ":/icons/custom/sysadm_circle_red.png"; }
  this->setIcon( QIcon(icon) );
  //Reset the icon flag as needed (for next run)
  iconreset = !iconreset;

}
