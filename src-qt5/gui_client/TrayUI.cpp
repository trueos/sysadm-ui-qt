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

// === PUBLIC ===
sysadm_tray::sysadm_tray() : QSystemTrayIcon(){
  CMAN = 0; SDLG = 0;
  showNotices = false;
  //Load any CORES
  updateCoreList();
  
  //Setup the tray icon
  this->setIcon( QIcon(":/icons/grey/lock.svg") );
  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivated()) );
	
  //Setup the menu
  menu = new MenuItem();
  this->setContextMenu(menu);
  connect(menu, SIGNAL(OpenConnectionManager()), this, SLOT(OpenConnectionManager()) );
  connect(menu, SIGNAL(OpenSettings()), this, SLOT(OpenSettings()) );
  connect(menu, SIGNAL(CloseApplication()),this, SLOT(CloseApplication()) );
  connect(menu, SIGNAL(OpenCore(QString)), this, SLOT(OpenCore(QString)) );
  connect(menu, SIGNAL(ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)), this, SLOT(ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)) );
  connect(menu, SIGNAL(UnlockConnections()), this, SLOT(UnlockConnections()) );
  QTimer::singleShot(0, menu, SLOT(UpdateMenu()) );
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
    #ifdef __FreeBSD__
     //Also load the currently-running user for this process and place that into the UI automatically
      //Note: This will only be valid on FreeBSD systems (since the server is only for FreeBSD)
    if(host==LOCALHOST){ CORES[host]->openConnection(getlogin(),"",LOCALHOST); }
    #endif
  }
  return CORES[host];
}

// === PRIVATE SLOTS ===
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
      QString host = known[i].section("/",1,500).section("/username",0,0);
      if(!CORES.contains(host)){
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
    if(CLIENTS[i]->currentCore()->currentHost()==host){
      CLIENTS[i]->showNormal();
      return;
    }
  }
  if(getCore(host)->isConnecting()){ return; } //wait - still trying to connect
  //Open a new window for this host
  MainUI *tmp = new MainUI(getCore(host));
  tmp->showNormal();
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
void sysadm_tray::ShowMessage(QString title, QString text, QSystemTrayIcon::MessageIcon icon, int ms){
  if(!showNotices){ return; } //skip this popup
  
  //Default popup notification system for systray icons
  this->showMessage(title, text, icon,ms);
}
