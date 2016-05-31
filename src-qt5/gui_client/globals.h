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
//#include <QtConcurrent>

// SSL Objects
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslCertificate>

//Network objects
#include <QNetworkAccessManager>
#include <QNetworkReply>

//GUI widgets
#include <QMainWindow>
#include <QDialog>
#include <QShortcut>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QActionGroup>
#include <QAction>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QWidgetAction>
#include <QResizeEvent>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>

//Special GUI classes
#include <QPropertyAnimation>

// OpenSSL includes
//#include <openssl/x509.h>
//#include <openssl/pem.h>
//#include <openssl/pkcs12.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../Core/sysadm-client.h"

//Now define all the global variables
// (those each subsystem might need to access independently)
extern QSettings *settings;
//Unencrypted SSL objects (after loading them by user passphrase)
extern QSslConfiguration SSL_cfg; //Check "isNull()" to see if the user settings have been loaded yet
extern QSslConfiguration SSL_cfg_bridge;

//Global SSL config functions
inline QString SSLFile(){
  if(settings==0){ return ""; } //should never happen: settings gets loaded at the start
  else{
    QDir dir(settings->fileName()); dir.cdUp(); //need the containing dir in an OS-agnostic way
    return dir.absoluteFilePath("sysadm_ssl.pfx12");
  }
}
inline QString SSLBridgeFile(){
  if(settings==0){ return ""; } //should never happen: settings gets loaded at the start
  else{
    QDir dir(settings->fileName()); dir.cdUp(); //need the containing dir in an OS-agnostic way
    return dir.absoluteFilePath("sysadm_ssl_bridge.pfx12");
  }
}

inline bool LoadSSLFile(QString pass){
  bool imported = true;
  for(int i=0; i<2 && imported; i++){
    QFile certFile( (i==0) ? SSLFile() : SSLBridgeFile() );
    if(!certFile.open(QFile::ReadOnly) ){ return false; } //could not open file
    QSslCertificate certificate;
    QSslKey key;
    QList<QSslCertificate> importedCerts;
    //Reset/Load some SSL stuff
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    imported = QSslCertificate::importPkcs12(&certFile, &key, &certificate, &importedCerts, QByteArray::fromStdString(pass.toStdString()));
    certFile.close();
    //If successfully unencrypted, save the SSL structs for use later
    if(imported){
      //First load the system defaults
      QSslConfiguration cfg;// = QSslConfiguration::defaultConfiguration();
      //QList<QSslCertificate> certs = SSL_cfg.caCertificates();
      QList<QSslCertificate> localCerts = cfg.localCertificateChain();
      cfg.setLocalCertificate(certificate); //add the new local certs (main cert)
      localCerts.append(certificate);
      if(!importedCerts.isEmpty()){ localCerts.append(importedCerts); } //any other certs
      //Now save the changes to the global struct
      cfg.setLocalCertificateChain(localCerts);
      cfg.setPrivateKey(key);
      //Now set the global variable(s)
      if(i==0){ SSL_cfg = cfg; }
      else{ SSL_cfg_bridge = cfg; }
    }
  } //end loop over files
  //Reset/Load some SSL stuff
    //OpenSSL_add_all_algorithms();
    //ERR_load_crypto_strings();
  return imported;
}


#define LOCALHOST QString("127.0.0.1")

#endif
