//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_CLASS_H
#define _PCBSD_SYSADM_CLIENT_CLASS_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QWebSocket>
#include <QObject>
#include <QSettings>
#include <QSslError>
#include <QHash>
#include <QDebug>
#include <QTimer>

//NOTE: The default port will be used unless the host IP has ":<port>" appended on the end
//NOTE-2: The host IP can be either a number (127.0.0.1) or a URL address-only  (mysystem.net)
#define WSPORTDEFAULT 12150
#define BRIDGEPORTDEFAULT 12149

//Number of automatic re-connects to try before failing out
#define FAIL_MAX 10

struct message_in{
  QString from_bridge_id;
  QString name, namesp, id;
  QJsonValue args;
};

struct bridge_data{
  QString enc_key; //current encryption key to use
  QString auth_tok; //current authentication token
};

class sysadm_client : public QObject{
	Q_OBJECT
public:
	enum EVENT_TYPE{ DISPATCHER, LIFEPRESERVER, SYSSTATE};
	
	sysadm_client();
	~sysadm_client();

	// Overall Connection functions (start/stop)
	void openConnection(QString user, QString pass, QString hostIP);
	void openConnection(QString authkey, QString hostIP);
	void openConnection(QString hostIP); //uses SSL auth if possible
	void openConnection(); //uses last valid auth settings
	void closeConnection();
	
	QString currentHost();
	bool isActive();
	bool isLocalHost(); //special case, checks currentHost for the localhost definitions
	bool needsBaseAuth(); //returns if a base user/password auth is required(always true until an SSL auth is attempted)
	bool isReady(); //returns true if the connection is all set and ready for inputs (auth successful, etc).
	bool isConnecting(); //returns true if it is currently trying to establish a connection
	
	//Bridged Connection Functions
	bool isBridge();
	QStringList bridgeConnections(); //list the known connections through the bridge	

	//Check if the sysadm server is running on the local system
	static bool localhostAvailable();

	// Register for Event Notifications (no notifications by default)
	void registerForEvents(EVENT_TYPE event, bool receive = true);
	//Return the current priority for the system state (0-9)
	int statePriority();
	//Register the custom SSL Certificate with the server
	void registerCustomCert();
	
	// Messages which are still pending a response
	QStringList pending(); //returns only the "id" for each 
	
	// Fetch a message from the recent cache
	QJsonObject cachedRequest(QString id);
	QJsonValue cachedReply(QString id);
	
private:
	QWebSocket *SOCKET;
	QString chost, cauthkey, cuser, cpass; //current host/authkey/user/pass
	QList<EVENT_TYPE> events;
	QHash<QString, QJsonObject> SENT, BACK; //small cache of sent/received messages
	QStringList PENDING; //ID's for sent but not received messages
	bool keepActive, SSLsuccess, usedSSL;
	int num_fail; //number of server connection failures
	int cPriority;
	bool isbridge;
	QHash<QString, bridge_data> BRIDGE;
	QTimer *connectTimer, *pingTimer;

	//Functions to do the initial socket setup
	void performAuth(QString user="", QString pass=""); //uses cauthkey if empty inputs
	void performAuth_bridge(QString bridge_id); //SSL only
	void clearAuth();

	//Communication subroutines with the server
	void sendEventSubscription(EVENT_TYPE event, bool subscribe = true);
	void sendEventSubscription_bridge(QString bridge_id, EVENT_TYPE event, bool subscribe = true);
	void sendSocketMessage(QJsonObject msg);
	void sendSocketMessage(QString msg);

	//Simplification functions
	message_in convertServerReply(QString reply);
	QString SSL_Encode_String(QString str);
	QString EncodeString(QString str, QString key);
	QString DecodeString(QString str, QString key);

public slots:
	// Overloaded Communication functions
	// Reply from server may be obtained later from the newReply() signal
	void communicate(QString ID, QString namesp, QString name, QJsonValue args);
	void communicate(QJsonObject);
	void communicate(QList<QJsonObject>);
	//Overloaded Bridge Communication functions
	void communicate_bridge(QString bridge_host_id, QString ID, QString namesp, QString name, QJsonValue args);
	void communicate_bridge(QString bridge_host_id, QJsonObject);
	void communicate_bridge(QString bridge_host_id, QList<QJsonObject>);

private slots:
        void setupSocket(); //uses chost/cport for setup
	void sendPing(); //Used to keep a connection active at regular intervals

	//Socket signal/slot connections
	void socketConnected(); //Signal: connected()
	void socketClosed(); //Signal: disconnected()
	void socketSslErrors(const QList<QSslError>&errlist); //Signal: sslErrors()
	void socketError(QAbstractSocket::SocketError err); //Signal:: error()
	//void socketProxyAuthRequired(const QNetworkProxy &proxy, QAuthenticator *auth); //Signal: proxyAuthenticationRequired()

	// - Main message input parsing
	void socketMessage(QString msg); //Signal: textMessageReceived()
	bool handleMessageInternally(message_in msg);

signals:
	void clientConnected(); //Stage 1 - host address is valid
	void clientAuthorized(); //Stage 2 - user is authorized to continue
	void clientReconnecting(); //emitted periodically while attempting to establish a connection to the server
	void clientDisconnected(); //Only emitted if the client could not automatically reconnect to the server
	void clientUnauthorized(); //Only emitted if the user needs to re-authenticate with the server
	void bridgeConnectionsChanged(QStringList); //list of server connections now available
	void bridgeAuthorized(QString);
	void newReply(QString ID, QString namesp, QString name, QJsonValue args);
	void bridgeReply(QString bridgeID, QString ID, QString namesp, QString name, QJsonValue args);
	void NewEvent(sysadm_client::EVENT_TYPE, QJsonValue);
	void statePriorityChanged(int);

};

#endif
