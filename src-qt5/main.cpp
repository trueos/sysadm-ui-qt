//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include <QApplication>
#include <QDebug>

#include "globals.h"
#include "mainUI.h"

//Initialize the global variables (defined in globals.h)
sysadm_client *S_CORE = new sysadm_client();
QSettings *settings = new QSettings("PCBSD","sysadm-client", 0);

int main( int argc, char ** argv )
{
  //Load the application
  QApplication A(argc, argv);
  //Start up the client UI
  MainUI dlg;
  dlg.show();
  //Start the event loop
  int ret = A.exec();
  //Clean up any global classes before exiting
  S_CORE->closeConnection();
  settings->sync();
  //Now fully-close the global classes
  delete settings;
  delete S_CORE;
  return ret;
}
