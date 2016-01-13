//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "mainUI.h"
#include "ui_mainUI.h"

// === PUBLIC ===
MainUI::MainUI() : QMainWindow(), ui(new Ui::MainUI){
  ui->setupUi(this); //load the designer form
  connect(S_CORE, SIGNAL(clientAuthorized()), this, SLOT(Authorized()) );
  connect(S_CORE, SIGNAL(clientUnauthorized()), this, SLOT(NoAuthorization()) );
  connect(S_CORE, SIGNAL(clientDisconnected()), this, SLOT(NoAuthorization()) );
  connect(ui->line_auth_pass, SIGNAL(returnPressed()), this, SLOT(auth_connect()) );
  connect(ui->push_auth_connect, SIGNAL(clicked()), this, SLOT(auth_connect()) );
  connect(ui->actionClose_Application, SIGNAL(triggered()), this, SLOT(close()) );
  connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(auth_disconnect()) );
}

MainUI::~MainUI(){
	
}

// === PRIVATE SLOTS ===
//UI Signals
void MainUI::auth_connect(){
  qDebug() << " UI Start Connection";
  S_CORE->openConnection(ui->line_auth_user->text(), ui->line_auth_pass->text(), ui->line_auth_host->text() );
  ui->line_auth_pass->clear();
}

void MainUI::auth_disconnect(){
  qDebug() << "UI Closing Connection";
  S_CORE->closeConnection();
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