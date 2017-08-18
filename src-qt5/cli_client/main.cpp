//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include <QCoreApplication>
#include <QFile>
#include <QtConcurrent>
#include <QJsonArray>
#include <QDebug>
#include "session.h"

//Initialize the global variables 
QSettings *settings = new QSettings("SysAdm","sysadm-cli", 0);
QSslConfiguration SSL_cfg, SSL_cfg_bridge;

int main( int argc, char ** argv )
{
  //Load the application
  QCoreApplication A(argc, argv);
  int ret = 0;
  QStringList args = A.arguments();
  QString name, namesp, jsonArgs, fileName, hostIP, user, pass, id;
  bool fullJSON=false, stdJSON=false;
  
  if(args.size() > 1) {
    for(int i = 1; i < args.size(); i++) {
      QString arg = args.at(i);

      if(i != args.size() - 1) {
        QString nextArg = args.at(i+1);
        if(arg == "-ns"){
          namesp = nextArg;
        }else if(arg == "-n"){
          name = nextArg;
        }else if(arg == "-a"){
          jsonArgs = nextArg;
        }else if(arg == "-F"){
          fullJSON = true;
          fileName = nextArg;
        }else if(arg == "-std"){
          stdJSON = true;
        }else if(arg == "-id"){
          id = nextArg;
        }else if(arg == "-ip"){
          hostIP = nextArg;
        }else if(arg == "-u"){
          user = nextArg;
        }else if(arg == "-p"){
          pass = nextArg;
        }
        i++;
      }
    }
  }else{
    //Prompt user if no args given
  }

  QJsonArray jsonRequests;
  if(fullJSON) {
    QFile *file = new QFile(fileName);
    file->open(QIODevice::ReadOnly);
    QJsonObject doc = QJsonDocument::fromJson(file->readAll()).object();
    id = doc["id"].toString();
    name = doc["name"].toString();
    namesp = doc["namespace"].toString();
    jsonRequests.append(doc["args"]);
    file->close();
    delete file;
  }else{
    jsonRequests.append( QJsonDocument::fromJson(jsonArgs.toLocal8Bit()).object() );
  }

  QStringList sessionArgs;
  sessionArgs << name << namesp << hostIP << user << pass << id;
  Session *session = new Session(sessionArgs, jsonRequests, &A);
  session->start();

  //Start the event loop
  ret = A.exec();

  //Clean up any global classes before exiting
  settings->sync();

  //Now fully-close the global classes
  delete session;
  delete settings;
  return ret;
}
