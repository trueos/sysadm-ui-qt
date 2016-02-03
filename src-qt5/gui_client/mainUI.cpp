//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "mainUI.h"
#include "ui_mainUI.h"


// === PUBLIC ===
MainUI::MainUI(sysadm_client *core) : QMainWindow(), ui(new Ui::MainUI){
  ui->setupUi(this); //load the designer form
  CORE = core;
  InitializeUI();
  if(CORE->currentHost().isEmpty()){ NoAuthorization(); }
  else{ Authorized(); }
}

MainUI::~MainUI(){
}

sysadm_client* MainUI::currentCore(){
  return CORE;
}

// === PRIVATE ===
void MainUI::InitializeUI(){
  //First load any pre-existing settings
  bool haslocalhost = sysadm_client::localhostAvailable();
  ui->actionLocalhost_Auto_Connect->setVisible(haslocalhost);
  ui->actionLocalhost_Auto_Connect->setEnabled(haslocalhost);
  ui->actionLocalhost_Auto_Connect->setChecked(false);
  if(haslocalhost){ 
    ui->actionLocalhost_Auto_Connect->setChecked(settings->value("auto-auth-localhost",true).toBool()); 
    ui->line_auth_host->setText("localhost");
    if(!ui->line_auth_user->text().isEmpty()){ ui->line_auth_pass->setFocus();  }
    else{ ui->line_auth_user->setFocus(); }
  }else{
    ui->line_auth_host->setFocus();
  }
  
  //Now setup the signals/slots
  connect(CORE, SIGNAL(clientAuthorized()), this, SLOT(Authorized()) );
  connect(CORE, SIGNAL(clientUnauthorized()), this, SLOT(NoAuthorization()) );
  connect(CORE, SIGNAL(clientDisconnected()), this, SLOT(NoAuthorization()) );
  connect(CORE, SIGNAL(newReply(QString,QString,QString,QJsonValue)), this, SLOT( NewMessage(QString,QString,QString,QJsonValue)) );
  connect(ui->line_auth_pass, SIGNAL(returnPressed()), this, SLOT(auth_connect()) );
  connect(ui->push_auth_connect, SIGNAL(clicked()), this, SLOT(auth_connect()) );
  connect(ui->actionClose_Application, SIGNAL(triggered()), this, SLOT(close()) );
  connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(auth_disconnect()) );	
  connect(ui->actionLocalhost_Auto_Connect, SIGNAL(triggered()), this, SLOT(auto_local_auth_changed()) );
}

// === PRIVATE SLOTS ===
//UI Signals
void MainUI::auth_connect(){
  qDebug() << " UI Start Connection";
  CORE->openConnection(ui->line_auth_user->text(), ui->line_auth_pass->text(), ui->line_auth_host->text() );
  ui->line_auth_pass->clear();
}

void MainUI::auth_disconnect(){
  qDebug() << "UI Closing Connection";
  CORE->closeConnection();
}

void MainUI::auto_local_auth_changed(){
  settings->setValue("auto-auth-localhost",  ui->actionLocalhost_Auto_Connect->isChecked());
}

// Temporary Test Functions
void MainUI::on_push_tmp_sendmsg_clicked(){
  //QString args = QJsonDocument::fromJson(ui->line_tmp_json->text()).object();
  QJsonValue args;
  QString txt = ui->line_tmp_json->text();
  if(txt.startsWith("{")){ args = QJsonDocument::fromJson(txt.toUtf8()).object(); }
  else if(txt.startsWith("[")){ args = QJsonDocument::fromJson(txt.toUtf8()).array(); }
  else{ args = QJsonValue(txt); }
  CORE->communicate("sampleID", ui->line_tmp_namesp->text(), ui->line_tmp_name->text(), args );
}

void MainUI::NewMessage(QString id, QString ns, QString nm, QJsonValue args){
  qDebug() << "New Message:" << id << ns << nm << args;
  ui->label_tmp_reply->setText(QJsonDocument(args.toObject()).toJson() );
}

//Core Signals
void MainUI::NoAuthorization(){
  qDebug() << "Lost Server authentication";
  ui->stackedWidget->setCurrentWidget(ui->page_auth);
  ui->line_auth_user->clear();
}

void MainUI::Authorized(){
  qDebug() << "Got Server Authentication";
  ui->stackedWidget->setCurrentWidget(ui->page_main);
}
