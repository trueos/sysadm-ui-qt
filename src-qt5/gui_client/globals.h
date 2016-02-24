//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_GLOBAL_HEADERS_H
#define _PCBSD_SYSADM_CLIENT_GLOBAL_HEADERS_H

//Main app classes
#include <QApplication>
#include <QSystemTrayIcon>

//Backend/core classes
#include <QString>
#include <QList>
#include <QThread>
#include <QStringList>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QWidget>
#include <QMenu>
#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>

// SSL Objects
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslCertificate>

//GUI widgets
#include <QMainWindow>
#include <QDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QActionGroup>
#include <QAction>
#include <QInputDialog>
#include <QLineEdit>
#include <QWidgetAction>
#include <QResizeEvent>

#include "../Core/sysadm-client.h"

//Now define all the global variables
// (those each subsystem might need to access independently)
extern QSettings *settings;
//Unencrypted SSL objects (after loading them by user passphrase)
extern QSslConfiguration SSL_cfg; //Check "isNull()" to see if the user settings have been loaded yet

//Global SSL config functions
inline QString SSLFile(){
  if(settings==0){ return ""; } //should never happen: settings gets loaded at the start
  else{
    QDir dir(settings->fileName()); dir.cdUp(); //need the containing dir in an OS-agnostic way
    return dir.absoluteFilePath("sysadm_ssl.pfx12");
  }
}

inline bool LoadSSLFile(QString pass){
  QFile certFile( SSLFile() );
    if(!certFile.open(QFile::ReadOnly) ){ return false; } //could not open file
  QSslCertificate certificate;
  QSslKey key;
  QList<QSslCertificate> importedCerts;

  bool imported = QSslCertificate::importPkcs12(&certFile, &key, &certificate, &importedCerts, QByteArray::fromStdString(pass.toStdString()));
  certFile.close();
  //If successfully unencrypted, save the SSL structs for use later
  if(imported){
    //First load the system defaults
    SSL_cfg = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> certs = SSL_cfg.caCertificates();
    QList<QSslCertificate> localCerts = SSL_cfg.localCertificateChain();
      localCerts.append(certificate); //add the new local certs
      certs.append(importedCerts); //add the new CA certs
    //Now save the changes to the global struct
    SSL_cfg.setLocalCertificateChain(localCerts);
    SSL_cfg.setCaCertificates(certs);
    SSL_cfg.setPrivateKey(key);
  }
  return imported;
}



#define LOCALHOST QString("127.0.0.1")

#endif


