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
  connect(ui->tool_rep_modify, SIGNAL(clicked()), this, SLOT(openOldRepInfo()) );
  connect(ui->tool_rep_start, SIGNAL(clicked()), this, SLOT(sendRepStart()) );
  connect(ui->tool_rep_init, SIGNAL(clicked()), this, SLOT(sendRepInit()) );
  connect(ui->combo_rep_freq, SIGNAL(currentIndexChanged(int)), this, SLOT(new_rep_freq_changed()) );

  connect(ui->tool_schedule_addsnap, SIGNAL(clicked()), this, SLOT(showNewSchSnapInfo()) );
  connect(ui->tool_schedule_modifysnap, SIGNAL(clicked()), this, SLOT(showOldSchSnapInfo()) );
  connect(ui->tool_schedule_rmsnap, SIGNAL(clicked()), this, SLOT(removeSchSnap()) );
  connect(ui->push_schedule_savesnap, SIGNAL(clicked()), this, SLOT(saveSchSnapInfo()) );
  connect(ui->push_schedule_cancelsnap, SIGNAL(clicked()), this, SLOT(hideSchSnapInfo()) );
  connect(ui->combo_schedule_snapfreq, SIGNAL(currentIndexChanged(int)), this, SLOT(snap_sch_freq_changed()) );
  connect(ui->tool_schedule_addscrub, SIGNAL(clicked()), this, SLOT(showNewSchScrubInfo()) );
  connect(ui->tool_schedule_modifyscrub, SIGNAL(clicked()), this, SLOT(showOldSchScrubInfo()) );
  connect(ui->tool_schedule_rmscrub, SIGNAL(clicked()), this, SLOT(removeSchScrub()) );
  connect(ui->push_schedule_scrubsave, SIGNAL(clicked()), this, SLOT(saveSchScrubInfo()) );
  connect(ui->push_schedule_scrubcancel, SIGNAL(clicked()), this, SLOT(hideSchScrubInfo()) );
  connect(ui->combo_schedule_scrubfreq, SIGNAL(currentIndexChanged(int)), this, SLOT(scrub_sch_freq_changed()) );

  closeNewRepInfo(); //start with this hidden initially
  ui->group_schedule_snap->setVisible(false);
  ui->group_schedule_scrub->setVisible(false);
  //Load the possible values for the replication "frequency" combobox
  ui->combo_rep_freq->clear(); 
  ui->combo_rep_freq->addItem(tr("Sync with snapshot"),"sync");
  ui->combo_rep_freq->addItem(tr("Daily"),"onhour"); //special internal flag
  ui->combo_rep_freq->addItem(tr("Hourly"),"hour");
  ui->combo_rep_freq->addItem(tr("30 Minutes"),"30min");
  ui->combo_rep_freq->addItem(tr("10 Minutes"),"10min");
  ui->combo_rep_freq->addItem(tr("Manual Only"),"manual");
  //Now do a similar thing for the snapshot frequency combobox
  ui->combo_schedule_snapfreq->clear();
  ui->combo_schedule_snapfreq->addItem(tr("Daily"),"onhour"); //special internal flag
  ui->combo_schedule_snapfreq->addItem(tr("Hourly"),"hourly");
  ui->combo_schedule_snapfreq->addItem(tr("30 Minutes"),"30min");
  ui->combo_schedule_snapfreq->addItem(tr("10 Minutes"),"10min");
  ui->combo_schedule_snapfreq->addItem(tr("5 Minutes"),"5min");
  //And another similar thing for the scrub frequency combobox
  ui->combo_schedule_scrubfreq->clear();
  ui->combo_schedule_scrubfreq->addItem(tr("Daily"), "daily");
  ui->combo_schedule_scrubfreq->addItem(tr("Weekly"), "weekly");
  ui->combo_schedule_scrubfreq->addItem(tr("Monthly"), "monthly");
  //Now fill out the options for the days of the week
  ui->combo_schedule_scrubday->clear();
  ui->combo_schedule_scrubday->addItem(tr("Sunday"), "07");
  ui->combo_schedule_scrubday->addItem(tr("Monday"), "01");
  ui->combo_schedule_scrubday->addItem(tr("Tuesday"), "02");
  ui->combo_schedule_scrubday->addItem(tr("Wednesday"), "03");
  ui->combo_schedule_scrubday->addItem(tr("Thursday"), "04");
  ui->combo_schedule_scrubday->addItem(tr("Friday"), "05");
  ui->combo_schedule_scrubday->addItem(tr("Saturday"), "06");
}

