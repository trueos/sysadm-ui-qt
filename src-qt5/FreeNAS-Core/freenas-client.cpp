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

//extern QSettings *settings;
//Unencrypted SSL objects (after loading them by user passphrase)
//extern QSslConfiguration SSL_cfg, SSL_cfg_bridge; //Check "isNull()" to see if the user settings have been loaded yet

// === PUBLIC ===
freenas_client::freenas_client(){
  qRegisterMetaType<freenas_client::EVENT_TYPE>("freenas_client::EVENT_TYPE");
  connect(this, SIGNAL(sendOutputMessage(QString)), this, SLOT(forwardSocketMessage(QString)) );
  SOCKET = new QWebSocket("sysadm-client", QWebSocketProtocol::VersionLatest, this);
    SOCKET->setSslConfiguration(QSslConfiguration::defaultConfiguration());
    //use the new Qt5 connection syntax for compile time checks that connections are valid
    connect(SOCKET, &QWebSocket::connected, this, &freenas_client::socketConnected);
    connect(SOCKET, &QWebSocket::disconnected, this, &freenas_client::socketClosed);
    connect(SOCKET, &QWebSocket::textMessageReceived, this, &freenas_client::socketMessage);
    //Old connect syntax for the "error" signal (special note about issues in the docs)
    connect(SOCKET, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)) );
    connect(SOCKET, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(socketSslErrors(const QList<QSslError>&)) );
  keepActive=SSLsuccess=usedSSL=isbridge=false; //not setup yet
  //events << SYSSTATE; //always pre-register for this type of event
  //cPriority = -1;
  //Timer for events while possibly attempting a connection for 30s->1minute
  connectTimer = new QTimer(this);
    connectTimer->setInterval(1000); //1 second intervals
    connect(connectTimer, SIGNAL(timeout()), this, SIGNAL(clientReconnecting()) );
  pingTimer = new QTimer(this);
    pingTimer->setInterval(90000); //90 second intervals 
    connect(pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()) );
    connect(this, SIGNAL(clientAuthorized()), pingTimer, SLOT(start()) );
}

freenas_client::~freenas_client(){
  //qDebug() << "Core deleted";
}

// Overall Connection functions (start/stop)
void freenas_client::openConnection(QString user, QString pass, QString hostIP){
  cuser = user; cpass = pass; chost = hostIP;
  //qDebug() << "Client: Setup connection:" << user << pass << hostIP;
  setupSocket();
}

/*void freenas_client::openConnection(QString authkey, QString hostIP){
  cauthkey = authkey; chost = hostIP;
  setupSocket();
}

void freenas_client::openConnection(QString hostIP){
  chost = hostIP;
  setupSocket();
}
void freenas_client::openConnection(){
  setupSocket();
}*/

void freenas_client::closeConnection(){
  keepActive = false;
  //de-authorize the current auth token
  /*if(!cauthkey.isEmpty()){
    cauthkey.clear(); 
    clearAuth();
  }*/
  //Now close the connection
  SOCKET->close(QWebSocketProtocol::CloseCodeNormal, "freenas-client closed");
}

QString freenas_client::currentHost(){
  return chost;	
}

bool freenas_client::isActive(){
  return ( (SOCKET!=0) && SOCKET->isValid() );	
}

/*bool freenas_client::isLocalHost(){
  return (chost==LOCALHOST || chost.startsWith(LOCALHOST+":"));
}*/

/*bool freenas_client::needsBaseAuth(){
  return !SSLsuccess;
}*/

bool freenas_client::isReady(){
  return pingTimer->isActive();
}

bool freenas_client::isConnecting(){
  //returns true if it is currently trying to establish a connection
  return connectTimer->isActive();
}

/*bool freenas_client::isBridge(){
  return isbridge;
}*/

/*QStringList freenas_client::bridgeConnections(){
  return BRIDGE.keys();
}

QString freenas_client::bridgedHostname(QString bridge_id){
  if(!BRIDGE.contains(bridge_id)){ return ""; }
  return BRIDGE[bridge_id].hostname;
}*/

//Check if the sysadm server is running on the local system
//bool freenas_client::localhostAvailable(){
  //#ifdef __FreeBSD__
    /*QProcess P;
    P.start("sockstat -l46 -P tcp -p "+QString::number(WSPORTDEFAULT) );
    P.waitForFinished();
    if( 0 == P.exitCode() ){
      if( QString(P.readAllStandardOutput()).contains(QString::number(WSPORTDEFAULT)) ){ return true; }
    }*/
    //return QFile::exists("/usr/local/bin/sysadm-binary"); //server available
  //#endif
  //return false;
//}

