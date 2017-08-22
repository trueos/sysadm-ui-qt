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
#include <iterator>
#include <istream>
#include <iostream>
#include <string>
#include <QDebug>
#include "session.h"
#include <unistd.h>

//Initialize the global variables 
QSettings *settings = new QSettings("SysAdm","sysadm-cli", 0);
QSslConfiguration SSL_cfg, SSL_cfg_bridge;

int main( int argc, char ** argv )
{
  QCoreApplication A(argc, argv);
  int ret = 0;
  QStringList args = A.arguments();
  QString name, namesp, jsonArgs, fileName, hostIP="127.0.0.1", user, pass, id;
  bool fullJSON=false, stdJSON=false;
  
  if(args.size() > 1) {
    for(int i = 1; i < args.size(); i+=2) {
      QString arg = args.at(i);

      if(i+1 != args.size()){
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
          i--;
        }else if(arg == "-id"){
          id = nextArg;
        }else if(arg == "-ip"){
          hostIP = nextArg;
        }else if(arg == "-u"){
          user = nextArg;
        }else if(arg == "-p"){
          pass = nextArg;
        }
      }
    }
  }

  if(user.isEmpty())
    user=QString(getlogin());
  if(hostIP != "127.0.0.1" && pass.isEmpty()){
    qDebug() << "Password required for non localhost IP";
    QTextStream in(stdin);
    pass = in.readLine();
  }

  QJsonArray jsonRequests;
  if(fullJSON) {
    QFile *file = new QFile(fileName);
    file->open(QIODevice::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(file->readAll());
    if(doc.isArray())
      jsonRequests = doc.array();
    else if(doc.isObject())
      jsonRequests.append(doc.object());
    else
      qDebug() << "Invalid JSON Request";
    file->close();
    delete file;
  }else if(stdJSON) {
    QTextStream in(stdin);
    QString line;
    while(!in.atEnd())
      line += in.readLine();
    QJsonDocument doc = QJsonDocument::fromJson(line.toLatin1());
    if(doc.isArray())
      jsonRequests = doc.array();
    else if(doc.isObject())
      jsonRequests.append(doc.object());
    else
      qDebug() << "Invalid JSON Request";
  }else{
    QJsonObject obj;
    obj.insert("name", name);
    obj.insert("namespace", namesp);
    obj.insert("id", id);
    obj.insert("args", QJsonDocument::fromJson(jsonArgs.toLocal8Bit()).object());
    jsonRequests.append(obj);
  }

  QStringList sessionArgs;
  sessionArgs << hostIP << user << pass;
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
