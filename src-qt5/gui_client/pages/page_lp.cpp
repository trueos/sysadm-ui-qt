//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_lp.h"
#include "ui_page_lp.h" //auto-generated from the .ui file

#define TAG QString("sysadm_client_LP_")

lp_page::lp_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::lp_ui){
  ui->setupUi(this);
  connect(ui->combo_snap_pool, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSnapshotPage()) );
  connect(ui->tool_snap_remove, SIGNAL(clicked()), this, SLOT(sendSnapshotRemove()) );
  connect(ui->tool_snap_revert, SIGNAL(clicked()), this, SLOT(sendSnapshotRevert()) );
  connect(ui->tool_snap_create, SIGNAL(clicked()), this, SLOT(sendSnapshotCreate()) );
  connect(ui->push_set_save, SIGNAL(clicked()), this, SLOT(sendSaveSettings()) );

  connect(ui->tool_rep_add, SIGNAL(clicked()), this, SLOT(openNewRepInfo()) );
  connect(ui->push_rep_hidenew, SIGNAL(clicked()), this, SLOT(closeNewRepInfo()) );
  connect(ui->push_rep_savenew, SIGNAL(clicked()), this, SLOT(sendRepCreate()) );
  connect(ui->tool_rep_remove, SIGNAL(clicked()), this, SLOT(sendRepRemove()) );
  connect(ui->tool_rep_start, SIGNAL(clicked()), this, SLOT(sendRepStart()) );
  connect(ui->tool_rep_init, SIGNAL(clicked()), this, SLOT(sendRepInit()) );
  
  connect(ui->combo_rep_freq, SIGNAL(currentIndexChanged(int)), this, SLOT(new_rep_freq_changed()) );
}

lp_page::~lp_page(){
  
}

//Initialize the CORE <-->Page connections
void lp_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );

}

//Page embedded, go ahead and startup any core requests
void lp_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Life Preserver") );
  //Now run any CORE communications
  send_list_zpools();
  updateSettings();
}

// === PRIVATE ===
void lp_page::send_list_zpools(){
  QJsonObject obj;
    obj.insert("action","list_pools");
  CORE->communicate(TAG+"list_zpools", "sysadm", "zfs",obj);
}

// === PRIVATE SLOTS ===
void lp_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if( !id.startsWith(TAG) ){ return; }
  qDebug() << "Got Reply:" << id << namesp << name << args;
  if(namesp=="error" || name=="error"){ return; }

  if(id==TAG+"list_zpools"){
    zpools = args.toObject().value("list_pools").toObject().keys(); //don't care about the info for the pools, just the names
    zpools.removeAll("");
    zpools.sort();
    //Now update the UI lists
    ui->combo_snap_pool->clear();
    ui->combo_snap_pool->addItems(zpools);

  }else if(id==TAG+"list_datasets"){
    QStringList datasets = args.toObject().value("datasets").toObject().keys(); //don't care about the info for the pools, just the names
    datasets.removeAll("");
    ui->combo_snap_dataset->clear();
    ui->combo_snap_dataset->addItems(datasets);

  }else if(id==TAG+"remove_snap" || id==TAG+"create_snap"){
    updateSnapshotPage(); //need to re-load the listof snapshots

  }else if(id==TAG+"list_snaps" ){
    ui->tree_snaps->clear();
    QJsonObject data = args.toObject().value("listsnap").toObject();
    QStringList keys = data.keys();
    for(int i=0; i<keys.length(); i++){
      ui->tree_snaps->addTopLevelItem( new QTreeWidgetItem( QStringList() << keys[i].section("@",1,-1) << data.value(keys[i]).toObject().value("comment").toString() ));
    }
    ui->tree_snaps->setEnabled(true);

  }else if(id==TAG+"list_settings"){
    QJsonObject data = args.toObject().value("settings").toObject();
    ui->spin_set_diskusage->setValue( data.value("diskwarn").toString().section("%",0,0).toInt() );
    ui->line_set_email->setText( data.value("emailaddress").toString() );
    QString opt = data.value("email").toString();
    int index = ui->combo_set_emailopt->findText(opt);
   if(index>=0){ ui->combo_set_emailopt->setCurrentIndex(index); }
   else{ 
     ui->combo_set_emailopt->addItem(opt); //just in case a new option is added later - this gives us more time to update the UI
     ui->combo_set_emailopt->setCurrentIndex(ui->combo_set_emailopt->count()-1);
    }
    ui->check_set_recursive->setChecked( data.value("recursive").toString()=="ON");
  }else if(id==TAG+"save_settings"){
    updateSettings();
  }
}

//CORE Interactions (buttons usually)
// - snapshot page
void lp_page::updateSnapshotPage(){
  QString pool = ui->combo_snap_pool->currentText();
  if(pool.isEmpty()){ return; }
 //Need a couple things after the selected pool is changed
 // - get the list of datasets for this pool
  QJsonObject obj;
    obj.insert("action","datasets");
    obj.insert("zpool", pool);
  CORE->communicate(TAG+"list_datasets", "sysadm", "zfs",obj);
// - get the list of snapshots for this pool
  ui->tree_snaps->setEnabled(false);
  QJsonObject obj2;
    obj2.insert("action","listsnap");
    obj2.insert("pool", pool);
  CORE->communicate(TAG+"list_snaps", "sysadm", "lifepreserver",obj2);
}

