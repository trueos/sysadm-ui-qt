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
#include <QHostInfo>
#include <QtConcurrent>

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
  qRegisterMetaType<sysadm_client::EVENT_TYPE>("sysadm_client::EVENT_TYPE");
  connect(this, SIGNAL(sendOutputMessage(QString)), this, SLOT(forwardSocketMessage(QString)) );
  SOCKET = new QWebSocket("sysadm-client", QWebSocketProtocol::VersionLatest, this);
    SOCKET->setSslConfiguration(QSslConfiguration::defaultConfiguration());
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
    connect(this, SIGNAL(clientAuthorized()), pingTimer, SLOT(start()) );
  QueueTimer = new QTimer(this);
    QueueTimer->setInterval(20); //20ms interval between sending of messages (prevent overflowing the socket)
    QueueTimer->setSingleShot(true);
    connect(QueueTimer, SIGNAL(timeout()), this, SLOT(sendFromQueue()));
}

sysadm_client::~sysadm_client(){
  qDebug() << "Core deleted";
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

QString sysadm_client::bridgedHostname(QString bridge_id){
  if(!BRIDGE.contains(bridge_id)){ return ""; }
  return BRIDGE[bridge_id].hostname;
}

//Check if the sysadm server is running on the local system
bool sysadm_client::localhostAvailable(){
  #ifdef __FreeBSD__
    return QFile::exists("/usr/local/bin/sysadm-binary"); //server available
  #endif
  return false;
}

bool sysadm_client::localhostRunning(){ 
  //If the local server is running
  #ifdef __FreeBSD__
  return (0 == system("service sysadm status"));
  #endif
return false;
}

// Register for Event Notifications (no notifications by default)
void sysadm_client::registerForEvents(EVENT_TYPE event, bool receive){
  bool set = events.contains(event);
  qDebug() << "Register for event:" << event << events << set << receive;
  if( set && receive){ return; } //already registered
  else if(!set && !receive){ return; } //already unregistered
  else if(!set){ events << event; }
  else{ events.removeAll(event); }
  //Since this can be setup before the socket is connected - see if we can send this message right away
  if(SOCKET->isValid()){
    if(isbridge){
      //Send this event notice to all bridge connections
      QStringList ids = BRIDGE.keys();
      for(int i=0; i<ids.length(); i++){
        sendEventSubscription_bridge(ids[i], event, receive);
      }
    }else{
      sendEventSubscription(event, receive);
    }
  }
}

int sysadm_client::statePriority(){
  if( isActive() ){ return cPriority; }
  else{ return 0; }  
}

//Register the custom SSL Certificate with the server
void sysadm_client::registerCustomCert(){
  qDebug() << "Register SSL Certificate on Server:" << currentHost();
  if(SSL_cfg.isNull() || SOCKET==0 || !SOCKET->isValid()){ return; }
  //Get the custom cert
  QSslCertificate cert = SSL_cfg.localCertificate();
  QString pubkey, email, nickname;
  //for(int i=0; i<certs.length(); i++){
    //if(certs[i].issuerInfo(QSslCertificate::Organization).contains("SysAdm-client")){
      pubkey = QString(cert.publicKey().toPem().toBase64());
      email = cert.issuerInfo(QSslCertificate::EmailAddress).join("");
      nickname = cert.issuerInfo(QSslCertificate::CommonName).join("");
      //break;
    //}
  //}
  if(pubkey.isEmpty()){ return; } //no cert found
  qDebug() << " - Sent registration";
  //Now assemble the request JSON
  SSLsuccess = true; //set the internal flag to use SSL on next attempt
  QJsonObject obj;
    obj.insert("action","register_ssl_cert");
    obj.insert("pub_key", pubkey);
    obj.insert("email",email);
    obj.insert("nickname",nickname);
  this->communicate("sysadm-auto-cert-register","rpc","settings", obj);
  
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
  qDebug() << "Start Auth:" << currentHost();
  //uses cauthkey if empty inputs
  QJsonObject obj;
  obj.insert("namespace","rpc");
  obj.insert("id","sysadm-client-auth-auto");
  usedSSL = false;
  //bool noauth = false;
  if(user.isEmpty() || isbridge){
    if(cauthkey.isEmpty()){
      qDebug() << " - SSL Auth stage 1";
      //SSL Authentication (Stage 1)
      usedSSL = true;
      obj.insert("name","auth_ssl");
      obj.insert("args","");
    }else{
      qDebug() << " - Saved Token";
      //Saved token authentication
      obj.insert("name","auth_token");
      QJsonObject arg;
	arg.insert("token",cauthkey);
      obj.insert("args", arg);
    }
  }else{
    qDebug() << " - User/Pass Auth";
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
  qDebug() << "Start Bridge Auth:" << bridge_id;
  QJsonObject obj;
  obj.insert("namespace","rpc");
  obj.insert("id","sysadm-client-auth-auto");
  obj.insert("name","auth_ssl");
  QJsonObject args;
    args.insert("md5_key", pubkeyMD5(SSL_cfg));
  obj.insert("args",args);
  bridge_data data = getBridgeData(bridge_id);
  if( !data.enc_key.isEmpty() ){
    data.enc_key.clear();
    BRIDGE.insert(bridge_id, data);
  }
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
  qDebug() << "Send Event Subscription:" << event << arg << subscribe;
  this->communicate("sysadm-client-event-auto","events", subscribe ? "subscribe" : "unsubscribe", arg);
}

void sysadm_client::sendEventSubscription_bridge(QString bridge_id, EVENT_TYPE event, bool subscribe){
  QString arg;
  if(event == DISPATCHER){ arg = "dispatcher"; }
  else if(event == LIFEPRESERVER){ arg = "life-preserver"; }
  else if(event== SYSSTATE){ arg = "system-state"; }
  qDebug() << "Send Event Subscription:" << event << arg << subscribe;
  this->communicate_bridge(bridge_id, "sysadm-client-event-auto","events", subscribe ? "subscribe" : "unsubscribe", arg);
}

void sysadm_client::sendSocketMessage(QJsonObject msg){
  //Overload: Convert JSON to text for transport
  sendSocketMessage(QJsonDocument(msg).toJson(QJsonDocument::Compact));
}
void sysadm_client::sendSocketMessage(QString msg){
   emit sendOutputMessage(msg);
}

//Simplification functions
bridge_data sysadm_client::getBridgeData(QString ID){
  if(BRIDGE.contains(ID)){ return BRIDGE[ID]; }
  else{
    //Need to initialize the data first
    bridge_data data;
      data.enc_key = "";
      data.auth_tok = "";
    BRIDGE.insert(ID, data);
    return data;
  }
}

message_in sysadm_client::convertServerReply(QString reply){
  message_in msg;
  if(!reply.startsWith("{")){
    //Bridge routed message
    int index = reply.indexOf("\n");
    msg.from_bridge_id = reply.left(index);
    reply = reply.remove(0,index+1);
  }
  if(!msg.from_bridge_id.isEmpty() ){
    QByteArray key = getBridgeData(msg.from_bridge_id).enc_key;
    //encrypted message through bridge - decrypt it
    qDebug() << "Fully encoded message:" << reply;
    if(!key.isEmpty()){ reply = DecodeString(reply, key); }
    qDebug() << " - Decoded:" << reply;
  }
  //if(!msg.from_bridge_id.isEmpty()){  qDebug() << "Convert reply:" << reply; }
  QJsonDocument doc = QJsonDocument::fromJson(reply.toUtf8());
  if(doc.isObject()){ 
    msg.id = doc.object().value("id").toString();
    msg.namesp = doc.object().value("namespace").toString();
    msg.name = doc.object().value("name").toString();
    msg.args = doc.object().value("args");
  }else{
    qDebug() << "Error with data to JSON conversion:" << reply;
  }
  return msg; 
}

QString sysadm_client::pubkeyMD5(QSslConfiguration cfg){
  QCryptographicHash chash(QCryptographicHash::Md5);
        chash.addData( cfg.localCertificate().publicKey().toPem() );
   return QString(chash.result().toBase64()); 
}

QString sysadm_client::SSL_Encode_String(QString str, QSslConfiguration cfg){
  //Get the private key
  QByteArray privkey = cfg.privateKey().toPem();
  //Print out the public key associated with this private key for debugging
  qDebug() << "Associated Public Key:" << cfg.localCertificate().publicKey().toPem();

    //Reset/Load some SSL stuff
    //OpenSSL_add_all_algorithms();
    //ERR_load_crypto_strings();
  //Now use this private key to encode the given string
  unsigned char *encode = (unsigned char*)malloc(2*str.length()); //give it plenty of extra space as needed
  RSA *rsa= NULL;
  BIO *keybio = NULL;
  keybio = BIO_new_mem_buf(privkey.data(), -1);
  if(keybio==NULL){ qDebug() << "Could not create keybio"; return ""; }
  rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
  if(rsa==NULL){ BIO_free_all(keybio); qDebug() << "Could not read private key"; return ""; }
  int len = RSA_private_encrypt(str.length(), (unsigned char*)(str.toLatin1().data()), encode, rsa, RSA_PKCS1_PADDING);
  RSA_free(rsa);
  BIO_free_all(keybio);
  if(len <0){ qDebug() << "Could not encrypt string"; return ""; }
  else{ 
    //Now return this as a base64 encoded string
    QByteArray str_encode( (char*)(encode), len);
    qDebug() << "Encoded String Info";
    qDebug() << " - Raw string:" << str << "Length:" << str.length();
    qDebug() << " - Encoded string:" << str_encode << "Length:" << str_encode.length();
    str_encode = str_encode.toBase64();
    qDebug() << " - Enc string (base64):" << str_encode << "Length:" << str_encode.length();
    qDebug() << " - Enc string (QString):" << QString(str_encode);
    return QString( str_encode ); 
  }

}
QString sysadm_client::EncodeString(QString str, QByteArray key){
  qDebug() << "Encode String";
  bool pub=true;
  if(key.contains(" PUBLIC KEY--")){ pub=true; }
  else if(key.contains(" PRIVATE KEY--")){ pub=false; }
  else{ qDebug() << " - No Encode"; return str; } //unknown encryption - just return as-is
  return str.toLocal8Bit().toBase64(); //TEMPORARY BYPASS
  qDebug() << "Start encoding String:" << pub << str.length() << str <<  key;
  //Reset/Load some SSL stuff
    //OpenSSL_add_all_algorithms();
    //ERR_load_crypto_strings();


  //Now Encrypt the string
  //unsigned char *encode; // = (unsigned char*)malloc(2*str.length()); //give it plenty of extra space as needed
  RSA *rsa= NULL;
  BIO *keybio = NULL;
  keybio = BIO_new_mem_buf(key.data(), -1);
  if(keybio==NULL){ qDebug() << " - Bad keybio"; return ""; }
  QString outstring;
  if(!pub){
    //Using PRIVATE key to encrypt
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
    if(rsa==NULL){ qDebug() << " - Bad private rsa"; qDebug() << key.data(); BIO_free_all(keybio); return ""; }
    QJsonArray array;
    int rsa_size = RSA_size(rsa)/2;
    for(int i=0; i<str.length(); i+=rsa_size){
      unsigned char *encode = (unsigned char*)malloc( RSA_size(rsa) );
      QByteArray bytes; bytes.append(str.mid(i,rsa_size));
      int len = RSA_private_encrypt(bytes.size(), (unsigned char*)(bytes.data()), encode, rsa, RSA_PKCS1_PADDING);
      if(len <0){ 
        qDebug() << " - Bad private rsa encrypt";  
        qDebug() << ERR_error_string (ERR_peek_error(), NULL);
        qDebug() << ERR_error_string (ERR_peek_last_error(), NULL); 
        array = QJsonArray();
        break;
      }
      array <<  QString( QByteArray( (char*)(encode), len).toBase64() );
    }
    RSA_free(rsa);
    BIO_free_all(keybio);
    if(array.count()==1){ outstring = array[0].toString(); }
    else if(array.count()>1){ outstring = QJsonDocument(array).toJson(QJsonDocument::Compact); }
  }else{
    //Using PUBLIC key to encrypt
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa,NULL, NULL);
    if(rsa==NULL){ qDebug() << " - Bad public rsa"; qDebug() << key.data(); BIO_free_all(keybio); return ""; }
    QJsonArray array;
    int rsa_size = RSA_size(rsa)/2;
    for(int i=0; i<str.length(); i+=rsa_size){
      unsigned char *encode = (unsigned char*)malloc( RSA_size(rsa) );
      QByteArray bytes; bytes.append(str.mid(i,rsa_size));
      int len = RSA_public_encrypt(bytes.size(), (unsigned char*)(bytes.data()), encode, rsa, RSA_PKCS1_PADDING);
      if(len <0){ 
        qDebug() << " - Bad public rsa encrypt";  
        qDebug() << ERR_error_string (ERR_peek_error(), NULL);
        qDebug() << ERR_error_string (ERR_peek_last_error(), NULL); 
        array = QJsonArray();
        break;
      }
      array <<  QString( QByteArray( (char*)(encode), len).toBase64() );
    }
    RSA_free(rsa);
    BIO_free_all(keybio);
    if(array.count()==1){ outstring = array[0].toString(); }
    else if(array.count()>1){ outstring = QJsonDocument(array).toJson(QJsonDocument::Compact); }
  }
  qDebug() << " - Finished Encode";
  return outstring;
}

QString sysadm_client::DecodeString(QString str, QByteArray key){
  bool pub=true;
  if(key.contains(" PUBLIC KEY--")){ pub=true; }
  else if(key.contains(" PRIVATE KEY--")){ pub=false; }
  else{  //unknown encryption - just return as-is
    if(!key.isEmpty()){ qDebug() << "Unknown key type!!" << key; } 
    return str; 
  }
  //Convert the input string into block elements as needed (and decode base64);
  QList<QByteArray> blocks;
  //QJsonDocument doc = QJsonDocument::fromJson(str.toLocal8Bit());
  //if(doc.isNull()){
    //No individual blocks - just one string
    QByteArray bytes; bytes.append(str);
    blocks << QByteArray::fromBase64(bytes);
  /*}else if(doc.isArray()){
    for(int i=0; i<doc.array().count(); i++){
      QByteArray bytes; bytes.append(doc.array()[i].toString());
      blocks << QByteArray::fromBase64(bytes);
    }
  }else{
    //Already valid JSON - return it
    return str;
  }*/
  qDebug() << "Decoded String:" << bytes;
  return QString(blocks.join()); //TEMPORARY BYPASS

   qDebug() << "Start decoding String:" << pub << str;//<< key;
  //Reset/Load some SSL stuff
    //OpenSSL_add_all_algorithms();
    //ERR_load_crypto_strings();

  QString outstring; //output string
  //unsigned char *decode = (unsigned char*)malloc(5*bytes.size());
  RSA *rsa= NULL;
  BIO *keybio = NULL;
  qDebug() << " - Generate keybio";
  keybio = BIO_new_mem_buf(key.data(), -1);
  if(keybio==NULL){ return ""; }
  qDebug() << " - Read pubkey";
  if(pub){
    //PUBLIC KEY
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa,NULL, NULL);
    if(rsa==NULL){ qDebug() << " - Invalid Public RSA key!!" <<  key; BIO_free_all(keybio); return ""; }
    //decode = (unsigned char*)malloc( RSA_size(rsa) );
    qDebug() << " - Decrypt string";
    for(int i=0; i<blocks.length(); i++){
      unsigned char *decode = (unsigned char*)malloc(2*blocks[i].size());
      int len = RSA_public_decrypt(blocks[i].size(), (unsigned char*)(blocks[i].data()), decode, rsa, RSA_PKCS1_PADDING);
      if(len<0){ 
        qDebug() << " - Could not decrypt"; 
        qDebug() << ERR_error_string (ERR_peek_error(), NULL);
        qDebug() << ERR_error_string (ERR_peek_last_error(), NULL); 
        outstring.clear();
        break;
      }
      qDebug() << " - done";
      outstring.append( QString( QByteArray( (char*)(decode), len) ) );
    }
    RSA_free(rsa);
    BIO_free_all(keybio);

  }else{
    //PRIVATE KEY
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
    if(rsa==NULL){ qDebug() << " - Invalid RSA key!!"; BIO_free_all(keybio); return ""; }
    //decode = (unsigned char*)malloc( RSA_size(rsa) );
    qDebug() << " - Decrypt string";
    for(int i=0; i<blocks.length(); i++){
      unsigned char *decode = (unsigned char*)malloc(2*blocks[i].size());
      int len = RSA_private_decrypt(blocks[i].size(), (unsigned char*)(blocks[i].data()), decode, rsa, RSA_PKCS1_PADDING);
      if(len<0){ 
        qDebug() << " - Could not decrypt"; 
        qDebug() << ERR_error_string (ERR_peek_error(), NULL);
        qDebug() << ERR_error_string (ERR_peek_last_error(), NULL); 
        outstring.clear();
        break;
      }
      qDebug() << " - done";
      outstring.append( QString( QByteArray( (char*)(decode), len) ) );
    }
    RSA_free(rsa);
    BIO_free_all(keybio);
  }
  return outstring;
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
    sendSocketMessage(requests[i]);
  }  
}
void sysadm_client::communicate_bridge(QString bridge_host_id, QString ID, QString namesp, QString name, QJsonValue args){
  //Overloaded function for a request which needs assembly
  QJsonObject obj;
  obj.insert("namespace",namesp);
  obj.insert("name", name);
  obj.insert("id", ID);
  obj.insert("args", args);
  QList<QJsonObject> list;
    list << obj;
  communicate_bridge(bridge_host_id, list);
}