lp_page::~lp_page(){
  
}

//Initialize the CORE <-->Page connections
void lp_page::setupCore(){

}

//Page embedded, go ahead and startup any core requests
void lp_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Life Preserver") );
  //Now run any CORE communications
  send_list_zpools();
  updateSettings();
  updateReplicationPage();
  updateSchedulePage();
}

// === PRIVATE ===
void lp_page::send_list_zpools(){
  QJsonObject obj;
    obj.insert("action","list_pools");
  communicate(TAG+"list_zpools", "sysadm", "zfs",obj);
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
    ui->combo_schedule_snappool->clear(); ui->combo_schedule_snappool->addItems(zpools);
    ui->combo_schedule_scrubpool->clear(); ui->combo_schedule_scrubpool->addItems(zpools);
    ui->combo_rep_localds->clear();
    avail_datasets.clear();
    //Now kick off the loading of all the datasets for all these pools
    for(int i=0; i<zpools.length(); i++){
      QJsonObject obj;
        obj.insert("action","datasets");
        obj.insert("zpool", zpools[i]);
      communicate(TAG+"list_dataset_"+QString::number(i+1), "sysadm", "zfs",obj);
    }
  }else if(id==TAG+"list_datasets"){
    QStringList datasets = args.toObject().value("datasets").toObject().keys(); //don't care about the info for the pools, just the names
    datasets.removeAll("");
    ui->combo_snap_dataset->clear();
    ui->combo_snap_dataset->addItems(datasets);

  }else if(id.startsWith(TAG+"list_dataset_")){
    QStringList datasets = args.toObject().value("datasets").toObject().keys(); //don't care about the info for the pools, just the names
    datasets.removeAll("");
    avail_datasets << datasets;
    ui->combo_rep_localds->addItems(datasets);

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

  }else if(id==TAG+"list_replication"){
    QJsonObject data = args.toObject().value("listreplication").toObject();
    QStringList ids = data.keys();
    qDebug() << "Replication List keys" << ids;
    ui->tree_rep->clear();
    for(int i=0; i<ids.length(); i++){
      QTreeWidgetItem *it = new QTreeWidgetItem();
      it->setText(0, data.value(ids[i]).toObject().value("dataset").toString());
      it->setText(1, data.value(ids[i]).toObject().value("host").toString());
      it->setText(2, data.value(ids[i]).toObject().value("rdset").toString());
      it->setText(3, data.value(ids[i]).toObject().value("port").toString());
      it->setText(4, data.value(ids[i]).toObject().value("frequency").toString());
      it->setText(5, data.value(ids[i]).toObject().value("user").toString());
      ui->tree_rep->addTopLevelItem(it);
    }
    ui->tab_replication->setEnabled(true);

  }else if(id==TAG+"remove_replication" || id==TAG+"add_replication"){
    updateReplicationPage();

  }else if(id==TAG+"change_schedules"){
    updateSchedulePage();

  }else if(id==TAG+"list_schedules"){
      QJsonObject data = args.toObject().value("listcron").toObject();
      QStringList pools = data.keys();
      ui->tree_schedule->clear();
	for(int i=0; i<pools.length(); i++){
        QTreeWidgetItem *it = new QTreeWidgetItem();
        it->setText(0, pools[i]);
        it->setText(1, data.value(pools[i]).toObject().value("keep").toString()); //snapshots to keep
        it->setText(2, data.value(pools[i]).toObject().value("schedule").toString()); //snapshot schedule
        it->setText(3, data.value(pools[i]).toObject().value("scrub").toString()); //scrub schedule
        ui->tree_schedule->addTopLevelItem(it);
      }

  }//end check of the "id"

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
  communicate(TAG+"list_datasets", "sysadm", "zfs",obj);
