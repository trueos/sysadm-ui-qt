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

#define LOCALHOST QString("127.0.0.1")
#define DEBUG 0

//==================================
// Note about connection flow:
//==================================
// 1) Attempt to connect
// 2) On connect, request system to identify itself
// 3) If system identifies as a server or bridge, start authentication
// 4) If auth successful, announce the connection is ready
// For any error in these steps, the connection will be closed automatically.
//==================================

extern QSettings *settings;
//Unencrypted SSL objects (after loading them by user passphrase)
extern QSslConfiguration SSL_cfg, SSL_cfg_bridge; //Check "isNull()" to see if the user settings have been loaded yet

// === PUBLIC ===
sysadm_client::sysadm_client(){
  SOCKET = new QWebSocket("sysadm-client", QWebSocketProtocol::VersionLatest, this);
    //use the new Qt5 connection syntax for compile time checks that connections are valid
    connect(SOCKET, &QWebSocket::connected, this, &sysadm_client::socketConnected);
    connect(SOCKET, &QWebSocket::disconnected, this, &sysadm_client::socketClosed);
    connect(SOCKET, &QWebSocket::textMessageReceived, this, &sysadm_client::socketMessage);
    //Old connect syntax for the "error" signal (special note about issues in the docs)
    connect(SOCKET, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)) );
    connect(SOCKET, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(socketSslErrors(const QList<QSslError>&)) );
  keepActive=SSLsuccess=usedSSL=isbridge=false; //not setup yet
  events << SYSSTATE; //always pre-register for this type of event
  cPriority = -1;
  //Timer for events while possibly attempting a connection for 30s->1minute
  connectTimer = new QTimer(this);
    connectTimer->setInterval(1000); //1 second intervals
    connect(connectTimer, SIGNAL(timeout()), this, SIGNAL(clientReconnecting()) );
  pingTimer = new QTimer(this);
    pingTimer->setInterval(90000); //90 second intervals 
    connect(pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()) );
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
void sysadm_client::openConnection(){
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
  return ( (SOCKET!=0) && SOCKET->isValid() );	
}

bool sysadm_client::isLocalHost(){
  return (chost==LOCALHOST || chost.startsWith(LOCALHOST+":"));
}

bool sysadm_client::needsBaseAuth(){
  return !SSLsuccess;
}

bool sysadm_client::isReady(){
  return pingTimer->isActive();
}

bool sysadm_client::isConnecting(){
  //returns true if it is currently trying to establish a connection
  return connectTimer->isActive();
}

bool sysadm_client::isBridge(){
  return isbridge;
}

QStringList sysadm_client::bridgeConnections(){
  return BRIDGE.keys();
}

//Check if the sysadm server is running on the local system
bool sysadm_client::localhostAvailable(){
  #ifdef __FreeBSD__
    /*QProcess P;
    P.start("sockstat -l46 -P tcp -p "+QString::number(WSPORTDEFAULT) );
    P.waitForFinished();
    if( 0 == P.exitCode() ){
      if( QString(P.readAllStandardOutput()).contains(QString::number(WSPORTDEFAULT)) ){ return true; }
    }*/
    return QFile::exists("/usr/local/bin/sysadm-binary"); //server available
  #endif
  return false;
}

// Register for Event Notifications (no notifications by default)
void sysadm_client::registerForEvents(EVENT_TYPE event, bool receive){
  bool set = events.contains(event);
  //qDebug() << "Register for event:" << event << events << set << receive;
  if( set && receive){ return; } //already registered
  else if(!set && !receive){ return; } //already unregistered
  else if(!set){ events << event; }
  else{ events.removeAll(event); }
  //Since this can be setup before the socket is connected - see if we can send this message right away
  if(SOCKET->isValid()){
    sendEventSubscription(event, receive);
  }
}

