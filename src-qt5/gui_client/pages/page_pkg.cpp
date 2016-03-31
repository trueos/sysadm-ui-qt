//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_pkg.h"
#include "ui_page_pkg.h" //auto-generated from the .ui file

#define TAG QString("sysadm_pkg_auto_")

pkg_page::pkg_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::pkg_page_ui){
  ui->setupUi(this);
  local_showall = local_advmode = local_hasupdates = false;
  local_viewM = new QMenu(this);
    ui->tool_local_filter->setMenu(local_viewM);
    //Now populate the filter menu
    QAction *tmp = local_viewM->addAction(tr("View All Packages"));
      tmp->setCheckable(true);
      tmp->setChecked(local_showall);
      connect(tmp, SIGNAL(toggled(bool)), this, SLOT(update_local_viewall(bool)) );
    tmp = local_viewM->addAction(tr("View Advanced Options"));
      tmp->setCheckable(true);
      tmp->setChecked(local_showall);
      connect(tmp, SIGNAL(toggled(bool)), this, SLOT(update_local_viewadv(bool)) );
  //Change any flags as needed
  ui->tree_local->setMouseTracking(true); //allow hover-over events (status tip)
  int lineheight = ui->tree_local->fontMetrics().lineSpacing();
  ui->tree_local->setIconSize( QSize(lineheight*3, lineheight) );
  //Setup the GUI connections
  connect(ui->check_local_all, SIGNAL(toggled(bool)), this, SLOT(update_local_pkg_check(bool)) );
  connect(ui->tree_local, SIGNAL(itemSelectionChanged()), this, SLOT(update_local_buttons()) );
  connect(ui->tree_local, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(update_local_buttons()) );
  connect(ui->tree_local, SIGNAL(itemActivated(QTreeWidgetItem*, int)), this, SLOT(goto_browser_from_local(QTreeWidgetItem*)) );
  connect(ui->tool_local_rem, SIGNAL(clicked()), this, SLOT(send_local_rmpkgs()) );
  connect(ui->tool_local_lock, SIGNAL(clicked()), this, SLOT(send_local_lockpkgs()) );
  connect(ui->tool_local_unlock, SIGNAL(clicked()), this, SLOT(send_local_unlockpkgs()) );
  connect(ui->tool_local_upgrade, SIGNAL(clicked()), this, SLOT(send_local_upgradepkgs()) );
  connect(ui->group_pending_log, SIGNAL(toggled(bool)), this, SLOT(pending_show_log(bool)) );
  update_local_buttons();
}

pkg_page::~pkg_page(){
  
}

//Initialize the CORE <-->Page connections
void pkg_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
  connect(CORE, SIGNAL(NewEvent(sysadm_client::EVENT_TYPE, QJsonValue)), this, SLOT(ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void pkg_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("AppCafe") );
  //Now run any CORE communications
  CORE->registerForEvents(sysadm_client::DISPATCHER);

  send_local_update();
  send_local_check_upgrade();
}

//Core requests
void pkg_page::send_local_update(){
  ui->tab_local->setEnabled(false);
  ui->label_local_loading->setVisible(true);
  QJsonObject obj;
    obj.insert("action","pkg_info");
    obj.insert("repo","local");
    obj.insert("result","full");
  CORE->communicate(TAG+"list_local", "sysadm", "pkg",obj);	
}

void pkg_page::send_local_audit(){
  QJsonObject obj;
    obj.insert("action","pkg_audit");
  CORE->communicate(TAG+"list_audit", "sysadm", "pkg",obj);	
}

void pkg_page::send_local_check_upgrade(){
  QJsonObject obj;
    obj.insert("action","pkg_check_upgrade");
  CORE->communicate(TAG+"check_upgrade", "sysadm", "pkg",obj);	  	
}