// - get the list of snapshots for this pool
  ui->tree_snaps->setEnabled(false);
  QJsonObject obj2;
    obj2.insert("action","listsnap");
    obj2.insert("pool", pool);
  communicate(TAG+"list_snaps", "sysadm", "lifepreserver",obj2);
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
  communicate(TAG+"revert_snap", "sysadm", "lifepreserver",obj);
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
  communicate(TAG+"remove_snap", "sysadm", "lifepreserver",obj);
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
  communicate(TAG+"create_snap", "sysadm", "lifepreserver",obj);
}
// - replication page
void lp_page::updateReplicationPage(){
  ui->tab_replication->setEnabled(false);
  QJsonObject obj;
    obj.insert("action","listreplication");
  communicate(TAG+"list_replication", "sysadm", "lifepreserver",obj);
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
  communicate(TAG+"add_replication", "sysadm", "lifepreserver",obj);
  closeNewRepInfo();
}

void lp_page::sendRepRemove(){
  if(ui->tree_rep->currentItem()==0){ return; }
  QJsonObject obj;
    obj.insert("action","removereplication");
    obj.insert("host", ui->tree_rep->currentItem()->text(1) );
    obj.insert("dataset", ui->tree_rep->currentItem()->text(0) );
  communicate(TAG+"remove_replication", "sysadm", "lifepreserver",obj);
}

void lp_page::sendRepStart(){
  if(ui->tree_rep->currentItem()==0){ return; }
  QJsonObject obj;
    obj.insert("action","runreplication");
    obj.insert("host", ui->tree_rep->currentItem()->text(1) );
    obj.insert("dataset", ui->tree_rep->currentItem()->text(0) );
  communicate(TAG+"start_replication", "sysadm", "lifepreserver",obj);
}

void lp_page::sendRepInit(){
  if(ui->tree_rep->currentItem()==0){ return; }
  QJsonObject obj;
    obj.insert("action","initreplication");
    obj.insert("host", ui->tree_rep->currentItem()->text(1) );
    obj.insert("dataset", ui->tree_rep->currentItem()->text(0) );
  communicate(TAG+"init_replication", "sysadm", "lifepreserver",obj);
}

void lp_page::openNewRepInfo(){
  ui->line_rep_pass->clear(); //ensure this is always cleared
  ui->line_rep_user->clear();
  ui->line_rep_host->clear();
  ui->line_rep_remoteds->clear();
  
  ui->group_rep_add->setVisible(true);
}

