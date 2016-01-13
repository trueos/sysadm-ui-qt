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
}

MainUI::~MainUI(){
	
}

// === PRIVATE SLOTS ===
//UI Signals
void MainUI::on_push_auth_connect_clicked(){
  qDebug() << " UI Start Connection";
  S_CORE->openConnection(ui->line_auth_user->text(), ui->line_auth_pass->text(), ui->line_auth_host->text() );
}

//Core Signals
void MainUI::NoAuthorization(){
  qDebug() << "Lost Server authentication";
  ui->stackedWidget->setCurrentWidget(ui->page_auth);
}

void MainUI::Authorized(){
  qDebug() << "Got Server Authentication";
  ui->stackedWidget->setCurrentWidget(ui->page_main);
}