//Parsing Core Replies
void pkg_page::update_local_list(QJsonObject obj){
  QStringList origins = obj.keys();
  //qDebug() << "Update Local List:" << origins;
  bool sort = ui->tree_local->topLevelItemCount()<1;
  //Quick removal of any items no longer installed
  for(int i=0; i<ui->tree_local->topLevelItemCount(); i++){
    if( !origins.contains(ui->tree_local->topLevelItem(i)->whatsThis(0)) ){
      delete ui->tree_local->takeTopLevelItem(i);
      i--;
    }
  }
  //QStringList fields;
  int topnum = 0;
  for(int i=0; i<origins.length(); i++){
    if(origins[i].simplified().isEmpty()){ continue; } //just in case we get any empty keys
    /*if(obj.value(origins[i]).toObject().value("name").toString().isEmpty()){
      qDebug() << "Empty Item:" << origins[i] << obj.value(origins[i]).toObject();
    }*/
    //fields << obj.value(origins[i]).toObject().keys(); fields.removeDuplicates();
    //Find the item for this origin (if one exists)
    QTreeWidgetItem *it = 0;
    for(int j=0; j<ui->tree_local->topLevelItemCount(); j++){
      if(ui->tree_local->topLevelItem(j)->whatsThis(0)==origins[i]){ it = ui->tree_local->topLevelItem(j); break; }
    }
    if(it==0){
      //No Item available - need to create one
      it = new QTreeWidgetItem();
      it->setWhatsThis(0,origins[i]);
      it->setCheckState(0,(ui->check_local_all->isChecked() ? Qt::Checked : Qt::Unchecked) );
    }
    //Remove this item as needed based on viewing options
    if(obj.value(origins[i]).toObject().contains("reverse_dependencies") && !local_showall){
      delete it; //this will also remove it from the tree widget if it already exists there
      continue;
    }
    //Update the info within the item
    it->setText(1, obj.value(origins[i]).toObject().value("name").toString() );
      it->setToolTip(1, obj.value(origins[i]).toObject().value("comment").toString() );
      it->setStatusTip(1, it->toolTip(1));
    if(obj.value(origins[i]).toObject().contains("icon")){
      //qDebug() << "Found Icon:" << origins[i] << obj.value(origins[i]).toObject().value("icon").toString();
      it->setIcon(1, QIcon(obj.value(origins[i]).toObject().value("icon").toString()) );
    }
    it->setText(2, obj.value(origins[i]).toObject().value("version").toString() );
    it->setText(3, BtoHR(obj.value(origins[i]).toObject().value("flatsize").toString().toDouble()) );
    it->setText(4, origins[i].section("/",0,0) ); //category
    //Now the hidden data within each item
    it->setWhatsThis(1, obj.value(origins[i]).toObject().value("repository").toString() ); //which repo the pkg was installed from
    QStringList stat_ico = it->data(0,Qt::UserRole).toStringList();
    bool stat_changed = updateStatusList(&stat_ico, "lock", obj.value(origins[i]).toObject().value("locked").toString()=="1");
    stat_changed = stat_changed || updateStatusList(&stat_ico, "req", obj.value(origins[i]).toObject().contains("reverse_dependencies"));
    if( !obj.value(origins[i]).toObject().contains("reverse_dependencies")){ topnum++; }
    if(stat_changed){ 
      it->setData(0,Qt::UserRole, stat_ico); //save this for later
      updateStatusIcon(it); 
    }
    //More room for expansion later
    //Add this item as needed based on viewing options
    if(ui->tree_local->indexOfTopLevelItem(it)<0){
      ui->tree_local->addTopLevelItem(it);
    }
  }
  //qDebug() << "pkg fields:" << fields;
  //Re-sort the items if necessary
  if(sort){
    ui->tree_local->sortItems(1,Qt::AscendingOrder);
    for(int i=0; i<ui->tree_local->columnCount(); i++){ ui->tree_local->resizeColumnToContents(i); }
  }else if(!ui->tree_local->selectedItems().isEmpty()){
    ui->tree_local->scrollToItem(ui->tree_local->selectedItems().first());
  }
  //Now list the summary of the current packages
  QString summary = tr("Top-Level Packages: %1,  Total Installed Packages: %2");
  ui->tree_local->setStatusTip( summary.arg(QString::number(topnum), QString::number(origins.length())) );
  //Now that we have a list of local packages, get the audit of them as well
  send_local_audit();
}

