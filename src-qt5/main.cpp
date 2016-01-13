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

//Initialize the core of the client
sysadm_client *S_CORE = new sysadm_client();

int main( int argc, char ** argv )
{
  //Load the application
  QApplication A(argc, argv);
  //Start up the client UI
  MainUI dlg;
  dlg.show();
  //Start the event loop
  return A.exec();
}
