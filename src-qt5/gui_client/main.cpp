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

//Initialize the global variables (defined in globals.h)
//sysadm_client *S_CORE = new sysadm_client();
QSettings *settings = new QSettings("PCBSD","sysadm-client", 0);

int main( int argc, char ** argv )
{
  //Load the application
  QApplication A(argc, argv);
    A.setQuitOnLastWindowClosed(false); //the tray icon is not considered a window
  //Wait a bit until a system tray is available
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
  int ret = A.exec();
  //Clean up any global classes before exiting
  //S_CORE->closeConnection();
  settings->sync();
  //Now fully-close the global classes
  delete settings;
  //delete S_CORE;
  return ret;
}