void pkg_page::update_local_audit(QJsonObject obj){
  //qDebug() << "Audit Result:" << obj.keys();
  //qDebug() << " - full:" << obj;
  QStringList impacts = ArrayToStringList(obj.value("impacts_pkgs").toArray());
  QStringList vuln = ArrayToStringList(obj.value("vulnerable_pkgs").toArray());
  //Now go through all the current items - update the status as needed, and assign the status icon
  for(int i=0; i<ui->tree_local->topLevelItemCount(); i++){
    QString name = ui->tree_local->topLevelItem(i)->text(1);
    QStringList stat = ui->tree_local->topLevelItem(i)->data(0,Qt::UserRole).toStringList();
    bool changed = updateStatusList(&stat, "vuln", vuln.contains(name));
    changed = changed || updateStatusList(&stat, "dep-vuln", impacts.contains(name));
    if(changed){
      ui->tree_local->topLevelItem(i)->setData(0,Qt::UserRole, stat);
      updateStatusIcon( ui->tree_local->topLevelItem(i) );
    }
  }
}

void pkg_page::update_pending_process(QJsonObject obj){
  QString stat = obj.value("state").toString();
  QJsonObject details = obj.value("process_details").toObject();
  QString id = details.value("proc_id").toString();
  qDebug() << "Update Proc:" << id << stat << obj.keys();
  QTreeWidgetItem *it = 0;
  for(int i=0; i<ui->tree_pending->topLevelItemCount(); i++){
    if(ui->tree_pending->topLevelItem(i)->whatsThis(0) == id){ it = ui->tree_pending->topLevelItem(i); break;}
  }
  if(it!=0 && stat=="finished"){
    qDebug() << " - Got Finished";
    delete ui->tree_pending->takeTopLevelItem( ui->tree_pending->indexOfTopLevelItem(it) );
    it = 0;
  }else if(it==0 && stat!="finished"){
    qDebug() << " - Create item";
    //Need to create a new entry for this process
    it = new QTreeWidgetItem(ui->tree_pending);
      it->setWhatsThis(0, id);
      it->setText(1, obj.value("action").toString());
      it->setText(2, obj.value("proc_cmd").toString());
  }
  
  if(it!=0){
    qDebug() << " - Update item";
    it->setText(0, stat);
    if(stat=="running"){ 
      qDebug() << " - got running";
      ui->tree_pending->setCurrentItem(it); 
      ui->text_proc_running->setPlainText(obj.value("pkg_log").toString());
      QTextCursor cur = ui->text_proc_running->textCursor();
	cur.movePosition(QTextCursor::End);
      ui->text_proc_running->setTextCursor( cur );
      ui->text_proc_running->ensureCursorVisible();
    }
  }else{ 
    qDebug() << " - Clear log";
    ui->text_proc_running->clear();
  }
  qDebug() << " - Update title";
  QString title = QString(tr("(%1) Pending")).arg(ui->tree_pending->topLevelItemCount());
  ui->tabWidget->setTabText( ui->tabWidget->indexOf(ui->tab_queue), title);
}

// == SIMPLIFICATION FUNCTIONS ==
QString pkg_page::BtoHR(double bytes){
  QStringList units; units << "B" << "K" << "M" << "G" << "T" << "P";
  int unit = 0;
  while(bytes > 1024 && unit <units.length() ){
    unit++;
    bytes = bytes/1024.0;
  }
  //Round off the final number to 2 decimel places
  bytes = qRound(bytes*100)/100.0;
  return ( QString::number(bytes)+units[unit] );
}

QStringList pkg_page::ArrayToStringList(QJsonArray array){
  QStringList list;
  for(int i=0; i<array.count(); i++){ list << array[i].toString(); }
  return list;
}

