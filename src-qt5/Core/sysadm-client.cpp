//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "sysadm-client.h"
#include <QSslConfiguration>
#include <QJsonArray>
#include <QProcess>
#include <QFile>
#include <QTimer>
#include <QSettings>
#include <QSslKey>
#include <QSslCertificate>

//SSL Stuff
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#define SERVERPIDFILE QString("/var/run/sysadm.pid")
#define LOCALHOST QString("127.0.0.1")
#define DEBUG 0

extern QSettings *settings;
//Unencrypted SSL objects (after loading them by user passphrase)
extern QSslConfiguration SSL_cfg; //Check "isNull()" to see if the user settings have been loaded yet

// === PUBLIC ===
sysadm_client::sysadm_client(){
  SOCKET = new QWebSocket("sysadm-client", QWebSocketProtocol::VersionLatest, this);
    //SOCKET->setSslConfiguration(QSslConfiguration::defaultConfiguration());
    //SOCKET->ignoreSslErrors();
    //use the new Qt5 connection syntax for compile time checks that connections are valid
    connect(SOCKET, &QWebSocket::connected, this, &sysadm_client::socketConnected);
    connect(SOCKET, &QWebSocket::disconnected, this, &sysadm_client::socketClosed);
    connect(SOCKET, &QWebSocket::textMessageReceived, this, &sysadm_client::socketMessage);
    //Old connect syntax for the "error" signal (special note about issues in the docs)
    connect(SOCKET, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)) );
    connect(SOCKET, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(socketSslErrors(const QList<QSslError>&)) );
  keepActive=false; //not setup yet
}

sysadm_client::~sysadm_client(){
	
}

// Overall Connection functions (start/stop)
void sysadm_client::openConnection(QString user, QString pass, QString hostIP){
  cuser = user; cpass = pass; chost = hostIP;
  //qDebug() << "Client: Setup connection:" << user << pass << hostIP;
  setupSocket();
}

void sysadm_client::openConnection(QString authkey, QString hostIP){
  cauthkey = authkey; chost = hostIP;
  setupSocket();
}

void sysadm_client::openConnection(QString hostIP){
  chost = hostIP;
  setupSocket();
}

void sysadm_client::closeConnection(){
  keepActive = false;
  //de-authorize the current auth token
  if(!cauthkey.isEmpty()){
    cauthkey.clear(); 
    clearAuth();
  }
  //Now close the connection
  SOCKET->close(QWebSocketProtocol::CloseCodeNormal, "sysadm-client closed");
}

QString sysadm_client::currentHost(){
  return chost;	
}

bool sysadm_client::isActive(){
  return ((SOCKET!=0) && SOCKET->isValid());	
}

//Check if the sysadm server is running on the local system
bool sysadm_client::localhostAvailable(){
  #ifdef __FreeBSD__
  if(DEBUG){ qDebug() << "Checking for Local Host:" << SERVERPIDFILE; }
  //Check if the local socket can connect
  if(QFile::exists(SERVERPIDFILE)){
    //int ret = QProcess::execute("pgrep -f \""+SERVERPIDFILE+"\"");
    //return (ret==0 || ret>3);
    return true;
  }
  #endif
  return false;
}

// Register for Event Notifications (no notifications by default)
void sysadm_client::registerForEvents(EVENT_TYPE event, bool receive){
  bool set = events.contains(event);
  if( set && receive){ return; } //already registered
  else if(!set && !receive){ return; } //already unregistered
  else if(set){ events << event; }
  else{ events.removeAll(event); }
  //Since this can be setup before the socket is connected - see if we can send this message right away
  if(SOCKET->isValid()){
    sendEventSubscription(event, receive);
  }
}
	
