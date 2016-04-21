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