void sysadm_client::communicate_bridge(QString bridge_host_id, QJsonObject request){
  //Overloaded function for a single JSON request
  QList<QJsonObject> list;
    list << request;
  communicate_bridge(bridge_host_id, list);
}

void sysadm_client::communicate_bridge(QString bridge_host_id, QList<QJsonObject> requests){
  if(bridge_host_id.isEmpty()){ communicate(requests); return; } //run the non-bridge version
  qDebug() << "Communicate Bridge:" << bridge_host_id;
  if(!BRIDGE.contains(bridge_host_id)){
    qDebug() << "Invalid bridge host:" << bridge_host_id;
    return;
  }
  qDebug() << "Get bridge data";
  bridge_data data = getBridgeData(bridge_host_id);
  QByteArray key = data.enc_key;
  qDebug() << "Start loop over requests:" << requests.length();
  for(int i=0; i<requests.length(); i++){
    QString ID = requests[i].value("id").toString();
    if(ID.isEmpty()){ 
      qDebug() << "Malformed JSON request:" << requests[i]; 
      continue; 
    }

    //Save this into the cache
    //SENT.insert(bridge_host_id+"::::"+ID, requests[i]);
    //if(BACK.contains(bridge_host_id+"::::"+ID)){ BACK.remove(bridge_host_id+"::::"+ID); }
    //PENDING << bridge_host_id+"::::"+ID;
    //Now send off the message
    if(SOCKET->isValid()){ 
         qDebug() << "Start encoding message";
        QString enc_msg = EncodeString( QJsonDocument(requests[i]).toJson(QJsonDocument::Compact), key);
        qDebug() << "Send encoded message";
	sendSocketMessage(bridge_host_id+"\n"+enc_msg);
        qDebug() << " - finished sending message";
	//if(pingTimer->isActive()){ pingTimer->stop(); pingTimer->start(); } //reset the timer for this interval
    }
  }
}