//Register the custom SSL Certificate with the server
void sysadm_client::registerCustomCert(){
  if(SSL_cfg.isNull() || SOCKET==0 || !SOCKET->isValid()){ return; }
  //Get the custom cert
  QList<QSslCertificate> certs = SSL_cfg.localCertificateChain();
  QString pubkey, email, nickname;
  for(int i=0; i<certs.length(); i++){
    if(certs[i].issuerInfo(QSslCertificate::Organization).contains("SysAdm-client")){
      pubkey = QString(certs[i].publicKey().toPem().toBase64());
      email = certs[i].issuerInfo(QSslCertificate::EmailAddress).join("");
      nickname = certs[i].issuerInfo(QSslCertificate::Organization).join("");
      break;
    }
  }
  if(pubkey.isEmpty()){ return; } //no cert found
  //Now assemble the request JSON
  QJsonObject obj;
    obj.insert("action","register_ssl_cert");
    obj.insert("pub_key", pubkey);
    obj.insert("email",email);
    obj.insert("nickname",nickname);
  this->communicate("sysadm-auto-cert-register","sysadm","settings", obj);
  
}

// Messages which are still pending a response
QStringList sysadm_client::pending(){ return PENDING; } //returns only the "id" for each 

// Fetch a message from the recent cache
QJsonObject sysadm_client::cachedRequest(QString id){
  if(SENT.contains(id)){ return SENT.value(id); }
  else{ return QJsonObject(); }
}

QJsonValue sysadm_client::cachedReply(QString id){
  if(BACK.contains(id)){ return BACK.value(id); }
  else{ return QJsonObject(); }  
}

// === PRIVATE ===
//Functions to do the initial socket setup
void sysadm_client::performAuth(QString user, QString pass){
  //uses cauthkey if empty inputs
  QJsonObject obj;
  obj.insert("namespace","rpc");
  obj.insert("id","sysadm-client-auth-auto");
  bool noauth = false;
  if(user.isEmpty()){
    if(cauthkey.isEmpty()){
      //SSL Authentication (Stage 1)
      obj.insert("name","auth_ssl");
      obj.insert("args","");
    }else{
      //Saved token authentication
      obj.insert("name","auth_token");
      QJsonObject arg;
	arg.insert("token",cauthkey);
      obj.insert("args", arg);
    }
  }else{
    //User/password authentication
    obj.insert("name","auth");
    QJsonObject arg;
      arg.insert("username",user);
      arg.insert("password",pass);
    obj.insert("args", arg);	  
  }
  sendSocketMessage(obj);
  if(noauth){ emit clientUnauthorized(); }
}

void sysadm_client::clearAuth(){
  QJsonObject obj;
  obj.insert("namespace","rpc");
  obj.insert("id","sysadm-client-auth-auto");
  obj.insert("name","auth_clear");
  obj.insert("args","");	
  sendSocketMessage(obj);
  emit clientUnauthorized();
}

//Communication subroutines with the server (block until message comes back)
void sysadm_client::sendEventSubscription(EVENT_TYPE event, bool subscribe){
  QJsonObject obj;
  obj.insert("namespace","events");
  obj.insert("name", subscribe ? "subscribe" : "unsubscribe");
  obj.insert("id", "sysadm-client-event-auto");
  QString arg;
  if(event == DISPATCHER){ arg = "dispatcher"; }
  else if(event == LIFEPRESERVER){ arg = "life-preserver"; }
  obj.insert("args", arg);
  sendSocketMessage(obj);
}

