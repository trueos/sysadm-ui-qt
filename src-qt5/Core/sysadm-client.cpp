//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "sysadm-client.h"
#include <QSslConfiguration>
#include <QJsonArray>

// === PUBLIC ===
sysadm_client::sysadm_client(){
  SOCKET = new QWebSocket("sysadm-client", QWebSocketProtocol::VersionLatest, this);
    SOCKET->setSslConfiguration(QSslConfiguration::defaultConfiguration());
    SOCKET->ignoreSslErrors( QList<QSslError>() << QSslError(QSslError::SelfSignedCertificate) );
    //use the new Qt5 connection syntax for compile time checks that connections are valid
    connect(SOCKET, &QWebSocket::connected, this, &sysadm_client::socketConnected);
    connect(SOCKET, &QWebSocket::disconnected, this, &sysadm_client::socketClosed);
    connect(SOCKET, &QWebSocket::textMessageReceived, this, &sysadm_client::socketMessage);
    //Old connect syntax for the "error" signal (special note about issues in the docs)
    connect(SOCKET, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)) );
    connect(SOCKET, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(socketSslErrors(const QList<QSslError>&)) );
  keepActive=false; //not setup yet
  settings = new QSettings("PC-BSD","sysadm-client", this);
}

sysadm_client::~sysadm_client(){
	
}

// Overall Connection functions (start/stop)
void sysadm_client::openConnection(QString user, QString pass, QString hostIP){
  cuser = user; cpass = pass; chost = hostIP;
  qDebug() << "Client: Setup connection:" << user << pass << hostIP;
  setupSocket();
}

void sysadm_client::openConnection(QString authkey, QString hostIP){
  cauthkey = authkey; chost = hostIP;
  setupSocket();
}

void sysadm_client::closeConnection(){
  keepActive = false;
  //de-authorize the current auth token
  cauthkey.clear(); 
  performAuth();
  //Now close the connection
  SOCKET->close(QWebSocketProtocol::CloseCodeNormal, "sysadm-client closed");
}

// Connection Hosts Database Access
QStringList sysadm_client::knownHosts(){
  //Returns: <IP>::::<Name>
  QStringList hosts =settings->value("knownhosts",QStringList()).toStringList();
  for(int i=0; i<hosts.length(); i++){
    hosts[i].append("::::" + settings->value("hostnames/"+hosts[i],"").toString() );
  }
  return hosts;
}

void sysadm_client::saveHost(QString IP, QString name){
  QStringList hosts = settings->value("knownhosts",QStringList()).toStringList();
  hosts << IP;
  hosts.removeDuplicates();
  settings->setValue("knownhosts",hosts);
  settings->setValue("hostnames/"+IP,name);
}

