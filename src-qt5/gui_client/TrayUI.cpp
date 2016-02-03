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

#define LOCALHOST QString("127.0.0.1")

// === PUBLIC ===
sysadm_tray::sysadm_tray() : QSystemTrayIcon(){
	
  //Connect signals/slots
  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivated()) );
  //Setup the tray icon
  this->setIcon( QIcon(":/icons/grey/disk2.svg") );
  this->setContextMenu( new QMenu() );
	
  //Create the open menu
  M_opengui = new QMenu();
    M_opengui->setTitle("Manage System");
    M_opengui->setIcon( QIcon(":/icons/black/disk2.svg") );
    connect(M_opengui, SIGNAL(triggered(QAction*)), this, SLOT(open_gui(QAction*)) );
	
  //Setup the main menu
  this->contextMenu()->addMenu(M_opengui);
  this->contextMenu()->addSeparator();
  this->contextMenu()->addAction(QIcon(":/icons/black/preferences.svg"),tr("Manage Connections"), this, SLOT(open_config()) );
  this->contextMenu()->addSeparator();
  this->contextMenu()->addAction(QIcon(":/icons/black/off.svg"),tr("Close SysAdm Client"), this, SLOT(close_tray()) );
  
  //Now kick off the menu/core loading systems
  QTimer::singleShot(0,this, SLOT(updateCoreList()) );
}

sysadm_tray::~sysadm_tray(){
  delete M_opengui;
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
  bool firstrun = M_opengui->isEmpty();
  M_opengui->clear();
  //First add the localhost to the top of the list (if available)
  if(sysadm_client::localhostAvailable() ){
    QAction* tmp = M_opengui->addAction( tr("Local System") );
      tmp->setWhatsThis(LOCALHOST);
    if(getCore(LOCALHOST)->currentHost()==LOCALHOST){ tmp->setIcon(QIcon(":/icons/black/disk.svg")); }
    else{ tmp->setIcon(QIcon(":/icons/grey/disk.svg")); }
  }
  //Now add any known hosts (including connection status)
  QStringList known = sysadm_client::knownHosts();
    known.sort(); //sort by name
  //Now add these hosts to the menu
  for(int i=0; i<known.length(); i++){
    QAction* tmp = M_opengui->addAction( known[i].section("::::",0,0) );
      tmp->setWhatsThis( known[i].section("::::",1,1) );
    if( getCore(known[i].section("::::",1,1))->currentHost()==known[i].section("::::",1,1)){ tmp->setIcon(QIcon(":/icons/black/disk.svg")); }
    else{ tmp->setIcon(QIcon(":/icons/grey/disk.svg")); }
  }
  
  if(firstrun){
    //Go ahead and run any auto-connection protocols
    
  }
}

void sysadm_tray::ClientClosed(MainUI* client){
  qDebug() << "Client Closed";
  if(CLIENTS.contains(client)){ CLIENTS.removeAll(client); }
}

void sysadm_tray::open_gui(QAction *act){
  QString host = act->whatsThis(); //selected host
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

void sysadm_tray::open_config(){
	
}

void sysadm_tray::close_tray(){
  //perform any cleanup
    // Disconnect any cores
  QStringList cores = CORES.keys();
  for(int i=0; i<cores.length(); i++){
    CORES[ cores[i] ]->closeConnection();
  }
    // Close any clients
  for(int i=0; i<CLIENTS.length(); i++){
    CLIENTS[i]->close();
  }
    // Delete any cores (should be disconnected by now)
  for(int i=0; i<cores.length(); i++){
    delete CORES.take(cores[i]);
  }
  QCoreApplication::exit(0);
}