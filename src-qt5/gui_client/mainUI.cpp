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
MainUI::MainUI(sysadm_client *core) : QMainWindow(), ui(new Ui::MainUI){
  ui->setupUi(this); //load the designer form
  CORE = core;
  InitializeUI();
  loadPage();
}

MainUI::~MainUI(){
}

sysadm_client* MainUI::currentCore(){
  return CORE;
}

// === PRIVATE ===
void MainUI::InitializeUI(){
  //Now setup the signals/slots
  connect(CORE, SIGNAL(clientAuthorized()), this, SLOT(Authorized()) );
  connect(CORE, SIGNAL(clientUnauthorized()), this, SLOT(NoAuthorization()) );
  connect(CORE, SIGNAL(clientDisconnected()), this, SLOT(Disconnected()) );

  connect(ui->actionClose_Application, SIGNAL(triggered()), this, SLOT(close()) );
}

// === PRIVATE SLOTS ===

//Page Management
void MainUI::loadPage(QString id){
  qDebug() << "Load Page:" << id;
  PageWidget *page = GetNewPage(id, this, CORE);
  if(page==0){ return; }
  //Connect Page
  connect(page, SIGNAL(HasPendingChanges()), this, SLOT(ShowSaveButton()) );
  connect(page, SIGNAL(ChangePageTitle(QString)), this, SLOT(ShowPageTitle(QString)) );
  connect(page, SIGNAL(ChangePage(QString)), this, SLOT(loadPage(QString)) );
  //Switch page in window
  //qDebug() << " - Swap page in window";
  QWidget *old = this->centralWidget();
  this->setCentralWidget(page);
  if(old!=0 && old!=ui->centralwidget){ old->disconnect(); old->deleteLater(); }
  //Now run the page startup routines
  //qDebug() << " - Setup Core";
  page->setupCore();
  //qDebug() << " - Start Page";
  page->startPage();
  //qDebug() << " - Give Page Focus";
  page->setFocus();
}

//Core Signals
void MainUI::NoAuthorization(){
  qDebug() << "Lost Server authentication";
  //ui->stackedWidget->setCurrentWidget(ui->page_auth);
  //ui->line_auth_user->clear();
}

void MainUI::Authorized(){
  qDebug() << "Got Server Authentication";
  loadPage();
}

void MainUI::Disconnected(){
  qDebug() << "Core Disconnected";
  this->close();
}
