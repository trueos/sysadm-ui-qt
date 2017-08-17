#include "session.h"
#include <QDebug>
#include <QtConcurrent>

#define DEBUG 0

Session::Session(QStringList sessionArgs, QJsonArray jsonRequests, QCoreApplication *app) 
  : A(app), requests(jsonRequests), args(sessionArgs)
{
  S_CORE = new sysadm_client();
}

Session::~Session() {
  S_CORE->closeConnection();
  delete S_CORE;
}

void Session::close() {
  if(replies == requests.size())
    S_CORE->closeConnection();
  A->exit();
}

void Session::receiveReply(QString ID, QString name, QString namesp, QJsonValue args) {
  QJsonObject obj;
  obj.insert("name", name);
  obj.insert("id",ID);
  obj.insert("namespace",namesp);
  obj.insert("args",args);
  QJsonDocument doc(obj);
  qDebug("%s", QString(doc.toJson(QJsonDocument::Indented)).toUtf8().data());
  replies++;
  if(replies == requests.size())
    emit(S_CORE->clientDisconnected());
}

void Session::start() {
  if(DEBUG) qDebug() << "Starting Session";
  S_CORE->openConnection(args[3], args[4], args[2]);
  QEventLoop loop;
  connect(S_CORE, SIGNAL(clientAuthorized()), &loop, SLOT(quit()));
  connect(S_CORE, SIGNAL(clientUnauthorized()), &loop, SLOT(quit()));
  connect(S_CORE, SIGNAL(newReply(QString,QString,QString,QJsonValue)), this, SLOT(receiveReply(QString,QString,QString,QJsonValue)));
  connect(S_CORE, SIGNAL(clientDisconnected()), this, SLOT(close()));
  loop.exec();
  if(DEBUG) qDebug() << "Core ready:" << S_CORE->isReady();
  replies = 0;

  for(int i = 0; i < requests.size(); i++) {
    if(DEBUG) qDebug() << "Name:" << args[0] << "Namespace" << args[1] << "Request" << requests[i];
    S_CORE->communicate("1", args[1], args[0], requests[i]);
  }
}