void lp_page::openOldRepInfo(){
  if(ui->tree_rep->currentItem()==0){ return; }
  //Load all the info from the current item into the settings
  QTreeWidgetItem *it = ui->tree_rep->currentItem();
  int index = ui->combo_rep_localds->findText( it->text(0) );
  if(index>=0){ ui->combo_rep_localds->setCurrentIndex(index); }
  ui->line_rep_host->setText( it->text(1) );
  ui->line_rep_remoteds->setText( it->text(2) );
  ui->spin_rep_port->setValue( it->text(3).toInt() );
  index = ui->combo_rep_freq->findData( it->text(4) );
  if(index<0){
    //daily - has the time code instead
    index = ui->combo_rep_freq->findData("onhour");
    ui->spin_rep_hour->setValue( it->text(4).toInt() );
  }
  ui->combo_rep_freq->setCurrentIndex(index);
  ui->line_rep_user->setText( it->text(5) );
  //Now show the settings
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
void lp_page::updateSchedulePage(){
  QJsonObject obj;
    obj.insert("action","listcron");
  communicate(TAG+"list_schedules", "sysadm", "lifepreserver",obj);
}

void lp_page::showNewSchSnapInfo(){
  ui->group_schedule_snap->setVisible(true);
  ui->group_schedule_scrub->setVisible(false);
}

void lp_page:: showOldSchSnapInfo(){
  if(ui->tree_schedule->currentItem()==0){ return; }
  QTreeWidgetItem *it = ui->tree_schedule->currentItem();
  int index = ui->combo_schedule_snappool->findText(it->text(0));
  if(index>=0){ ui->combo_schedule_snappool->setCurrentIndex(index); }
  ui->spin_schedule_snapkeep->setValue( it->text(1).toInt() );
  index = ui->combo_schedule_snapfreq->findData( it->text(2) );
  if(index<0){ 
    index = ui->combo_schedule_snapfreq->findData( "onhour" ); 
    ui->spin_schedule_snaphour->setValue( it->text(2).section("@",1,-1).toInt() );
  }
  ui->combo_schedule_snapfreq->setCurrentIndex(index);
  ui->group_schedule_snap->setVisible(true);
  ui->group_schedule_scrub->setVisible(false);
}

void lp_page::removeSchSnap(){
  if(ui->tree_schedule->currentItem()==0){ return; }
  QTreeWidgetItem *it = ui->tree_schedule->currentItem();
  QJsonObject obj;
    obj.insert("action","cronsnap");
    obj.insert("pool", it->text(0));
    obj.insert("keep", "0");
    obj.insert("frequency", "none"); //disables the schedule
  communicate(TAG+"change_schedules", "sysadm", "lifepreserver",obj);
}

void lp_page::saveSchSnapInfo(){
  QString freq = ui->combo_schedule_snapfreq->currentData().toString();
  if(freq == "onhour"){ 
    freq = QString::number(ui->spin_schedule_snaphour->value());
    if(freq.length()==1){ freq.prepend("0"); }
    freq.prepend("daily@");
  }
  QJsonObject obj;
    obj.insert("action","cronsnap");
    obj.insert("pool", ui->combo_schedule_snappool->currentText() );
    obj.insert("keep", QString::number(ui->spin_schedule_snapkeep->value()) );
    obj.insert("frequency", freq);
  qDebug() << "SEND:" << obj;
  communicate(TAG+"change_schedules", "sysadm", "lifepreserver",obj);
  ui->group_schedule_snap->setVisible(false);
}

void lp_page::hideSchSnapInfo(){
  ui->group_schedule_snap->setVisible(false);
}

void lp_page::snap_sch_freq_changed(){
  ui->spin_schedule_snaphour->setVisible( ui->combo_schedule_snapfreq->currentData().toString()=="onhour");
}

void lp_page::showNewSchScrubInfo(){
  ui->group_schedule_scrub->setVisible(true);
  ui->group_schedule_snap->setVisible(false);
}

void lp_page::showOldSchScrubInfo(){
  if(ui->tree_schedule->currentItem()==0){ return; }
  QTreeWidgetItem *it = ui->tree_schedule->currentItem();
  int index = ui->combo_schedule_scrubpool->findText(it->text(0));
  if(index>=0){ ui->combo_schedule_scrubpool->setCurrentIndex(index); }
  QString freq = it->text(2);
  index = ui->combo_schedule_scrubfreq->findData( freq.section("@",0,0) );
  if(index>=0){ ui->combo_schedule_scrubfreq->setCurrentIndex(index); }
  if(freq.startsWith("daily@")){ 
    ui->spin_schedule_scrubhour->setValue(freq.section("@",1,-1).toInt());
  }else if(freq.startsWith("weekly@")){
    ui->combo_schedule_scrubday->setCurrentIndex( freq.section("@",1,1).toInt()-1 );
    ui->spin_schedule_scrubhour->setValue(freq.section("@",2,-1).toInt());
  }else if(freq.startsWith("monthly@")){
    ui->spin_schedule_scrubdate->setValue(freq.section("@",1,1).toInt());
    ui->spin_schedule_scrubhour->setValue(freq.section("@",2,-1).toInt());
  }
  ui->group_schedule_scrub->setVisible(true);
  ui->group_schedule_snap->setVisible(false);
}

void lp_page::removeSchScrub(){
  if(ui->tree_schedule->currentItem()==0){ return; }
  QTreeWidgetItem *it = ui->tree_schedule->currentItem();
  QJsonObject obj;
    obj.insert("action","cronscrub");
    obj.insert("pool", it->text(0));
    obj.insert("frequency", "none"); //disables the schedule
  communicate(TAG+"change_schedules", "sysadm", "lifepreserver",obj);
}

void lp_page::saveSchScrubInfo(){
  QString freq = ui->combo_schedule_scrubfreq->currentData().toString();
  if(freq=="daily"){ 
    QString tmp = ui->spin_schedule_scrubhour->cleanText();
    if(tmp.length()==1){ tmp.prepend("0"); }
    freq.append("@"+tmp);
  }else if(freq=="weekly"){
    QString tmp = ui->combo_schedule_scrubday->currentData().toString();
    if(tmp.length()==1){ tmp.prepend("0"); }
    freq.append("@"+tmp);
    tmp = ui->spin_schedule_scrubhour->cleanText();
    if(tmp.length()==1){ tmp.prepend("0"); }
    freq.append("@"+tmp);
  }else if(freq=="monthly"){ 
    QString tmp = ui->spin_schedule_scrubdate->cleanText();
    if(tmp.length()==1){ tmp.prepend("0"); }
    freq.append("@"+tmp);
    tmp = ui->spin_schedule_scrubhour->cleanText();
    if(tmp.length()==1){ tmp.prepend("0"); }
    freq.append("@"+tmp);
  }

  QJsonObject obj;
    obj.insert("action","cronscrub");
    obj.insert("pool", ui->combo_schedule_scrubpool->currentText());
    obj.insert("frequency", freq);
  communicate(TAG+"change_schedules", "sysadm", "lifepreserver",obj);
  ui->group_schedule_scrub->setVisible(false);
}

void lp_page::hideSchScrubInfo(){
  ui->group_schedule_scrub->setVisible(false);
}

void lp_page::scrub_sch_freq_changed(){
  QString freq = ui->combo_schedule_scrubfreq->currentData().toString();
  if(freq=="daily"){
    ui->spin_schedule_scrubdate->setVisible(false);
    ui->spin_schedule_scrubhour->setVisible(true);
    ui->combo_schedule_scrubday->setVisible(false);
  }else if(freq=="weekly"){
    ui->spin_schedule_scrubdate->setVisible(false);
    ui->spin_schedule_scrubhour->setVisible(true);
    ui->combo_schedule_scrubday->setVisible(true);
  }else if(freq=="monthly"){
    ui->spin_schedule_scrubdate->setVisible(true);
    ui->spin_schedule_scrubhour->setVisible(true);
    ui->combo_schedule_scrubday->setVisible(false);
  }
}

// - settings page
void lp_page::updateSettings(){
  QJsonObject obj;
    obj.insert("action","settings");
  communicate(TAG+"list_settings", "sysadm", "lifepreserver",obj);
}

void lp_page::sendSaveSettings(){
  QJsonObject obj;
    obj.insert("action","savesettings");
    obj.insert("duwarn", QString::number(ui->spin_set_diskusage ->value()) );
    obj.insert("email", ui->line_set_email->text());
    obj.insert("emailopts", ui->combo_set_emailopt->currentText() );
    obj.insert("recursive", ui->check_set_recursive->isChecked() ? "true" : "false");
  communicate(TAG+"save_settings", "sysadm", "lifepreserver",obj);
}
