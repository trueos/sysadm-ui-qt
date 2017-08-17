#include <QJsonArray>
#include <QCoreApplication>
#include "../Core/sysadm-client.h"

#ifndef SESSION_H
#define SESSION_H

class Session : public QObject {
  Q_OBJECT

  private:
    sysadm_client *S_CORE;
    QJsonArray requests;
    QStringList args;
    int replies;
    QCoreApplication *A;

  public:
    Session(QStringList, QJsonArray, QCoreApplication*);
    ~Session();
    void start();

  public slots:
    void receiveReply(QString, QString, QString, QJsonValue);
    void close();
};
#endif