void pkg_page::updateStatusIcon( QTreeWidgetItem *it ){
  QStringList stat = it->data(0,Qt::UserRole).toStringList();
  if(stat.isEmpty()){ it->setIcon(0,QIcon()); it->setStatusTip(0,""); it->setToolTip(0,""); return;}
  QStringList TT;
  QList<QPixmap> IL; //icon list
  for(int i=0; i<stat.length(); i++){
    if(stat[i]=="lock"){ IL << QPixmap(":/icons/black/lock.svg"); TT << tr("* Locked"); }
    else if(stat[i]=="vuln"){ IL << QPixmap(":/icons/black/forbidden.svg"); TT << tr("* Security vulnerability"); }
    else if(stat[i]=="dep-vuln"){ IL << QPixmap(":/icons/black/attention.svg"); TT << tr("* Dependency has security vulnerability"); }
    else if(stat[i]=="req"){ IL << QPixmap(":/icons/black/bookmark-used.svg"); TT << tr("* Used by other packages"); }
  }
  //Now set the icon
  if(IL.length()==1){
    //Show the icon itself
    //it->setIconSize( it->iconSize().height(), it->iconSize().height());
    it->setIcon(0, IL.first());
  }else{
    //Put all the icons in a row next to each other
    QPixmap combo(ui->tree_local->iconSize()); combo.fill(QColor(Qt::transparent));
    QPainter P(&combo);
    int sz = combo.height();
    for(int i=0; i<IL.length(); i++){
      P.drawPixmap(i*sz, 0, IL[i] );
    }
    QIcon ico; ico.addPixmap(combo);
    it->setIcon(0, ico);
  }
  //Now set the information text(s)
  it->setToolTip(0,TT.join("\n"));
  it->setStatusTip(0, it->whatsThis(0)+":  "+TT.join("   "));
}

bool pkg_page::updateStatusList(QStringList *list, QString stat, bool enabled){
  if(list->contains(stat) && !enabled){ 
    list->removeAll(stat); return true;
  }else if(!list->contains(stat) && enabled){
    list->append(stat); return true;
  }else{
    return false;
  }
}

// === PRIVATE SLOTS ===
void pkg_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(TAG) || namesp=="error" || name=="error"){ return; }
  qDebug() << "Got Reply:" << id << args.toObject().keys();
  if( id==TAG+"list_local" && args.isObject() ){
    //Got update to the list of locally installed packages
    ui->tab_local->setEnabled(true);
    ui->label_local_loading->setVisible(false);
    if(args.toObject().contains("pkg_info")){ update_local_list( args.toObject().value("pkg_info").toObject() ); }
  }else{
    qDebug() << " - arguments:" << args;
  }
}

void pkg_page::ParseEvent(sysadm_client::EVENT_TYPE type, QJsonValue val){
  if( type!=sysadm_client::DISPATCHER ){ return; }
  qDebug() << "Got Dispatcher Event:" << val.toObject().value("event_system").toString();
  //Also check that this is a sysadm/pkg event (ignore all others)
  if(!val.toObject().contains("event_system") || val.toObject().value("event_system").toString()!="sysadm/pkg"){ return; }
  //Now go through and update the UI based on the type of action for this event
  QString act = val.toObject().value("action").toString();
  bool finished = (val.toObject().value("state").toString() == "finished");
  if( act=="pkg_remove" || act=="pkg_install" || act=="pkg_lock" || act=="pkg_unlock" || act=="pkg_upgrade"){
    if(finished){ 
      //Need to update the list of installed packages    
      send_local_update();
    }
    // Need to update the list of pending processes
    update_pending_process(val.toObject());
  }else if(act=="pkg_check_upgrade"){
    local_hasupdates = (val.toObject().value("updates_available").toString()=="true");
    update_local_buttons();
  }else if(act=="pkg_audit" && finished){
    update_local_audit(val.toObject());
  }
}