// === PRIVATE SLOTS ===
void sysadm_client::setupSocket(){
  qDebug() << "Setup Socket:" << SOCKET->isValid();
  if(SOCKET->isValid()){ return; }
  //Setup the SSL config as needed
  SSLsuccess = false;
  //uses chost for setup
  // - assemble the host URL
  if(chost.contains("://")){ chost = chost.section("://",1,1); } //Chop off the custom http/ftp/other header (always need "wss://")
  QString url = "wss://"+chost;
  bool hasport = false;
  url.section(":",-1).toInt(&hasport); //check if the last piece of the url is a valid number
  //Could add a check for a valid port number as well - but that is a bit overkill right now
  if(!hasport){ url.append(":"+QString::number(WSPORTDEFAULT)); }
  else if(url.endsWith(":12149")){ isbridge = true; } //assume this is a bridge for the moment (will adjust on connection)
  qDebug() << " Open WebSocket:  URL:" << url;
  QTimer::singleShot(0,SOCKET, SLOT(ignoreSslErrors()) );
  SOCKET->open(QUrl(url));
  connectTimer->start();
}

void sysadm_client::sendPing(){
  communicate("sysadm_client_identify", "rpc","identify","");  
  if(!this->isReady()){ return; }
  communicate("sysadm_client_ping", "rpc","query","");
  QStringList b_id = BRIDGE.keys();
  for(int i=0; i<b_id.length(); i++){
    communicate_bridge(b_id[i], "sysadm_client_identify", "rpc", "identify","");
    communicate_bridge(b_id[i], "sysadm_client_ping", "rpc", "query","");
  }
}