// Register for Event Notifications (no notifications by default)
/*void freenas_client::registerForEvents(EVENT_TYPE event, bool receive){
  bool set = events.contains(event);
  //qDebug() << "Register for event:" << event << events << set << receive;
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

int freenas_client::statePriority(){
  if( isActive() ){ return cPriority; }
  else{ return 0; }  
}*/

//Register the custom SSL Certificate with the server
/*void freenas_client::registerCustomCert(){
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
  
}*/

// Messages which are still pending a response
//QStringList freenas_client::pending(){ return PENDING; } //returns only the "id" for each 

// Fetch a message from the recent cache
/*QJsonObject freenas_client::cachedRequest(QString id){
  if(SENT.contains(id)){ return SENT.value(id); }
  else{ return QJsonObject(); }
}*/

/*QJsonValue freenas_client::cachedReply(QString id){
  if(BACK.contains(id)){ return BACK.value(id); }
  else{ return QJsonObject(); }  
}*/

// === PRIVATE ===
//Functions to do the initial socket setup
void freenas_client::performAuth(QString user, QString pass){
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

/*void freenas_client::performAuth_bridge(QString bridge_id){
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
}*/

void freenas_client::clearAuth(){
  /*QJsonObject obj;
  obj.insert("namespace","rpc");
  obj.insert("id","sysadm-client-auth-auto");
  obj.insert("name","auth_clear");
  obj.insert("args","");	
  sendSocketMessage(obj);*/
  emit clientUnauthorized();
}

//Communication subroutines with the server
/*void freenas_client::sendEventSubscription(EVENT_TYPE event, bool subscribe){
  QString arg;
  if(event == DISPATCHER){ arg = "dispatcher"; }
  else if(event == LIFEPRESERVER){ arg = "life-preserver"; }
  else if(event== SYSSTATE){ arg = "system-state"; }
  //qDebug() << "Send Event Subscription:" << event << arg << subscribe;
  this->communicate("sysadm-client-event-auto","events", subscribe ? "subscribe" : "unsubscribe", arg);
}

void freenas_client::sendEventSubscription_bridge(QString bridge_id, EVENT_TYPE event, bool subscribe){
  QString arg;
  if(event == DISPATCHER){ arg = "dispatcher"; }
  else if(event == LIFEPRESERVER){ arg = "life-preserver"; }
  else if(event== SYSSTATE){ arg = "system-state"; }
  //qDebug() << "Send Event Subscription:" << event << arg << subscribe;
  this->communicate_bridge(bridge_id, "sysadm-client-event-auto","events", subscribe ? "subscribe" : "unsubscribe", arg);
}

void freenas_client::sendSocketMessage(QJsonObject msg){
  //Overload: Convert JSON to text for transport
  sendSocketMessage(QJsonDocument(msg).toJson(QJsonDocument::Compact));
}
void freenas_client::sendSocketMessage(QString msg){
   emit sendOutputMessage(msg);
}

//Simplification functions
bridge_data freenas_client::getBridgeData(QString ID){
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

message_in freenas_client::convertServerReply(QString reply){
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
    //qDebug() << "Fully encoded message:" << reply;
    if(!key.isEmpty()){ reply = DecodeString(reply, key); }
    //qDebug() << " - Decoded:" << reply;
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

QString freenas_client::pubkeyMD5(QSslConfiguration cfg){
  QCryptographicHash chash(QCryptographicHash::Md5);
        chash.addData( cfg.localCertificate().publicKey().toPem() );
   return QString(chash.result().toBase64()); 
}*/

/*QString freenas_client::SSL_Encode_String(QString str, QSslConfiguration cfg){
  //Get the private key
  QByteArray privkey = cfg.privateKey().toPem();
  
    //Reset/Load some SSL stuff
    //OpenSSL_add_all_algorithms();
    //ERR_load_crypto_strings();
  //Now use this private key to encode the given string
  unsigned char *encode = (unsigned char*)malloc(2*str.length()); //give it plenty of extra space as needed
  RSA *rsa= NULL;
  BIO *keybio = NULL;
  keybio = BIO_new_mem_buf(privkey.data(), -1);
  if(keybio==NULL){ return ""; }
  rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
  if(rsa==NULL){ BIO_free_all(keybio); return ""; }
  int len = RSA_private_encrypt(str.length(), (unsigned char*)(str.toLatin1().data()), encode, rsa, RSA_PKCS1_PADDING);
  RSA_free(rsa);
  BIO_free_all(keybio);
  if(len <0){ return ""; }
  else{ 
    //Now return this as a base64 encoded string
    QByteArray str_encode( (char*)(encode), len);*/
    /*qDebug() << "Encoded String Info";
    qDebug() << " - Raw string:" << str << "Length:" << str.length();
    qDebug() << " - Encoded string:" << str_encode << "Length:" << str_encode.length();*/
    //str_encode = str_encode.toBase64();
    /*qDebug() << " - Enc string (base64):" << str_encode << "Length:" << str_encode.length();
    qDebug() << " - Enc string (QString):" << QString(str_encode);*/
    //return QString( str_encode ); 
  //}

//}

/*QString freenas_client::EncodeString(QString str, QByteArray key){
  qDebug() << "Encode String";
  bool pub=true;
  if(key.contains(" PUBLIC KEY--")){ pub=true; }
  else if(key.contains(" PRIVATE KEY--")){ pub=false; }
  else{ qDebug() << " - No Encode"; return str; } //unknown encryption - just return as-is
  return str.toLocal8Bit().toBase64(); //TEMPORARY BYPASS
  //qDebug() << "Start encoding String:" << pub << str.length() << str <<  key;
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
}*/

/*QString freenas_client::DecodeString(QString str, QByteArray key){
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
    blocks << QByteArray::fromBase64(bytes);*/
  /*}else if(doc.isArray()){
    for(int i=0; i<doc.array().count(); i++){
      QByteArray bytes; bytes.append(doc.array()[i].toString());
      blocks << QByteArray::fromBase64(bytes);
    }
  }else{
    //Already valid JSON - return it
    return str;
  }*/
  //qDebug() << "Decoded String:" << bytes;
  //return QString(blocks.join()); //TEMPORARY BYPASS

   //qDebug() << "Start decoding String:" << pub << str;//<< key;
  //Reset/Load some SSL stuff
    //OpenSSL_add_all_algorithms();
    //ERR_load_crypto_strings();

  /*QString outstring; //output string
  //unsigned char *decode = (unsigned char*)malloc(5*bytes.size());
  RSA *rsa= NULL;
  BIO *keybio = NULL;
  //qDebug() << " - Generate keybio";
  keybio = BIO_new_mem_buf(key.data(), -1);
  if(keybio==NULL){ return ""; }
  //qDebug() << " - Read pubkey";
  if(pub){
    //PUBLIC KEY
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa,NULL, NULL);
    if(rsa==NULL){ qDebug() << " - Invalid Public RSA key!!" <<  key; BIO_free_all(keybio); return ""; }
    //decode = (unsigned char*)malloc( RSA_size(rsa) );
    //qDebug() << " - Decrypt string";
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
      //qDebug() << " - done";
      outstring.append( QString( QByteArray( (char*)(decode), len) ) );
    }
    RSA_free(rsa);
    BIO_free_all(keybio);

  }else{
    //PRIVATE KEY
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
    if(rsa==NULL){ qDebug() << " - Invalid RSA key!!"; BIO_free_all(keybio); return ""; }
    //decode = (unsigned char*)malloc( RSA_size(rsa) );
    //qDebug() << " - Decrypt string";
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
      //qDebug() << " - done";
      outstring.append( QString( QByteArray( (char*)(decode), len) ) );
    }
    RSA_free(rsa);
    BIO_free_all(keybio);
  }
  return outstring;
}*/

// === PUBLIC SLOTS ===
// Communications with server (send message, get response via signal later)
/*void freenas_client::communicate(QString ID, QString namesp, QString name, QJsonValue args){
  //Overloaded function for a request which needs assembly
  QJsonObject obj;
  obj.insert("namespace",namesp);
  obj.insert("name", name);
  obj.insert("id", ID);
  obj.insert("args", args);
  //qDebug() << "Send Message:" << QJsonDocument(obj).toJson();
  communicate(QList<QJsonObject>() << obj);
}*/

void freenas_client::communicate(QJsonObject request){
  //Overloaded function for a single JSON request
  communicate(QList<QJsonObject>() << request);
}

void freenas_client::communicate(QList<QJsonObject> requests){
  for(int i=0; i<requests.length(); i++){
    /*QString ID = requests[i].value("id").toString();
    if(ID.isEmpty()){ 
      qDebug() << "Malformed JSON request:" << requests[i]; 
      continue; 
    }
    //Save this into the cache
    SENT.insert(ID, requests[i]);
    if(BACK.contains(ID)){ BACK.remove(ID); }
     PENDING << ID;
    //Now send off the message*/
    sendSocketMessage(requests[i]);
  }  
}
/*void freenas_client::communicate_bridge(QString bridge_host_id, QString ID, QString namesp, QString name, QJsonValue args){
  //Overloaded function for a request which needs assembly
  QJsonObject obj;
  obj.insert("namespace",namesp);
  obj.insert("name", name);
  obj.insert("id", ID);
  obj.insert("args", args);
  QList<QJsonObject> list;
    list << obj;
  communicate_bridge(bridge_host_id, list);
}*/

/*void freenas_client::communicate_bridge(QString bridge_host_id, QJsonObject request){
  //Overloaded function for a single JSON request
  QList<QJsonObject> list;
    list << request;
  communicate_bridge(bridge_host_id, list);
}*/

/*void freenas_client::communicate_bridge(QString bridge_host_id, QList<QJsonObject> requests){
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
}*/

// === PRIVATE SLOTS ===
void freenas_client::setupSocket(){
  //qDebug() << "Setup Socket:" << SOCKET->isValid();
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

void freenas_client::sendPing(){
  /*communicate("freenas_client_identify", "rpc","identify","");  
  if(!this->isReady()){ return; }
  communicate("freenas_client_ping", "rpc","query","");
  QStringList b_id = BRIDGE.keys();
  for(int i=0; i<b_id.length(); i++){
    communicate_bridge(b_id[i], "freenas_client_identify", "rpc", "identify","");
    communicate_bridge(b_id[i], "freenas_client_ping", "rpc", "query","");
  }*/
}

//Socket signal/slot connections
void freenas_client::socketConnected(){ //Signal: connected()
  if(connectTimer->isActive()){ connectTimer->stop(); }
  //keepActive = true; //got a valid connection - try to keep this open automatically unless the user closes it
  //emit clientConnected();
  if(DEBUG){ qDebug() << "Socket Connected:"; }
  //communicate("freenas_client_identify","rpc","identify",""); //ask the other system to identify itself
  performAuth(cuser, cpass);
  //cpass.clear(); //just to ensure no trace left in memory
  //Ensure SSL connection to non-localhost (only user needed for localhost)
  //if(chost!=LOCALHOST && !chost.startsWith(LOCALHOST+":") ){ cuser.clear(); }
}

void freenas_client::socketClosed(){ //Signal: disconnected()
  qDebug() << " - Connection Closed:" << chost;
  if(connectTimer->isActive()){ connectTimer->stop(); }
  if(pingTimer->isActive()){ pingTimer->stop(); }
  //BRIDGE.clear();
  //isbridge = false;
  //cauthkey.clear();
  if(keepActive){ 
    //Socket closed due to timeout/server
    // Go ahead and re-open it in one minute if possible with the last-used settings/auth
    qDebug() << " - - Will attempt to reconnect in 1 minute";
    QTimer::singleShot(60000, this, SLOT(setupSocket()) );
  }
  emit clientDisconnected();
  //Server cache is now invalid - completely lost connection
  //cPriority = -1;
  //SENT.clear(); BACK.clear(); PENDING.clear(); 
}

void freenas_client::socketSslErrors(const QList<QSslError>&errlist){ //Signal: sslErrors()
  //qWarning() << "SSL Errors Detected:" << errlist.length();
  QList<QSslError> ignored;
  for(int i=0; i< errlist.length(); i++){
    if(errlist[i].error()==QSslError::SelfSignedCertificate || errlist[i].error()==QSslError::HostNameMismatch ){
      //qDebug() << " - (IGNORED) " << errlist[i].errorString();
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

void freenas_client::socketError(QAbstractSocket::SocketError err){ //Signal:: error()
  qWarning() << "Socket Error detected:" << err;
  if(err==QAbstractSocket::SslHandshakeFailedError){qWarning() << " - SSL Handshake Failed"; }
  qWarning() << " - Final websocket error:" << SOCKET->errorString();
}
//void freenas_client::socketProxyAuthRequired(const QNetworkProxy &proxy, QAuthenticator *auth); //Signal: proxyAuthenticationRequired()

// - Main message output routine (tied to an internal signal - don't use manually)
void freenas_client::forwardSocketMessage(QString msg){
  if(SOCKET->isValid()){ 
    SOCKET->sendTextMessage(msg);
    if(pingTimer->isActive()){ pingTimer->stop(); pingTimer->start(); } //reset the timer for this interval
  }
}

// - Main message input parsing
void freenas_client::socketMessage(QString msg){ //Signal: textMessageReceived()
  //Handle this message in a separate thread
  QtConcurrent::run(this, &freenas_client::handleMessage, msg);
}

void freenas_client::handleMessage(const QString msg){
  //if(DEBUG){  qDebug() << "New Reply From Server:" << msg; }
  if(DEBUG){ qDebug() << "Got FreeNAS Message"; }
  //message_in msg_in = convertServerReply(msg);
  QJsonDocument doc = QJsonDocument::fromJson(msg.toLocal8Bit());
  if(!handleMessageInternally(doc.toObject())){
    //qDebug() << "Send out message:";
    emit newReply(doc.toObject());
  }
}
bool freenas_client::handleMessageInternally(QJsonObject data){
  return false;
}