int sysadm_client::statePriority(){
  if( isActive() ){ return cPriority; }
  else{ return 0; }  
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
      nickname = certs[i].issuerInfo(QSslCertificate::CommonName).join("");
      break;
    }
  }
  if(pubkey.isEmpty()){ return; } //no cert found
  //Now assemble the request JSON
  SSLsuccess = true; //set the internal flag to use SSL on next attempt
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
  usedSSL = false;
  //bool noauth = false;
  if(user.isEmpty() || isbridge){
    if(cauthkey.isEmpty()){
      //SSL Authentication (Stage 1)
      usedSSL = true;
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
  //if(noauth){ emit clientUnauthorized(); }
}

void sysadm_client::performAuth_bridge(QString bridge_id){
  QJsonObject obj;
  obj.insert("namespace","rpc");
  obj.insert("id","sysadm-client-auth-auto");
  obj.insert("name","auth_ssl");
  obj.insert("args","");
  communicate_bridge(bridge_id, obj);
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

//Communication subroutines with the server
void sysadm_client::sendEventSubscription(EVENT_TYPE event, bool subscribe){
  QString arg;
  if(event == DISPATCHER){ arg = "dispatcher"; }
  else if(event == LIFEPRESERVER){ arg = "life-preserver"; }
  else if(event== SYSSTATE){ arg = "system-state"; }
  //qDebug() << "Send Event Subscription:" << event << arg << subscribe;
  this->communicate("sysadm-client-event-auto","events", subscribe ? "subscribe" : "unsubscribe", arg);
}

void sysadm_client::sendEventSubscription_bridge(QString bridge_id, EVENT_TYPE event, bool subscribe){
  QString arg;
  if(event == DISPATCHER){ arg = "dispatcher"; }
  else if(event == LIFEPRESERVER){ arg = "life-preserver"; }
  else if(event== SYSSTATE){ arg = "system-state"; }
  //qDebug() << "Send Event Subscription:" << event << arg << subscribe;
  this->communicate_bridge(bridge_id, "sysadm-client-event-auto","events", subscribe ? "subscribe" : "unsubscribe", arg);
}

void sysadm_client::sendSocketMessage(QJsonObject msg){
  //Overload: Convert JSON to text for transport
  sendSocketMessage(QJsonDocument(msg).toJson(QJsonDocument::Compact));
}
void sysadm_client::sendSocketMessage(QString msg){
  if(!isActive()){ return; }
  if(DEBUG){ qDebug() << "Send Socket Message:" << msg; }
  SOCKET->sendTextMessage(msg);
}

//Simplification functions
message_in sysadm_client::convertServerReply(QString reply){
  message_in msg;
  if(!reply.startsWith("{")){
    //Bridge routed message
    int index = reply.indexOf("\n");
    msg.from_bridge_id = reply.left(index);
    reply = reply.remove(0,index+1);
  }
  if(!msg.from_bridge_id.isEmpty() && BRIDGE.contains(msg.from_bridge_id) ){
    //encrypted message through bridge - decrypt it
    QString key = BRIDGE[msg.from_bridge_id].enc_key;
    if(!key.isEmpty()){ reply = DecodeString(reply, key); }
  }
  QJsonDocument doc = QJsonDocument::fromJson(reply.toUtf8());
  if(doc.isObject()){ 
    msg.id = doc.object().value("id").toString();
    msg.namesp = doc.object().value("namespace").toString();
    msg.name = doc.object().value("name").toString();
    msg.args = doc.object().value("args");
  }
  return msg; 
}

QString sysadm_client::pubkeyMD5(QSslConfiguration cfg){
  QCryptographicHash chash(QCryptographicHash::Md5);
        chash.addData( cfg.localCertificate().publicKey().toPem() );
   return QString(chash.result()); 
}

QString sysadm_client::SSL_Encode_String(QString str, QSslConfiguration cfg){
  //Get the private key
  QByteArray privkey = cfg.privateKey().toPem();
  
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
QString sysadm_client::EncodeString(QString str, QString key){
  //TO-DO
  return str;
}

QString sysadm_client::DecodeString(QString str, QString key){
  //TO-DO
  return str;
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
    if(SOCKET->isValid()){ 
	sendSocketMessage(requests[i]);
	if(pingTimer->isActive()){ pingTimer->stop(); pingTimer->start(); } //reset the timer for this interval
    }
  }  
}
void sysadm_client::communicate_bridge(QString bridge_host_id, QString ID, QString namesp, QString name, QJsonValue args){
  //Overloaded function for a request which needs assembly
  QJsonObject obj;
  obj.insert("namespace",namesp);
  obj.insert("name", name);
  obj.insert("id", ID);
  obj.insert("args", args);
  //qDebug() << "Send Message:" << QJsonDocument(obj).toJson();
  communicate_bridge(bridge_host_id, QList<QJsonObject>() << obj);
}

void sysadm_client::communicate_bridge(QString bridge_host_id, QJsonObject request){
  //Overloaded function for a single JSON request
  communicate_bridge(bridge_host_id, QList<QJsonObject>() << request);
}

void sysadm_client::communicate_bridge(QString bridge_host_id, QList<QJsonObject> requests){
  if(bridge_host_id.isEmpty()){ communicate(requests); return; } //run the non-bridge version
  if(!BRIDGE.contains(bridge_host_id)){
    qDebug() << "Invalid bridge host:" << bridge_host_id;
    return;
  }
  QString key = BRIDGE[bridge_host_id].enc_key;
  for(int i=0; i<requests.length(); i++){
    QString ID = requests[i].value("id").toString();
    if(ID.isEmpty()){ 
      qDebug() << "Malformed JSON request:" << requests[i]; 
      continue; 
    }

    //Save this into the cache
    SENT.insert(bridge_host_id+"::::"+ID, requests[i]);
    if(BACK.contains(bridge_host_id+"::::"+ID)){ BACK.remove(bridge_host_id+"::::"+ID); }
    PENDING << bridge_host_id+"::::"+ID;
    //Now send off the message
    if(SOCKET->isValid()){ 
        QString enc_msg = EncodeString( QJsonDocument(requests[i]).toJson(QJsonDocument::Compact), key);
	sendSocketMessage(bridge_host_id+"\n"+enc_msg);
	if(pingTimer->isActive()){ pingTimer->stop(); pingTimer->start(); } //reset the timer for this interval
    }
  }
}

// === PRIVATE SLOTS ===
void sysadm_client::setupSocket(){
  //qDebug() << "Setup Socket:" << SOCKET->isValid();
  if(SOCKET->isValid()){ return; }
  //Setup the SSL config as needed
  SOCKET->setSslConfiguration(QSslConfiguration::defaultConfiguration());
  SSLsuccess = false;
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
  connectTimer->start();
}

void sysadm_client::sendPing(){
  communicate("sysadm_client_ping", "rpc","query","");
}

//Socket signal/slot connections
void sysadm_client::socketConnected(){ //Signal: connected()
  if(connectTimer->isActive()){ connectTimer->stop(); }
  //keepActive = true; //got a valid connection - try to keep this open automatically unless the user closes it
  //emit clientConnected();
  communicate("sysadm_client_identify","rpc","identify",""); //ask the other system to identify itself
  //performAuth(cuser, cpass);
  //cpass.clear(); //just to ensure no trace left in memory
  //Ensure SSL connection to non-localhost (only user needed for localhost)
  //if(chost!=LOCALHOST && !chost.startsWith(LOCALHOST+":") ){ cuser.clear(); }
}

void sysadm_client::socketClosed(){ //Signal: disconnected()
  qDebug() << " - Connection Closed:" << chost;
  if(connectTimer->isActive()){ connectTimer->stop(); }
  if(pingTimer->isActive()){ pingTimer->stop(); }
  isbridge = false; //always reset this flag
  BRIDGE.clear();
  if(keepActive){ 
    //Socket closed due to timeout/server
    // Go ahead and re-open it in one minute if possible with the last-used settings/auth
    QTimer::singleShot(60000, this, SLOT(setupSocket()) );
  }
  emit clientDisconnected();
  //Server cache is now invalid - completely lost connection
  cPriority = -1;
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
  message_in msg_in = convertServerReply(msg);
  if(!handleMessageInternally(msg_in)){
    //Now save this message into the cache for use later (if not an auth reply)
    if(!msg_in.id.isEmpty()){ 
      PENDING.removeAll(msg_in.id);
      //BACK.insert(msg_in.id, msg_in);
    }
    if(msg_in.from_bridge_id.isEmpty()){
      emit newReply(msg_in.id, msg_in.name, msg_in.namesp, msg_in.args);
    }else{
      emit bridgeReply(msg_in.from_bridge_id, msg_in.id, msg_in.name, msg_in.namesp, msg_in.args);
    }
  }
}
bool sysadm_client::handleMessageInternally(message_in msg){
  //First check to see if this is something which should be handled internally
  if(msg.name=="response" && !PENDING.contains(msg.id) && msg.id!="sysadm-client-auth-auto"){ return true; } //do nothing - might be an injected/fake response
  if(msg.id == "sysadm-client-event-auto"){ return true; } //do nothing - automated response
  //HANDLE AUTH SYSTEMS
  QJsonObject reply;
  if(msg.id=="sysadm_client_identify"){
    QString type = msg.args.toObject().value("type").toString();
    bool startauth = false;
    if(type=="bridge"){ isbridge = true; startauth = true; }
    else if(type=="server"){ isbridge = false; startauth = true; }
    else{  
      qDebug() << "Unknown system type:" << type <<"\nClosing Connection..."; 
      this->closeConnection();  //unknown type of system - disconnect now
    }
    if(startauth){
      keepActive = true; //got a valid connection - try to keep this open automatically unless the user closes it
      emit clientConnected();
      performAuth(cuser, cpass);
      cpass.clear(); //just to ensure no trace left in memory
      //Ensure SSL connection to non-localhost (only user needed for localhost)
      if(chost!=LOCALHOST && !chost.startsWith(LOCALHOST+":") ){ cuser.clear(); }
    }
  }else if(msg.id=="sysadm-client-auth-auto"){
    //Reply to automated auth system
    if(msg.name=="error"){
      closeConnection();
      emit clientUnauthorized();
    }else{
      if(msg.args.isArray()){ 
	//Successful authorization
	if(msg.from_bridge_id.isEmpty()){
          if(cuser.isEmpty()){ SSLsuccess = true; }
	  cauthkey = msg.args.toArray().first().toString();
	  emit clientAuthorized();
	  pingTimer->start();
	  //Now automatically re-subscribe to events as needed
	  //qDebug() << "Re-subscribe to events:" << events;
	  for(int i=0; i<events.length(); i++){ sendEventSubscription(events[i]); }
        }else{
	  BRIDGE[msg.from_bridge_id].auth_tok = msg.args.toArray().first().toString();
	  emit bridgeAuthorized(msg.from_bridge_id);
	  //Now automatically re-subscribe to events as needed
	  //qDebug() << "Re-subscribe to events:" << events;
	  for(int i=0; i<events.length(); i++){ sendEventSubscription_bridge(msg.from_bridge_id, events[i]); }
       }

      }else if(msg.args.isObject()){
        //SSL Auth Stage 2
        QString randomkey = msg.args.toObject().value("test_string").toString();
        if(!randomkey.isEmpty()){
          reply.insert("name","auth_ssl");
          reply.insert("namespace","rpc");
          QJsonObject args;
	  QString enc_string;
          if(isbridge && msg.from_bridge_id.isEmpty()){ enc_string = SSL_Encode_String(randomkey, SSL_cfg_bridge); }
          else{ enc_string = SSL_Encode_String(randomkey, SSL_cfg); }
          args.insert("encrypted_string", enc_string);
          reply.insert("args",args);
        }
      }else{
        return false; //unhandled
      }
    }

  }else if(msg.namesp=="events"){
    //Event notification - not tied to any particular request
    if(msg.name=="dispatcher"){ 
      if(msg.from_bridge_id.isEmpty()){ emit NewEvent(DISPATCHER, msg.args); }
      else{ emit bridgeEvent(msg.from_bridge_id, DISPATCHER, msg.args); }
    }else if(msg.name=="life-preserver"){ 
      if(msg.from_bridge_id.isEmpty()){ emit NewEvent(LIFEPRESERVER, msg.args); }
      else{ emit bridgeEvent(msg.from_bridge_id, LIFEPRESERVER, msg.args); }
    }else if(msg.name=="system-state"){ 
      QString pri = msg.args.toObject().value("priority").toString();
      int priority = pri.section("-",0,0).simplified().toInt();
      //qDebug() << "Got System State Event:" << priority << "Formerly:" << cPriority;
      if(cPriority!=priority){ 
	cPriority = priority;
	if(msg.from_bridge_id.isEmpty()){ emit statePriorityChanged(cPriority);  }
        else{ emit bridgeStatePriorityChanged(msg.from_bridge_id, cPriority);  }
      }
      if(msg.from_bridge_id.isEmpty()){ emit NewEvent(SYSSTATE, msg.args); }
      else{ emit bridgeEvent(msg.from_bridge_id, SYSSTATE, msg.args); }
    }else if(isbridge && msg.name=="bridge"){
      QJsonArray conns = msg.args.toObject().value("available_connections").toArray();
      QStringList avail;
      for(int i=0; i<conns.count(); i++){
        QString conn = conns[i].toString();
        if(conn.isEmpty()){ continue; }
        avail << conn;
        //Now add the connection to the internal DB if needed
        if(!BRIDGE.contains(conn)){
          BRIDGE.insert(conn, bridge_data());
          performAuth_bridge(conn);
        }
      }
      //Now go through and clean out any old bridge data (from disconnections)
      QStringList curr = BRIDGE.keys();
      for(int i=0; i<curr.length(); i++){
        if(!avail.contains(curr[i])){ BRIDGE.remove(curr[i]); }
      }
      emit bridgeConnectionsChanged(avail);
    }
  }else if(msg.namesp=="rpc" && msg.name=="identify"){
          QJsonObject args;
          args.insert("type", "client");
          reply.insert("args",args);
  }else if(msg.namesp=="rpc" && msg.name=="settings"){
    if(msg.args.isObject() && msg.args.toObject().value("action").toString()=="list_ssl_checksums"){
      QJsonObject args;
        QJsonArray arr;
        arr << pubkeyMD5(SSL_cfg);
        args.insert("md5_keys",arr);
      reply.insert("args",args);
    }else{ return false; }
  }else{
    return false; //not handled internally
  }
  //Send any reply if needed
  if(!reply.isEmpty()){
    if(!reply.contains("id")){ reply.insert("id",msg.id); } //re-use this special ID if necessary
    if(!reply.contains("name")){ reply.insert("name","response"); }
    if(!reply.contains("namespace")){ reply.insert("namespace",msg.namesp); }
    if(msg.from_bridge_id.isEmpty()){ this->communicate(reply); }
    else{ this->communicate_bridge(msg.from_bridge_id, reply); }
  }
  return true;
}
