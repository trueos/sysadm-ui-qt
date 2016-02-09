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

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

//Initialize the global variables (defined in globals.h)
//sysadm_client *S_CORE = new sysadm_client();
QSettings *settings = new QSettings("PCBSD","sysadm-client", 0);

int main( int argc, char ** argv )
{
  //Load the application
  QApplication A(argc, argv);
    
  //Determine if this is a stand-alone instance of the client for the localhost	
  bool local_only = false;
  for(int i=1; i<argc; i++){
    #ifdef __FreeBSD__
    if(QString(argv[i])=="-localhost"){ local_only = true; break;}
    #endif
  }
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
    sysadm_tray *T = new sysadm_tray();
    T->show();
    //Start the event loop
    ret = A.exec();
    
  }else{
    //Open the stand-alone client just for the localhost
    sysadm_client CORE;
      #ifdef __FreeBSD__
      CORE.openConnection(getlogin(),"","127.0.0.1");
      #endif
    MainUI M(&CORE);
    M.show();
    ret = A.exec();
    CORE.disconnect();
    CORE.closeConnection();
  }
  
  qDebug() << "Cleanly Closing Client...";
  //Clean up any global classes before exiting
  settings->sync();
  //Now fully-close the global classes
  delete settings;
  return ret;
}