//Socket signal/slot connections
void sysadm_client::socketConnected(){ //Signal: connected()
  if(connectTimer->isActive()){ connectTimer->stop(); }
  //keepActive = true; //got a valid connection - try to keep this open automatically unless the user closes it
  //emit clientConnected();
  if(DEBUG){ qDebug() << "Socket Connected:"; }
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
  BRIDGE.clear();
  isbridge = false;
  cauthkey.clear();
  if(keepActive){ 
    //Socket closed due to timeout/server
    // Go ahead and re-open it in one minute if possible with the last-used settings/auth
    qDebug() << " - - Will attempt to reconnect in 1 minute";
    QTimer::singleShot(60000, this, SLOT(setupSocket()) );
  }
  emit clientDisconnected();
  //Server cache is now invalid - completely lost connection
  cPriority = -1;
  SENT.clear(); BACK.clear(); PENDING.clear(); 
}

void sysadm_client::socketSslErrors(const QList<QSslError>&errlist){ //Signal: sslErrors()
  //qWarning() << "SSL Errors Detected:" << errlist.length();
  QList<QSslError> ignored;
  for(int i=0; i< errlist.length(); i++){
    if(errlist[i].error()==QSslError::SelfSignedCertificate || errlist[i].error()==QSslError::HostNameMismatch ){
      qDebug() << " - (IGNORED) " << errlist[i].errorString();
      ignored << errlist[i];
    }else{
      qWarning() << "Unhandled SSL Error:" << errlist[i].errorString();
    }
  }
  if(ignored.length() != errlist.length()){
    qWarning() << "Closing Connection due to unhandled SSL errors";
    SOCKET->close(); //SSL errors - close the connection
  }
}

