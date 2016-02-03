//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include <QCoreApplication>
#include <QDebug>

#include "../Core/sysadm-client.h"

#define TEST 1 //just a simple compile/run test - don't actually do anything

//Initialize the global variables 
sysadm_client *S_CORE = new sysadm_client();
QSettings *settings = new QSettings("PCBSD","sysadm-client", 0);

int main( int argc, char ** argv )
{
  //Load the application
  QCoreApplication A(argc, argv);
  //Parse input variables
  int ret = 0;
  if(!TEST){
    //Start the event loop
    ret = A.exec();
  }else{
    qDebug() << "SysAdm test successful";
  }
  //Clean up any global classes before exiting
  S_CORE->closeConnection();
  settings->sync();
  //Now fully-close the global classes
  delete settings;
  delete S_CORE;
  return ret;
}