void sysadm_client::removeHost(QString IP){
  QStringList hosts = settings->value("knownhosts",QStringList()).toStringList();
  hosts.removeAll(IP);
  settings->setValue("knownhosts",hosts);
  settings->remove("hostnames/"+IP);
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
void sysadm_client::setupSocket(){
  qDebug() << "Setup Socket:" << SOCKET->isValid();
  if(SOCKET->isValid()){ return; }
  //uses chost for setup
  // - assemble the host URL
  if(chost.contains("://")){ chost = chost.section("://",1,1); } //Chop off the custom http/ftp/other header (always need "wss://")
  QString url = "wss://"+chost;
  bool hasport = false;
  url.section(":",-1).toInt(&hasport); //check if the last piece of the url is a valid number
  //Could add a check for a valid port number as well - but that is a bit overkill right now
  if(!hasport){ url.append(":"+QString::number(WSPORTDEFAULT)); }
  qDebug() << " - URL:" << url;
  SOCKET->open(QUrl(url));
}

void sysadm_client::performAuth(QString user, QString pass){
  //uses cauthkey if empty inputs
  QJsonObject obj;
  obj.insert("namespace","rpc");
  obj.insert("id","sysadm-client-auth-auto");
  bool noauth = false;
  if(user.isEmpty() && pass.isEmpty()){
    if(cauthkey.isEmpty()){
      //Nothing to authenticate - de-auth the connection instead
      obj.insert("name","auth_clear");
      obj.insert("args","");
      noauth = true;
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

//Communication subroutines with the server (block until message comes back)
void sysadm_client::sendEventSubscription(EVENT_TYPE event, bool subscribe){
  QJsonObject obj;
  obj.insert("namespace","events");
  obj.insert("name", subscribe ? "subscribe" : "unsubscribe");
  obj.insert("id", "sysadm-client-event-auto");
  QString arg;
  if(event == DISPATCHER){ arg = "dispatcher"; }
  obj.insert("args", arg);
  sendSocketMessage(obj);
}

void sysadm_client::sendSocketMessage(QJsonObject msg){
  QJsonDocument doc(msg);
  SOCKET->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

//Simplification functions
QJsonObject sysadm_client::convertServerReply(QString reply){
  QJsonDocument doc = QJsonDocument::fromJson(reply.toUtf8());
  if(doc.isObject()){ return doc.object(); }
  else{ return QJsonObject(); }
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
//Socket signal/slot connections
void sysadm_client::socketConnected(){ //Signal: connected()
  keepActive = true; //got a valid connection - try to keep this open automatically
  num_fail = 0; //reset fail counter - got a valid connection
  emit clientConnected();
  performAuth(cuser, cpass);
  cuser.clear(); cpass.clear(); //just to ensure no trace left in memory
}

void sysadm_client::socketClosed(){ //Signal: disconnected()
  if(keepActive && num_fail < FAIL_MAX){ 
    //Socket closed due to timeout? 
    // Go ahead and re-open it if possible with the last-used settings/auth
    num_fail++;
    setupSocket();
  }else{
    num_fail = 0; //reset back to nothing
    emit clientDisconnected();
    //Server cache is now invalid - completely lost connection
    SENT.clear(); BACK.clear(); PENDING.clear(); 
  }	  
}

void sysadm_client::socketSslErrors(const QList<QSslError>&errlist){ //Signal: sslErrors()
  qWarning() << "SSL Errors Detected:" << errlist.length();
  for(int i=0; i< errlist.length(); i++){
    qWarning() << " - " << errlist[i].errorString();
  }
  qWarning() << "Ignoring SSL errors...";
  SOCKET->ignoreSslErrors();
}

void sysadm_client::socketError(QAbstractSocket::SocketError err){ //Signal:: error()
  qWarning() << "Socket Error detected:" << err;
  qWarning() << " - " << SOCKET->errorString();
}
//void sysadm_client::socketProxyAuthRequired(const QNetworkProxy &proxy, QAuthenticator *auth); //Signal: proxyAuthenticationRequired()

// - Main message input parsing
void sysadm_client::socketMessage(QString msg){ //Signal: textMessageReceived()
  qDebug() << "New Reply From Server:" << msg;
  //Convert this into a JSON object
  QJsonObject obj = convertServerReply(msg);
  QString ID = obj.value("id").toString();
  QString namesp = obj.value("namespace").toString();
  QString name = obj.value("name").toString();
  //Still need to add some parsing to the return message
  if(ID=="sysadm-client-auth-auto"){
    //Reply to automated auth system
    if(name=="error"){
      emit clientUnauthorized();
    }else{
      QJsonValue args = obj.value("args");
      if(args.isArray()){ 
	  cauthkey = args.toArray().first().toString();
	  emit clientAuthorized();
	  //Now automatically re-subscribe to events as needed
	  for(int i=0; i<events.length(); i++){ sendEventSubscription(events[i]); }
      }
    }
  }else if(ID=="sysadm-client-event-auto"){
    //Reply to automated event subscription - don't need to save this
  }else if(namesp=="events"){
    //Event notification - not tied to any particular request
    if(name=="dispatcher"){ emit DispatcherEvent(obj.value("args")); }
  }else{
    //Now save this message into the cache for use later (if not an auth reply)
    if(!ID.isEmpty()){ 
      PENDING.removeAll(ID);
      BACK.insert(ID, obj);
    }
    emit newReply(ID, name, namesp, obj.value("args"));
  }
}
	