void sysadm_client::socketError(QAbstractSocket::SocketError err){ //Signal:: error()
  qWarning() << "Socket Error detected:" << err;
  if(err==QAbstractSocket::SslHandshakeFailedError){qWarning() << " - SSL Handshake Failed"; }
  qWarning() << " - Final websocket error:" << SOCKET->errorString();
}
//void sysadm_client::socketProxyAuthRequired(const QNetworkProxy &proxy, QAuthenticator *auth); //Signal: proxyAuthenticationRequired()

// - Main message output routine (tied to an internal signal - don't use manually)
void sysadm_client::forwardSocketMessage(QString msg){
  /*if(SOCKET->isValid()){ 
    SOCKET->sendTextMessage(msg);
    if(pingTimer->isActive()){ pingTimer->stop(); pingTimer->start(); } //reset the timer for this interval
  }*/
  QUEUE << msg;
  if(!QueueTimer->isActive()){ QueueTimer->start(); }
}

void sysadm_client::sendFromQueue(){
  if(QUEUE.isEmpty()){ return; }
  QString msg = QUEUE.takeFirst();
  if(SOCKET->isValid()){ 
    SOCKET->sendTextMessage(msg);
    if(pingTimer->isActive()){ pingTimer->stop(); pingTimer->start(); } //reset the timer for this interval
  }
  if(!QUEUE.isEmpty()){ QueueTimer->start(); }
}

