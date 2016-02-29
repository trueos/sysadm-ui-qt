//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "NewConnectionWizard.h"
#include "ui_NewConnectionWizard.h"

NewConnectionWizard::NewConnectionWizard(QWidget *parent, QString nickname) : QDialog(parent), ui(new Ui::NewConnectionWizard){
  ui->setupUi(this);
  nick = nickname;
  core = new sysadm_client();
  success = false;
  ui->push_finished->setVisible(false);
  ui->label_results->setVisible(false);
  //Setup connections
  connect(core, SIGNAL(clientConnected()), this, SLOT(coreConnected()) );
  connect(core, SIGNAL(clientDisconnected()), this, SLOT(coreDisconnected()) );
  connect(core, SIGNAL(clientAuthorized()), this, SLOT(coreAuthenticated()) );
  //Line edit connections
  connect(ui->line_host, SIGNAL(textEdited(const QString&)), this, SLOT(checkInputs()) );
  connect(ui->line_user, SIGNAL(textEdited(const QString&)), this, SLOT(checkInputs()) );
  connect(ui->line_pass, SIGNAL(textEdited(const QString&)), this, SLOT(checkInputs()) );
}

NewConnectionWizard::~NewConnectionWizard(){
	
}

void NewConnectionWizard::LoadPrevious(QString host, QString user){
  ui->line_host->setText(host);
  ui->line_host->setEnabled(false); //when loading a previous connection - don't allow changing the host
  ui->line_user->setText(user);
}

// === PRIVATE SLOTS ===
void NewConnectionWizard::checkInputs(){
  ui->push_start_test->setEnabled( !ui->line_host->text().isEmpty() && !ui->line_user->text().isEmpty() && !ui->line_pass->text().isEmpty() );
}

//core signals/slots
void NewConnectionWizard::coreConnected(){
  ui->label_results->setText( tr("Host Valid") );
}
void NewConnectionWizard::coreAuthenticated(){
  ui->label_results->setText( tr("Test Successful") );
  ui->label_results->setVisible(true);
  ui->push_cancel->setVisible(false);
  ui->push_finished->setVisible(true);
  //Now do any post-auth first-time setup
  success = true;
  host = core->currentHost();
  //Save the good info to the settings file
  settings->setValue("Hosts/"+host, nick); //save the nickname
  settings->setValue("Hosts/"+host+"/username", ui->line_user->text());
	
  //Clean up any core interactions (but leave it running for later)
  
  disconnect(core, 0, this, 0);
}

void NewConnectionWizard::coreDisconnected(){
  //Test was a failure - leave the results label visible so the user can see how far it got
  ui->group_host->setEnabled(true);
  ui->label_results->setVisible(true);
  ui->push_cancel->setVisible(true);
  ui->push_finished->setVisible(false);
}

//Buttons
void NewConnectionWizard::on_push_start_test_clicked(){
  ui->group_host->setEnabled(false); //de-activate for the moment
  ui->label_results->setText( tr("Host Invalid") );
  core->openConnection(ui->line_user->text(), ui->line_pass->text(), ui->line_host->text());
}

void NewConnectionWizard::on_push_finished_clicked(){
  this->close();
}

void NewConnectionWizard::on_push_cancel_clicked(){
  this->close();
}
