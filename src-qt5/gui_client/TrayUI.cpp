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
  CMAN = 0;
	
  //Load any CORES
  updateCoreList();
  
  //Setup the tray icon
  this->setIcon( QIcon(":/icons/grey/disk2.svg") );
  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivated()) );
	
  //Setup the menu
  menu = new MenuItem();
  this->setContextMenu(menu);
  connect(menu, SIGNAL(OpenConnectionManager()), this, SLOT(OpenConnectionManager()) );
  connect(menu, SIGNAL(OpenSettings()), this, SLOT(OpenSettings()) );
  connect(menu, SIGNAL(CloseApplication()),this, SLOT(CloseApplication()) );
  connect(menu, SIGNAL(OpenCore(QString)), this, SLOT(OpenCore(QString)) );
  connect(menu, SIGNAL(OpenCoreLogs(QString)), this, SLOT(OpenCoreLogs(QString)) );
  connect(menu, SIGNAL(ShowMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)), this, SLOT(showMessage(QString, QString, QSystemTrayIcon::MessageIcon, int)) );

  QTimer::singleShot(0, menu, SLOT(UpdateMenu()) );
}

sysadm_tray::~sysadm_tray(){
  if(CMAN!=0){ delete CMAN; }
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
void sysadm_tray::updateCoreList(){
  bool firstrun = (this->contextMenu()==0);
  //First add the localhost to the top of the list (if available)
  if(sysadm_client::localhostAvailable() ){
    getCore(LOCALHOST);
  }
  //Now add any known hosts (including connection status)
  QStringList known = sysadm_client::knownHosts();
    known.sort(); //sort by name
  //Now add these hosts to the menu
  for(int i=0; i<known.length(); i++){
    getCore(known[i].section("::::",1,1));
  }
  
  if(firstrun){
    //Go ahead and run any auto-connection protocols
    
  }
}

void sysadm_tray::ClientClosed(MainUI* client){
  qDebug() << "Client Closed";
  int index = CLIENTS.indexOf(client);
  if(index >=0){ CLIENTS.takeAt(index)->deleteLater(); }
}

//Menu Actions
void sysadm_tray::OpenConnectionManager(){
  if(CMAN==0){ CMAN = new C_Manager(); }
  CMAN->showNormal();
}

void sysadm_tray::OpenSettings(){
	
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
  //Open a new window for this host
  MainUI *tmp = new MainUI(getCore(host));
  tmp->showNormal();
  connect(tmp, SIGNAL(ClientClosed(MainUI*)), this, SLOT(ClientClosed(MainUI*)) );
  CLIENTS << tmp;	
}

void sysadm_tray::OpenCoreLogs(QString host){
	
}
