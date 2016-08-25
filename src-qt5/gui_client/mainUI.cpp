//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "mainUI.h"
#include "ui_mainUI.h"

#include "pages/getPage.h"

// === PUBLIC ===
MainUI::MainUI(sysadm_client *core, QString pageID, QString bridgeID) : QMainWindow(), ui(new Ui::MainUI){
  CORE = core;
  host = core->currentHost();
  b_id = bridgeID;
  if(b_id.isEmpty()){ nickname = host; }
  else{ nickname = CORE->bridgedHostname(b_id); } //only use the first part of the name (rest is a UUID)

  //Setup the CORE connections
  connect(this, SIGNAL(send_client_message(QString, QJsonObject)), CORE, SLOT(communicate_bridge(QString, QJsonObject)) );

  ui->setupUi(this); //load the designer form
  //Need to tinker with the toolbar a bit to get actions in the proper places
  QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  ui->toolBar->insertWidget(ui->actionPower, spacer);
  ui->actionTitle->setEnabled(false);
  //Create the shortcut for closing the window
  s_quit = new QShortcut(QKeySequence(Qt::Key_Escape), this);
  connect(s_quit, SIGNAL(activated()), this, SLOT(close()) );
  s_closewin = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_W), this);
  connect(s_closewin, SIGNAL(activated()), this, SLOT(close()) );
  //Create the menu of power options for the server/connection
  if(!CORE->isLocalHost()){
    ui->actionPower->setMenu(new QMenu(this));
    ui->actionPower->menu()->addAction(QIcon(":/icons/black/eject.svg"), tr("Disconnect From System"), this, SLOT(ServerDisconnect())	);
    ui->actionPower->menu()->addSeparator();
    ui->actionPower->menu()->addAction(QIcon(":/icons/black/sync-circled.svg"), tr("Reboot System"), this, SLOT(ServerReboot()) );
    ui->actionPower->menu()->addAction(QIcon(":/icons/black/circled-off.svg"), tr("Shutdown System"), this, SLOT(ServerShutdown()) );
    connect(ui->actionPower, SIGNAL(triggered()), this, SLOT(ShowPowerPopup()) );
  }else{
    ui->toolBar->removeAction(ui->actionPower);
  }
  //Now finish up the rest of the init
  qDebug() << "Init UI";
  InitializeUI();
  currentPage = pageID;
  if(currentPage.isEmpty()){ currentPage = "somedummy_page"; }
  if(!CORE->isActive()){
    qDebug() << " - CORE not active";
    if(CORE->needsBaseAuth() && !CORE->isLocalHost()){
      QMessageBox *dlg = new QMessageBox(QMessageBox::Warning, tr("Authentication Settings Invalid"), tr("Please reset your authentication procedures for this server within the connection manager."),QMessageBox::Ok, this);
      dlg->setModal(true);
      connect(dlg, SIGNAL(finished(int)), this, SLOT(close()) );
      dlg->show();
    }else{
      qDebug() << " - open connection";
      CORE->openConnection();
    }
  }
  if( CORE->isReady() ){
    qDebug() << " - Core is ready:" << CORE->isBridge() << b_id;
    if(CORE->isBridge() && b_id.isEmpty()){ QTimer::singleShot(5,this, SLOT(close()) ); }
    else{ loadPage(pageID); }
  }else if(pageID.isEmpty()){
    QTimer::singleShot(500, this, SLOT(loadPage()) ); //1/2 second wait max for a connection
  }
}

MainUI::~MainUI(){
}

sysadm_client* MainUI::currentCore(){
  return CORE;
}

QString MainUI::currentHost(){
  if(b_id.isEmpty()){ return host; }
  else{ return host+"/"+b_id; }
}

// === PRIVATE ===
void MainUI::InitializeUI(){
  //Now setup the signals/slots
  connect(CORE, SIGNAL(clientAuthorized()), this, SLOT(Authorized()) );
  connect(CORE, SIGNAL(clientUnauthorized()), this, SLOT(NoAuthorization()) );
  connect(CORE, SIGNAL(clientDisconnected()), this, SLOT(Disconnected()) );
  connect(CORE, SIGNAL(newReply(QString,QString,QString,QJsonValue)), this, SLOT(newReply(QString,QString,QString,QJsonValue)) );
  connect(CORE, SIGNAL(bridgeReply(QString,QString,QString,QString,QJsonValue)), this, SLOT(bridgeReply(QString,QString,QString,QString,QJsonValue)) );
  connect(CORE, SIGNAL(NewEvent(sysadm_client::EVENT_TYPE, QJsonValue)), this, SLOT(newEvent(sysadm_client::EVENT_TYPE, QJsonValue)) );
  connect(CORE, SIGNAL(bridgeEvent(QString, sysadm_client::EVENT_TYPE, QJsonValue)), this, SLOT(bridgeEvent(QString, sysadm_client::EVENT_TYPE, QJsonValue)) );

  connect(ui->actionBack, SIGNAL(triggered()), this, SLOT(loadPage()) );
  connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(SavePage()) );
	
  //Now set the initial window size based on the last saved setting
  QSize orig = settings->value("preferences/MainWindowSize", QSize()).toSize();
  if(!orig.isEmpty() && orig.isValid()){
    //Make sure the old size is larger than the default size hint
    if(orig.width() < this->sizeHint().width()){ orig.setWidth(this->sizeHint().width()); }
    if(orig.height() < this->sizeHint().height()){ orig.setHeight(this->sizeHint().height()); }    
    //Also ensure the old size is smaller than the current screen size
    QSize screen = QApplication::desktop()->availableGeometry(this).size();
    if(orig.width() > screen.width()){ orig.setWidth(screen.width()); }
    if(orig.height() > screen.height()){ orig.setHeight(screen.height()); }
    //Now resize the window
    this->resize(orig);
  }
  
  //Now setup the window title/icon
  host = settings->value("Hosts/"+CORE->currentHost(),"").toString();
  if(host.isEmpty()){ 
    if(CORE->isLocalHost()){ host = tr("Local System"); }
    else{ host = CORE->currentHost(); }
  }else{ host.append(" ("+CORE->currentHost()+")" ); }
  this->setWindowTitle("SysAdm: "+host );
  this->setWindowIcon( QIcon(":/icons/custom/sysadm.svg") );
}

