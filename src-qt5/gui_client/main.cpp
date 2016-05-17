//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include <QApplication>
#include <QDebug>

#include "globals.h"
#include "TrayUI.h"
#include "mainUI.h"
#include "SettingsDialog.h"

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

//Initialize the global variables (defined in globals.h)
QSettings *settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "PCBSD","sysadm-client", 0);
QSslConfiguration SSL_cfg, SSL_cfg_bridge; //null-loaded config objects

int main( int argc, char ** argv )
{
  //Load the application
  QApplication A(argc, argv);

  //Determine if this is a stand-alone instance of the client for the localhost	
  bool local_only = false;
  QString gotopage = "";
#ifdef __FreeBSD__
  for(int i=1; i<argc; i++){
    if(QString(argv[i])=="-page" && argc>i+1){
      local_only = true;
      gotopage = argv[i+1]; i++;
    }else if(QString(argv[i])=="-localhost"){ 
      local_only = true; 
    }
  }
#endif
  int ret = 0; //return code
  if(!local_only){
  //Wait a bit until a system tray is available
    A.setQuitOnLastWindowClosed(false); //the tray icon is not considered a window
    bool ready = false;
    for(int i=0; i<60 && !ready; i++){
      ready = QSystemTrayIcon::isSystemTrayAvailable();
      if(!ready){
        //Pause for 5 seconds
       A.thread()->usleep(5000000); //don't worry about stopping event handling - nothing running yet
      }
    }
    if(!ready){
      qDebug() << "Could not find any available system tray after 5 minutes: exiting....";
      return 1;
    }
    //Now start up the system tray
    qDebug() << "Loading Settings From:" << settings->fileName();
    qDebug() << " - Encrypted SSL Cert Bundle:" << SSLFile();
    SettingsDialog::InitSettings();
    sysadm_tray *T = new sysadm_tray();
    T->show();
    //Start the event loop
    ret = A.exec();
    
  }else{
    //Open the stand-alone client just for the localhost
    sysadm_client CORE;
      #ifdef __FreeBSD__
      CORE.openConnection(getlogin(),"","127.0.0.1");
	while( CORE.isConnecting() ){
	  QApplication::processEvents();
	}
      #endif
    qDebug() << "Open Localhost window:" << "On Page:" << gotopage;
    MainUI M(&CORE, gotopage);
    M.show();
    ret = A.exec();
    CORE.disconnect();
    CORE.closeConnection();
  }
  
  //qDebug() << "Cleanly Closing Client...";
  //Clean up any global classes before exiting
  settings->sync();
  //Now fully-close the global classes
  settings->deleteLater();
  //qDebug() << "Returning:" << ret;
  return ret;
}