//GUI Updates
// - local tab
void pkg_page::update_local_buttons(){
  ui->tool_local_lock->setVisible(local_advmode);
  ui->tool_local_unlock->setVisible(local_advmode);
  ui->tool_local_upgrade->setVisible(local_advmode && local_hasupdates);
  //Now see if there are any items checked and enable/disable buttons as needed	
  bool gotcheck = false;
  for(int i=0; i<ui->tree_local->topLevelItemCount() && !gotcheck; i++){
    gotcheck = (ui->tree_local->topLevelItem(i)->checkState(0)==Qt::Checked);
  }
  ui->tool_local_lock->setEnabled(gotcheck);
  ui->tool_local_unlock->setEnabled(gotcheck);
  ui->tool_local_rem->setEnabled(gotcheck);
}

void pkg_page::update_local_pkg_check(bool checked){
  for(int i=0; i<ui->tree_local->topLevelItemCount(); i++){
    ui->tree_local->topLevelItem(i)->setCheckState(0, (checked ? Qt::Checked : Qt::Unchecked) );
  }
}
void pkg_page::update_local_viewall(bool checked){
  local_showall = checked;
  send_local_update();
}

void pkg_page::update_local_viewadv(bool checked){
  local_advmode = checked;
  update_local_buttons();
}

void pkg_page::goto_browser_from_local(QTreeWidgetItem *it){
  QString origin = it->whatsThis(0);
  QString repo = it->whatsThis(1);
  browser_goto_pkg(origin, repo);
  ui->tabWidget->setCurrentWidget(ui->tab_repo);
}

// - repo tab
void pkg_page::browser_goto_pkg(QString origin, QString repo){
	
}

// - pending tab
void pkg_page::pending_show_log(bool show){
  ui->text_proc_running->setVisible(show);	
}

//GUI -> Core Requests
void pkg_page::send_local_rmpkgs(){
  QStringList pkgs;
  for(int i=0; i<ui->tree_local->topLevelItemCount(); i++){
    if(ui->tree_local->topLevelItem(i)->checkState(0)==Qt::Checked){ pkgs << ui->tree_local->topLevelItem(i)->whatsThis(0); }
  }
  if(pkgs.isEmpty()){ return; } //nothing to do
  QJsonObject obj;
    obj.insert("action","pkg_remove");
    obj.insert("recursive","true"); //cleanup orphaned packages
    obj.insert("pkg_origins", QJsonArray::fromStringList(pkgs) );
  CORE->communicate(TAG+"pkg_remove", "sysadm", "pkg",obj);		
}

void pkg_page::send_local_lockpkgs(){
QStringList pkgs;
  for(int i=0; i<ui->tree_local->topLevelItemCount(); i++){
    if(ui->tree_local->topLevelItem(i)->checkState(0)==Qt::Checked){ pkgs << ui->tree_local->topLevelItem(i)->whatsThis(0); }
  }
  if(pkgs.isEmpty()){ return; } //nothing to do
  QJsonObject obj;
    obj.insert("action","pkg_lock");
    obj.insert("pkg_origins", QJsonArray::fromStringList(pkgs) );
  CORE->communicate(TAG+"pkg_lock", "sysadm", "pkg",obj);
}

void pkg_page::send_local_unlockpkgs(){
QStringList pkgs;
  for(int i=0; i<ui->tree_local->topLevelItemCount(); i++){
    if(ui->tree_local->topLevelItem(i)->checkState(0)==Qt::Checked){ pkgs << ui->tree_local->topLevelItem(i)->whatsThis(0); }
  }
  if(pkgs.isEmpty()){ return; } //nothing to do
  QJsonObject obj;
    obj.insert("action","pkg_unlock");
    obj.insert("pkg_origins", QJsonArray::fromStringList(pkgs) );
  CORE->communicate(TAG+"pkg_unlock", "sysadm", "pkg",obj);
}

void pkg_page::send_local_upgradepkgs(){
  QJsonObject obj;
    obj.insert("action","pkg_upgrade");
  CORE->communicate(TAG+"pkg_upgrade", "sysadm", "pkg",obj);
  ui->tabWidget->setCurrentWidget(ui->tab_queue);
  local_hasupdates = false;
  update_local_buttons();
}