// === PRIVATE SLOTS ===
void MainUI::ShowPowerPopup(){
  ui->actionPower->menu()->popup(this->mapToGlobal(ui->toolBar->widgetForAction(ui->actionPower)->geometry().bottomLeft()) );
}
void MainUI::ServerDisconnect(){
  qDebug() << "- User Request Disconnect:" << CORE->currentHost();
  CORE->closeConnection();
}
void MainUI::ServerReboot(){
  qDebug() << "- User Request Reboot:" << CORE->currentHost();
  QJsonObject obj;
    obj.insert("action","reboot");
  CORE->communicate("client_power_action","sysadm","systemmanager",obj);
}
void MainUI::ServerShutdown(){
  qDebug() << "- User Request Shutdown:" << CORE->currentHost();
  QJsonObject obj;
    obj.insert("action","halt");
  CORE->communicate("client_power_action","sysadm","systemmanager",obj);
}

//Page Management
void MainUI::loadPage(QString id){
  //qDebug() << "Load Page:" << id;
  QString lastPage = this->centralWidget()->whatsThis();
  PageWidget *page = GetNewPage(id, this, CORE);
  //qDebug() << "Got Page:" << page;
  if(page==0){ return; }
  page->setObjectName(id);
  page->setPreviousPage(lastPage);
  currentPage = page->whatsThis();
  //Connect Page
  connect(page, SIGNAL(HasPendingChanges()), this, SLOT(ShowSaveButton()) );
  connect(page, SIGNAL(ChangePageTitle(QString)), this, SLOT(ShowPageTitle(QString)) );
  connect(page, SIGNAL(ChangePage(QString)), this, SLOT(loadPage(QString)) );
  connect(ui->actionSave, SIGNAL(triggered()), page, SLOT(SaveSettings()) );
  connect(page, SIGNAL(send_message(QJsonObject)), this, SLOT(send_message(QJsonObject)) );
  //Switch page in window
  //qDebug() << " - Swap page in window";
  QWidget *old = this->centralWidget();
  this->setCentralWidget(page);
  ui->actionBack->setVisible(!id.isEmpty());
  ui->actionSave->setVisible(false);
  ui->actionSave->setEnabled(false);
  ui->actionTitle->setText("");
  if(old!=0 && old!=ui->centralwidget){ old->disconnect(); old->deleteLater(); }
  //Now run the page startup routines
  //qDebug() << " - Setup Core";
  page->setupCore();
  //qDebug() << " - Start Page";
  page->startPage();
  //qDebug() << " - Give Page Focus";
  page->setFocus();
  this->showNormal();
}

void MainUI::ShowPageTitle(QString title){
  ui->actionTitle->setText(title);
  this->setWindowTitle("SysAdm: "+title+" ("+nickname+")" );
}

void MainUI::ShowSaveButton(){
  ui->actionSave->setEnabled(true);
  ui->actionSave->setVisible(true);
}

void MainUI::SavePage(){
  //This signal is overloaded so the page gets it directly - just hide/disable the button here
  ui->actionSave->setEnabled(false);
  ui->actionSave->setVisible(false);
}

void MainUI::send_message(QJsonObject msg){
  //qDebug() << "Send Message:" << b_id << msg;
  this->emit send_client_message(b_id, msg); //include the bridge ID (if there is one)
}

//Core Signals
void MainUI::NoAuthorization(){
  qDebug() << "Lost Server authentication";
  //ui->stackedWidget->setCurrentWidget(ui->page_auth);
  //ui->line_auth_user->clear();
}

void MainUI::Authorized(){
  qDebug() << "Got Server Authentication";
  if(CORE->isBridge() && b_id.isEmpty()){ this->close(); }
  else{ loadPage( currentPage ); }
}

void MainUI::Disconnected(){
  qDebug() << "Core Disconnected";
  this->close();
}

//Main message signals from core
void MainUI::newReply(QString id, QString namesp, QString name, QJsonValue args){
  //qDebug() << "Normal Reply:" << b_id << id;
  if(!b_id.isEmpty()){ return; } //this reply not for this window
  static_cast<PageWidget*>(this->centralWidget())->ParseReply(id, namesp, name, args);
}

void MainUI::bridgeReply(QString bridge_id,QString id, QString namesp, QString name, QJsonValue args){
  //qDebug() << "New Bridge Reply:" << bridge_id << b_id << id;
  if(b_id != bridge_id){ return; } //this reply not for this window
  static_cast<PageWidget*>(this->centralWidget())->ParseReply(id, namesp, name, args);
}

void MainUI::newEvent(sysadm_client::EVENT_TYPE type, QJsonValue val){
  if(!b_id.isEmpty()){ return; } //this reply not for this window
  static_cast<PageWidget*>(this->centralWidget())->ParseEvent(type, val);
}

void MainUI::bridgeEvent(QString bridge_id, sysadm_client::EVENT_TYPE type, QJsonValue val){
  if(b_id != bridge_id){ return; } //this reply not for this window
  static_cast<PageWidget*>(this->centralWidget())->ParseEvent(type, val);
}