void lp_page::sendSnapshotRevert(){
  if(ui->tree_snaps->currentItem()==0){ return; }
  QString ds = ui->combo_snap_dataset->currentText();
  QString snap = ui->tree_snaps->currentItem()->text(0);
  if(ds.isEmpty() || snap.isEmpty()){ return; }
  QJsonObject obj;
    obj.insert("action","revertsnap");
    obj.insert("dataset", ds);
    obj.insert("snap", snap);
  CORE->communicate(TAG+"revert_snap", "sysadm", "lifepreserver",obj);
}

void lp_page::sendSnapshotRemove(){
  if(ui->tree_snaps->currentItem()==0){ return; }
  QString ds, snap;
  ds = ui->combo_snap_pool->currentText(); //current zpool
  snap = ui->tree_snaps->currentItem()->text(0);
  if(ds.isEmpty() || snap.isEmpty()){ return; }
  QJsonObject obj;
    obj.insert("action","removesnap");
    obj.insert("dataset", ds);
    obj.insert("snap", snap);
  CORE->communicate(TAG+"remove_snap", "sysadm", "lifepreserver",obj);
}

void lp_page::sendSnapshotCreate(){
  QString ds = ui->combo_snap_pool->currentText();
  if(ds.isEmpty()){ return; }
  bool ok = false;
  QString snapname = QInputDialog::getText(this, tr("Create Snapshot"), tr("Name:"), QLineEdit::Normal, tr("NewSnap"), &ok );
  if(!ok || snapname.isEmpty()){ return; }
  QJsonObject obj;
    obj.insert("action","createsnap");
    obj.insert("dataset",ds);
    obj.insert("snap", snapname);
    obj.insert("comment", "GUI Snapshot");
  CORE->communicate(TAG+"create_snap", "sysadm", "lifepreserver",obj);
}
// - replication page
void lp_page::updateReplicationPage(){
  QJsonObject obj;
    obj.insert("action","listreplication");
  CORE->communicate(TAG+"list_replication", "sysadm", "lifepreserver",obj);
}

void lp_page::sendRepCreate(){
  QString host = ui->line_rep_host->text();
  QString port = QString::number(ui->spin_rep_port->value());
  QString user = ui->line_rep_user->text();
  QString pass = ui->line_rep_pass->text();
  QString ds = ui->combo_rep_localds->currentText();
  QString rds = ui->line_rep_remoteds->text();
  QString freq = ui->combo_rep_freq->currentData().toString();
  if(freq=="onhour"){ 
	freq = QString::number(ui->spin_rep_hour->value()); 
	if(freq.length()<2){ freq.prepend("0"); } //need 2-digit number
  }
  if(host.isEmpty() || user.isEmpty() || pass.isEmpty() || ds.isEmpty() || rds.isEmpty()){ return; }
  QJsonObject obj;
    obj.insert("action","addreplication");
    obj.insert("host", host);
    obj.insert("port", port);
    obj.insert("user", user);
    obj.insert("password", pass);
    obj.insert("dataset", ds);
    obj.insert("remotedataset", rds);
    obj.insert("frequency",freq);
  CORE->communicate(TAG+"add_replication", "sysadm", "lifepreserver",obj);
  closeNewRepInfo();
}

void lp_page::sendRepRemove(){
  if(ui->tree_rep->currentItem()==0){ return; }
  QJsonObject obj;
    obj.insert("action","removereplication");
    obj.insert("host", ui->tree_rep->currentItem()->text(1) );
    obj.insert("dataset", ui->tree_rep->currentItem()->text(0) );
  CORE->communicate(TAG+"remove_replication", "sysadm", "lifepreserver",obj);
}

void lp_page::sendRepStart(){
  if(ui->tree_rep->currentItem()==0){ return; }
  QJsonObject obj;
    obj.insert("action","runreplication");
    obj.insert("host", ui->tree_rep->currentItem()->text(1) );
    obj.insert("dataset", ui->tree_rep->currentItem()->text(0) );
  CORE->communicate(TAG+"start_replication", "sysadm", "lifepreserver",obj);
}

void lp_page::sendRepInit(){
  if(ui->tree_rep->currentItem()==0){ return; }
  QJsonObject obj;
    obj.insert("action","initreplication");
    obj.insert("host", ui->tree_rep->currentItem()->text(1) );
    obj.insert("dataset", ui->tree_rep->currentItem()->text(0) );
  CORE->communicate(TAG+"init_replication", "sysadm", "lifepreserver",obj);
}

void lp_page::openNewRepInfo(){
  ui->line_rep_pass->clear(); //ensure this is always cleared
  ui->group_rep_add->setVisible(true);
}

void lp_page::closeNewRepInfo(){
  ui->line_rep_pass->clear(); //ensure this is always cleared
  ui->group_rep_add->setVisible(false);
}

void lp_page::new_rep_freq_changed(){
  ui->spin_rep_hour->setVisible("onhour" == ui->combo_rep_freq->currentData().toString());
}
// - schedule page

// - settings page
void lp_page::updateSettings(){
  QJsonObject obj;
    obj.insert("action","settings");
  CORE->communicate(TAG+"list_settings", "sysadm", "lifepreserver",obj);
}

void lp_page::sendSaveSettings(){
  QJsonObject obj;
    obj.insert("action","savesettings");
    obj.insert("duwarn", QString::number(ui->spin_set_diskusage ->value()) );
    obj.insert("email", ui->line_set_email->text());
    obj.insert("emailopts", ui->combo_set_emailopt->currentText() );
    obj.insert("recursive", ui->check_set_recursive->isChecked() ? "true" : "false");
  CORE->communicate(TAG+"save_settings", "sysadm", "lifepreserver",obj);
}
