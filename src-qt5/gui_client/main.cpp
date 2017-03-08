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
QSettings *settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "SysAdm","sysadm-client", 0);
QSslConfiguration SSL_cfg, SSL_cfg_bridge; //null-loaded config objects

int main( int argc, char ** argv )
{
    // see if the user wants the args
    if (argc == 2 && QString(argv[1]) == "-help")
    {
        qDebug() << "Usage: " << QString(argv[0]) << " [-localhost] [-page N]";
        qDebug() << "    -page N start on page N";
        qDebug() << "    -localhost connect to server on localhost";
        return 0;
    }
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
    //Verify that the settings directory exists first - and create it as needed
    QDir sdir(settings->fileName().section("/",0,-2));
      bool check = false;
      if(!sdir.exists()){ 
        check = true;
        sdir.mkpath(sdir.absolutePath());
        qDebug() << "Creating Settings Directory:" << sdir.absolutePath(); 
      }
      if(check){
        //Now see if the settings files exist in the old PC-BSD directory, and move them over to the new dir as needed
        if(QFile::exists(settings->fileName().replace("/SysAdm/", "/PCBSD/") )){
	  QString ndir = sdir.absolutePath();
          sdir.cd(settings->fileName().section("/",0,-3)+"/PCBSD");
          qDebug() << " - Migrating old settings files from:" << sdir.absolutePath();
	  QStringList files = sdir.entryList(QDir::Files | QDir::NoDotAndDotDot);
          for(int i=0; i<files.length(); i++){
            QFile::rename(sdir.absoluteFilePath(files[i]), ndir+"/"+files[i]);
          }
          //Now remove the old directory
          qDebug() << " - Removing old settings directory";
          sdir.rmpath(sdir.absolutePath());
          settings->sync(); //resync with new settings file
        }

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
    if( !sysadm_client::localhostRunning() ){
      QMessageBox::warning(0, QObject::tr("Local Service Not Found"), QObject::tr("The local sysadm service does not appear to be running. Please start it and then try again."));
      return 1;
    }
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