void sysadm_client::sendSocketMessage(QJsonObject msg){
  QJsonDocument doc(msg);
  if(DEBUG){ qDebug() << "Send Socket Message:" << doc.toJson(QJsonDocument::Compact); }
  SOCKET->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

//Simplification functions
QJsonObject sysadm_client::convertServerReply(QString reply){
  QJsonDocument doc = QJsonDocument::fromJson(reply.toUtf8());
  if(doc.isObject()){ return doc.object(); }
  else{ return QJsonObject(); }
}

QString sysadm_client::SSL_Encode_String(QString str){
  //Get the private key
  QByteArray privkey = SSL_cfg.privateKey().toPem();
  
  //Now use this private key to encode the given string
  unsigned char encode[4098] = {};
  RSA *rsa= NULL;
  BIO *keybio = NULL;
  keybio = BIO_new_mem_buf(privkey.data(), -1);
  if(keybio==NULL){ return ""; }
  rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
  if(rsa==NULL){ return ""; }
  int len = RSA_private_encrypt(str.length(), (unsigned char*)(str.toLatin1().data()), encode, rsa, RSA_PKCS1_PADDING);
  if(len <0){ return ""; }
  else{ 
    //Now return this as a base64 encoded string
    QByteArray str_encode( (char*)(encode), len);
    /*qDebug() << "Encoded String Info";
    qDebug() << " - Raw string:" << str << "Length:" << str.length();
    qDebug() << " - Encoded string:" << str_encode << "Length:" << str_encode.length();*/
    str_encode = str_encode.toBase64();
    /*qDebug() << " - Enc string (base64):" << str_encode << "Length:" << str_encode.length();
    qDebug() << " - Enc string (QString):" << QString(str_encode);*/
    return QString( str_encode ); 
  }

}
// === PUBLIC SLOTS ===
// Communications with server (send message, get response via signal later)
void sysadm_client::communicate(QString ID, QString namesp, QString name, QJsonValue args){
  //Overloaded function for a request which needs assembly
  QJsonObject obj;
  obj.insert("namespace",namesp);
  obj.insert("name", name);
  obj.insert("id", ID);
  obj.insert("args", args);
  //qDebug() << "Send Message:" << QJsonDocument(obj).toJson();
  communicate(QList<QJsonObject>() << obj);
}

void sysadm_client::communicate(QJsonObject request){
  //Overloaded function for a single JSON request
  communicate(QList<QJsonObject>() << request);
}

void sysadm_client::communicate(QList<QJsonObject> requests){
  for(int i=0; i<requests.length(); i++){
    QString ID = requests[i].value("id").toString();
    if(ID.isEmpty()){ 
      qDebug() << "Malformed JSON request:" << requests[i]; 
      continue; 
    }
    //Save this into the cache
    SENT.insert(ID, requests[i]);
    if(BACK.contains(ID)){ BACK.remove(ID); }
    PENDING << ID;
    //Now send off the message
    if(SOCKET->isValid()){ sendSocketMessage(requests[i]); }
  }  
}
	
// === PRIVATE SLOTS ===
void sysadm_client::setupSocket(){
  //qDebug() << "Setup Socket:" << SOCKET->isValid();
  if(SOCKET->isValid()){ return; }
  //Setup the SSL config as needed
  SOCKET->setSslConfiguration(QSslConfiguration::defaultConfiguration());
  //uses chost for setup
  // - assemble the host URL
  if(chost.contains("://")){ chost = chost.section("://",1,1); } //Chop off the custom http/ftp/other header (always need "wss://")
  QString url = "wss://"+chost;
  bool hasport = false;
  url.section(":",-1).toInt(&hasport); //check if the last piece of the url is a valid number
  //Could add a check for a valid port number as well - but that is a bit overkill right now
  if(!hasport){ url.append(":"+QString::number(WSPORTDEFAULT)); }
  qDebug() << " Open WebSocket:  URL:" << url;
  QTimer::singleShot(0,SOCKET, SLOT(ignoreSslErrors()) );
  SOCKET->open(QUrl(url));
}

//Socket signal/slot connections
void sysadm_client::socketConnected(){ //Signal: connected()
  keepActive = true; //got a valid connection - try to keep this open automatically unless the user closes it
  emit clientConnected();
  performAuth(cuser, cpass);
  cpass.clear(); //just to ensure no trace left in memory
  //Ensure SSL connection to non-localhost (only user needed for localhost)
  if(chost!=LOCALHOST && !chost.startsWith(LOCALHOST+":") ){ cuser.clear(); }
}

void sysadm_client::socketClosed(){ //Signal: disconnected()
  qDebug() << " - Connection Closed:" << chost;
  if(keepActive){ 
    //Socket closed due to timeout/server
    // Go ahead and re-open it in one minute if possible with the last-used settings/auth
    QTimer::singleShot(60000, this, SLOT(setupSocket()) );
  }
  emit clientDisconnected();
  //Server cache is now invalid - completely lost connection
  SENT.clear(); BACK.clear(); PENDING.clear(); 
}

void sysadm_client::socketSslErrors(const QList<QSslError>&errlist){ //Signal: sslErrors()
  qWarning() << "SSL Errors Detected:" << errlist.length();
  QList<QSslError> ignored;
  for(int i=0; i< errlist.length(); i++){
    if(errlist[i].error()==QSslError::SelfSignedCertificate || errlist[i].error()==QSslError::HostNameMismatch ){
      qDebug() << " - (IGNORED) " << errlist[i].errorString();
      ignored << errlist[i];
    }else{
      qWarning() << " - " << errlist[i].errorString();
    }
  }
  if(ignored.length() != errlist.length()){
    SOCKET->close(); //SSL errors - close the connection
  }
}

void sysadm_client::socketError(QAbstractSocket::SocketError err){ //Signal:: error()
  qWarning() << "Socket Error detected:" << err;
  if(err==QAbstractSocket::SslHandshakeFailedError){qWarning() << " - SSL Handshake Failed"; }
  qWarning() << " - Final websocket error:" << SOCKET->errorString();
}
//void sysadm_client::socketProxyAuthRequired(const QNetworkProxy &proxy, QAuthenticator *auth); //Signal: proxyAuthenticationRequired()

// - Main message input parsing
void sysadm_client::socketMessage(QString msg){ //Signal: textMessageReceived()
  if(DEBUG){ qDebug() << "New Reply From Server:" << msg; }
  //Convert this into a JSON object
  QJsonObject obj = convertServerReply(msg);
  QString ID = obj.value("id").toString();
  QString namesp = obj.value("namespace").toString();
  QString name = obj.value("name").toString();
  //Still need to add some parsing to the return message
  if(ID=="sysadm-client-auth-auto"){
    //Reply to automated auth system
    if(name=="error"){
      closeConnection();
      emit clientUnauthorized();
    }else{
      QJsonValue args = obj.value("args");
      if(args.isArray()){ 
	      cauthkey = args.toArray().first().toString();
	      emit clientAuthorized();
	      //Now automatically re-subscribe to events as needed
	      for(int i=0; i<events.length(); i++){ sendEventSubscription(events[i]); }
      }else if(args.isObject()){
        //SSL Auth Stage 2
        QString randomkey = args.toObject().value("test_string").toString();
        if(!randomkey.isEmpty()){
          QJsonObject obj;
          obj.insert("name","auth_ssl");
          obj.insert("namespace","rpc");
          obj.insert("id",ID); //re-use this special ID
          QJsonObject args;
          args.insert("encrypted_string", SSL_Encode_String(randomkey));
          obj.insert("args",args);
          this->communicate(obj);
        }
      }
    }
  }else if(ID=="sysadm-client-event-auto"){
    //Reply to automated event subscription - don't need to save this
  }else if(namesp=="events"){
    //Event notification - not tied to any particular request
    if(name=="dispatcher"){ emit NewEvent(DISPATCHER, obj.value("args")); }
    else if(name=="life-preserver"){ emit NewEvent(LIFEPRESERVER, obj.value("args")); }
    else if(name=="system-state"){ emit NewEvent(SYSSTATE, obj.value("args")); }
  }else{
    //Now save this message into the cache for use later (if not an auth reply)
    if(!ID.isEmpty()){ 
      PENDING.removeAll(ID);
      BACK.insert(ID, obj);
    }
    emit newReply(ID, name, namesp, obj.value("args"));
  }
}
	