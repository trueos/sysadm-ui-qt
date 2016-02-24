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
  //Need to tinker with the toolbar a bit to get actions in the proper places
  //  -- insert a spacer so that the title/save actions are aligned right
  QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  ui->toolBar->insertWidget(ui->actionTitle, spacer);
  ui->actionTitle->setEnabled(false);
  //Now finish up the rest of the init
  CORE = core;
  InitializeUI();
  if(!CORE->isActive()){
    CORE->openConnection();
  }
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
  QString host = settings->value("Host/"+CORE->currentHost(),"").toString();
  if(host.isEmpty()){ host = CORE->currentHost(); }
  else{ host.append(" ("+CORE->currentHost()+")" ); }
  this->setWindowTitle("SysAdm: "+host );
  this->setWindowIcon( QIcon(":/icons/black/desktop.svg") );
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
  connect(ui->actionSave, SIGNAL(triggered()), page, SLOT(SaveSettings()) );
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
}

void MainUI::ShowPageTitle(QString title){
  ui->actionTitle->setText(title);
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