// - Main message input parsing
void sysadm_client::socketMessage(QString msg){ //Signal: textMessageReceived()
  //Handle this message in a separate thread
  QtConcurrent::run(this, &sysadm_client::handleMessage, msg);
}

void sysadm_client::handleMessage(const QString msg){
  //if(DEBUG){  qDebug() << "New Reply From Server:" << msg; }
  if(DEBUG){ qDebug() << "Got Message"; }
  message_in msg_in = convertServerReply(msg);
  if(!handleMessageInternally(msg_in)){
    qDebug() << "Send out message:";
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
  //if(msg.name=="response" && !PENDING.contains(msg.id) && msg.id!="sysadm-client-auth-auto"){ return true; } //do nothing - might be an injected/fake response
  if(msg.id == "sysadm-client-event-auto"){ return true; } //do nothing - automated response
  //HANDLE AUTH SYSTEMS
  QJsonObject reply;
  if(msg.id=="sysadm_client_identify"){
    if(DEBUG){ qDebug() << "Identification Reply:" << msg.from_bridge_id << msg.args; }
    if(msg.from_bridge_id.isEmpty()){
      QString type = msg.args.toObject().value("type").toString();
      bool startauth = false;
      bool oldisbridge = isbridge;
      if(type=="bridge"){ isbridge = true; startauth = cauthkey.isEmpty(); }
      else if(type=="server"){ isbridge = false; startauth = cauthkey.isEmpty(); }
      else{  
        qDebug() << "Unknown system type:" << type <<"\nClosing Connection..."; 
        this->closeConnection();  //unknown type of system - disconnect now
      }
      //qDebug() << "Got identify response:" << type << isbridge << startauth;
    
      if(startauth){
        keepActive = true; //got a valid connection - try to keep this open automatically unless the user closes it
        emit clientConnected();
        if(isbridge){ cauthkey.clear(); performAuth("", ""); } //force SSL auth
        else{ performAuth(cuser, cpass); }
        cpass.clear(); //just to ensure no trace left in memory
        //Ensure SSL connection to non-localhost (only user needed for localhost)
        if(chost!=LOCALHOST && !chost.startsWith(LOCALHOST+":") ){ cuser.clear(); }
      }
      if(oldisbridge != isbridge){ emit clientTypeChanged(); }
    }else{
      //Bridged relay identification
      bridge_data data = getBridgeData(msg.from_bridge_id);
        data.hostname = msg.args.toObject().value("hostname").toString();
      BRIDGE.insert(msg.from_bridge_id, data);
    }

  }else if(msg.id=="sysadm-client-auth-auto"){
    //qDebug() << "Auth Reply" << msg.name << msg.namesp << msg.args;
    //Reply to automated auth system
    if(msg.name=="error"){
      qDebug() << "Authentication error:" << chost << "Bridge:" << isbridge << "Bridge_Relay:" << msg.from_bridge_id;
      qDebug() << " - Error Information:" << msg.args;
      if(msg.from_bridge_id.isEmpty()){
        QTimer::singleShot(0, this, SLOT(closeConnection()) );
        emit clientUnauthorized();
      }else{
        //Clear out the data for this bridged connection
        bridge_data data = getBridgeData(msg.from_bridge_id);
          data.enc_key.clear();
          data.auth_tok.clear();
        BRIDGE.insert(msg.from_bridge_id, data);
        //Now send out an updated list of which bridged connections are available
        QStringList curr = BRIDGE.keys();
          for(int i=0; i<curr.length(); i++){
            if(getBridgeData(curr[i]).auth_tok.isEmpty()){ curr.removeAt(i); i--; }
          }
          emit bridgeConnectionsChanged(curr);
      }
    }else{
      if(msg.args.isArray() && msg.args.toArray().count()>=2){ 
	//Successful authorization
	if(msg.from_bridge_id.isEmpty()){
          if(cuser.isEmpty()){ SSLsuccess = true; }
	  cauthkey = msg.args.toArray().first().toString();
          qDebug() << "Connection Authorized:" << chost;
	  emit clientAuthorized();
	  //pingTimer->start();
	  //Now automatically re-subscribe to events as needed
	  //qDebug() << "Re-subscribe to events:" << events;
	  for(int i=0; i<events.length(); i++){ sendEventSubscription(events[i]); }
        }else{
          qDebug() << "Bridge Connection Authorized:" << msg.from_bridge_id;
          bridge_data data = getBridgeData(msg.from_bridge_id); //make sure the data struct is initialized first
          data.auth_tok = msg.args.toArray().first().toString();
	  BRIDGE.insert(msg.from_bridge_id, data);
          QStringList curr = BRIDGE.keys();
          for(int i=0; i<curr.length(); i++){
            if(getBridgeData(curr[i]).auth_tok.isEmpty()){ curr.removeAt(i); i--; }
          }
          emit bridgeConnectionsChanged(curr);
	  emit bridgeAuthorized(msg.from_bridge_id);
	  //Now automatically re-subscribe to events as needed
	  //qDebug() << "Re-subscribe to events:" << events;
	  for(int i=0; i<events.length(); i++){ sendEventSubscription_bridge(msg.from_bridge_id, events[i]); }
       }

      }else if(msg.args.isObject()){
        //SSL Auth Stage 2
        qDebug() << "Got Stage 2 SSL Auth:" << chost << msg.from_bridge_id;//<< msg.args.toObject();
        QString randomkey = msg.args.toObject().value("test_string").toString();
          //qDebug() << " - randomkey (raw):" << randomkey;
        if(!msg.from_bridge_id.isEmpty()){
          //New Encryption layer starting - randomkey needs decoding too
          QByteArray c_key = SSL_cfg.privateKey().toPem(); //SSL_cfg.localCertificate().publicKey().toPem();
          randomkey = DecodeString(randomkey, c_key);
          //qDebug() << "randomkey (decoded):" << randomkey;
          //Also re-assemble the new private key to use in the future
          QByteArray key = c_key; //current private key
          if(msg.args.toObject().contains("new_ssl_key")){
            QByteArray p_key;
            QJsonValue keyval = msg.args.toObject().value("new_ssl_key");
            if(keyval.isArray()){
              QJsonArray pkeyarr = keyval .toArray();
              QByteArray p_key;
              for(int i=0; i<pkeyarr.count(); i++){
	        p_key.append( DecodeString(pkeyarr[i].toString(), c_key) );
              }

            }else if(keyval.isString()){
              p_key.append( DecodeString(keyval.toString(), c_key) );
            }
            //qDebug() << "New Key:" << p_key;
            //qDebug() << "Old Key:" << key;
            key = p_key;
          }
          bridge_data data = getBridgeData(msg.from_bridge_id); //make sure the data struct is initialized first
          data.enc_key = key; //save this key for use later
          BRIDGE.insert(msg.from_bridge_id, data);
        }
        qDebug() << "Random key sent by server:" << randomkey;
        if(!randomkey.isEmpty()){
	  reply.insert("id",msg.id);
          reply.insert("name","auth_ssl");
          reply.insert("namespace","rpc");
          QJsonObject args;
	  QString enc_string;
          if(isbridge && msg.from_bridge_id.isEmpty()){ qDebug() << "Auth Bridge:"; enc_string = SSL_Encode_String(randomkey, SSL_cfg_bridge); }
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
        //Now go through and clean out any old bridge data (from disconnections)
        QStringList curr = BRIDGE.keys();
        bool removeonly = false;
        for(int i=0; i<curr.length(); i++){
          if(!conns.contains(curr[i])){ removeonly = true; qDebug() << "Removing bridge data"; BRIDGE.remove(curr[i]); }
        }
      QStringList avail;
      for(int i=0; i<conns.count(); i++){
        QString conn = conns[i].toString();
        if(conn.isEmpty()){ continue; }
        avail << conn;

        //Now add the connection to the internal DB if needed
        if(getBridgeData(conn).auth_tok.isEmpty() && getBridgeData(conn).enc_key.isEmpty()){
          removeonly = false;
          communicate_bridge(conn, "sysadm_client_identify", "rpc", "identify","");
          performAuth_bridge(conn);
        }
      }
      if(removeonly){ emit bridgeConnectionsChanged(avail); } 
    }
  }else if(msg.namesp=="rpc" && msg.name=="identify"){
          QJsonObject args;
          args.insert("type", "client");
          args.insert("hostname", QHostInfo::localHostName() );
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
    //if(!msg.from_bridge_id.isEmpty()){ qDebug() << "INTERNAL REPLY:" << reply; }
    if(msg.from_bridge_id.isEmpty()){ this->communicate(reply); }
    else{ this->communicate_bridge(msg.from_bridge_id, reply); }
  }
  return true;
}
