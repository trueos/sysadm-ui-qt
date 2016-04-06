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
  NMAN = new QNetworkAccessManager();
  local_showall = local_advmode = local_hasupdates = false;
  //Create the special menus
  repo_backM = new QMenu(this);
    connect(repo_backM, SIGNAL(triggered(QAction*)), this, SLOT(browser_go_back(QAction*)) );
    ui->tool_repo_back->setMenu(repo_backM);
  repo_catM = new QMenu(this);
    connect(repo_catM, SIGNAL(triggered(QAction*)), this, SLOT(browser_goto_cat(QAction*)) );
    ui->tool_home_gotocat->setMenu(repo_catM);
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
  connect(NMAN, SIGNAL(finished(QNetworkReply*)), this, SLOT(icon_available(QNetworkReply*)) );
  connect(ui->check_local_all, SIGNAL(toggled(bool)), this, SLOT(update_local_pkg_check(bool)) );
  connect(ui->tree_local, SIGNAL(itemSelectionChanged()), this, SLOT(update_local_buttons()) );
  connect(ui->tree_local, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(update_local_buttons()) );
  connect(ui->tree_local, SIGNAL(itemActivated(QTreeWidgetItem*, int)), this, SLOT(goto_browser_from_local(QTreeWidgetItem*)) );
  connect(ui->tool_local_rem, SIGNAL(clicked()), this, SLOT(send_local_rmpkgs()) );
  connect(ui->tool_local_lock, SIGNAL(clicked()), this, SLOT(send_local_lockpkgs()) );
  connect(ui->tool_local_unlock, SIGNAL(clicked()), this, SLOT(send_local_unlockpkgs()) );
  connect(ui->tool_local_upgrade, SIGNAL(clicked()), this, SLOT(send_local_upgradepkgs()) );
  connect(ui->group_pending_log, SIGNAL(toggled(bool)), this, SLOT(pending_show_log(bool)) );
  connect(ui->combo_repo, SIGNAL(activated(const QString&)), this, SLOT(update_repo_changed()) );
  connect(ui->tool_app_install, SIGNAL(clicked()), this, SLOT(send_repo_installpkg()) );
  connect(ui->tool_app_uninstall, SIGNAL(clicked()), this, SLOT(send_repo_rmpkg()) );
  connect(ui->tool_app_firstss, SIGNAL(clicked()), this, SLOT(browser_first_ss()) );
  connect(ui->tool_app_prevss, SIGNAL(clicked()), this, SLOT(browser_prev_ss()) );
  connect(ui->tool_app_nextss, SIGNAL(clicked()), this, SLOT(browser_next_ss()) );
  connect(ui->tool_app_lastss, SIGNAL(clicked()), this, SLOT(browser_last_ss()) );
  connect(ui->line_repo_search, SIGNAL(returnPressed()), this, SLOT(send_start_search()) );
  connect(ui->tool_repo_search, SIGNAL(clicked()), this, SLOT(send_start_search()) );
  connect(ui->combo_search_cat, SIGNAL(activated(int)), this, SLOT(browser_filter_search_cat()) );
  connect(ui->tool_repo_back, SIGNAL(clicked()), this, SLOT(browser_go_back()) );
  //Ensure that the proper pages/tabs are initially visible
  ui->tabWidget->setCurrentWidget(ui->tab_repo);
  ui->stacked_repo->setCurrentWidget(ui->page_home);
  ui->tabWidget_app->setCurrentWidget(ui->tab_app_description);
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

  send_list_repos();
  send_local_update();
  send_local_check_upgrade();
}

//Core requests
void pkg_page::send_list_repos(){
  QJsonObject obj;
    obj.insert("action","list_repos");
  CORE->communicate(TAG+"list_repos", "sysadm", "pkg",obj);
}

void pkg_page::send_list_cats(QString repo){
  if(repo.isEmpty()){ repo = "local"; }
  QJsonObject obj;
    obj.insert("action","list_categories");
    obj.insert("repo",repo);
  CORE->communicate(TAG+"list_cats", "sysadm", "pkg",obj); 
}

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

void pkg_page::send_repo_app_info(QString origin, QString repo){
  ui->page_pkg->setEnabled(false);
  qDebug() << "Send app info request:" << origin << repo;
  if(repo.isEmpty()){ repo = "local"; }
  QJsonObject obj;
    obj.insert("action","pkg_info");
    obj.insert("repo",repo);
    obj.insert("result","full");
    obj.insert("pkg_origins",origin);
  CORE->communicate(TAG+"list_app_info", "sysadm", "pkg",obj);	
}

//Parsing Core Replies
void pkg_page::update_local_list(QJsonObject obj){
  QStringList origins = obj.keys();
  origin_installed = origins; //keep this list around for later
      //See if the currently-shown pkgs needs updating too
      if(!ui->page_pkg->whatsThis().isEmpty() && ui->stacked_repo->currentWidget()==ui->page_pkg){
        send_repo_app_info(ui->page_pkg->whatsThis(), ui->combo_repo->currentText());
      }else if(ui->scroll_search->widget()!=0 && ui->stacked_repo->currentWidget()==ui->page_search){
	send_start_search(ui->label_search_term->text()); //re-run the last search and update items as needed
      }
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
  //qDebug() << "Update Proc:" << id << stat << obj.keys();
  QTreeWidgetItem *it = 0;
  for(int i=0; i<ui->tree_pending->topLevelItemCount(); i++){
    if(ui->tree_pending->topLevelItem(i)->whatsThis(0) == id){ it = ui->tree_pending->topLevelItem(i); break;}
  }
  if(it!=0 && stat=="finished"){
    //qDebug() << " - Got Finished";
    QStringList origins = it->text(2).split(" ").filter("/");
    for(int i=0; i<origins.length(); i++){ origin_pending.removeAll(origins[i]); }
    delete ui->tree_pending->takeTopLevelItem( ui->tree_pending->indexOfTopLevelItem(it) );
    it = 0;
    //See if the current pkg page needs the status updated as well
    if(origins.contains( ui->page_pkg->whatsThis()) && ui->stacked_repo->currentWidget()==ui->page_pkg){
      send_repo_app_info(ui->page_pkg->whatsThis(), ui->combo_repo->currentText());
    }
  }else if(it==0 && stat!="finished"){
    //qDebug() << " - Create item";
    //Need to create a new entry for this process
    it = new QTreeWidgetItem(ui->tree_pending);
      it->setWhatsThis(0, id);
      it->setText(1, obj.value("action").toString());
      it->setText(2, obj.value("proc_cmd").toString());
    //Update the internal list of origins which are pending
    QStringList origins = it->text(2).split(" ").filter("/");
    origin_pending << origins;
    origin_pending.removeDuplicates();
    //See if the current pkg page needs the status updated as well
    if(origins.contains( ui->page_pkg->whatsThis()) && ui->stacked_repo->currentWidget()==ui->page_pkg){
      send_repo_app_info(ui->page_pkg->whatsThis(), ui->combo_repo->currentText());
    }
  }
  
  if(it!=0){
    //qDebug() << " - Update item";
    it->setText(0, stat);
    if(stat=="running"){ 
      //qDebug() << " - got running";
      ui->tree_pending->setCurrentItem(it); 
      ui->text_proc_running->setPlainText(obj.value("pkg_log").toString());
      QTextCursor cur = ui->text_proc_running->textCursor();
	cur.movePosition(QTextCursor::End);
      ui->text_proc_running->setTextCursor( cur );
      ui->text_proc_running->ensureCursorVisible();
    }
  }else{ 
    //qDebug() << " - Clear log";
    ui->text_proc_running->clear();
  }
  //qDebug() << " - Update title";
  QString title = QString(tr("(%1) Pending")).arg(ui->tree_pending->topLevelItemCount());
  ui->tabWidget->setTabText( ui->tabWidget->indexOf(ui->tab_queue), title);
}

void pkg_page::update_repo_app_info(QJsonObject obj){
  //qDebug() << "Got app Info:" << obj;
  if(obj.isEmpty()){ return; }
  QString origin = obj.keys().first();
  if(origin.isEmpty()){ return; }
  ui->page_pkg->setWhatsThis(origin);
  obj = obj.value(origin).toObject(); //simplification - go one level deeper for easy access to info
  qDebug() << "Show Package:" << origin << obj.keys();
  QString repo = obj.value("repository").toString();
  
  //Now update all the info on the page
  // - General Info Tab
  if(obj.contains("icon")){
    LoadImageFromURL( ui->label_app_icon, obj.value("icon").toString());
    ui->label_app_icon->setVisible(true);
  }else{ ui->label_app_icon->setVisible(false); }
  QString name = obj.value("name").toString();
  if(obj.contains("www")){ name = "<a href=\""+obj.value("www").toString()+"\">"+name+"</a>"; }
  ui->label_app_name->setText(name);
  ui->label_app_comment->setText(obj.value("comment").toString());
  ui->label_app_version->setText(obj.value("version").toString());
  ui->label_app_maintainer->setText(obj.value("maintainer").toString());
  ui->label_app_arch->setText(obj.value("arch").toString());
  if(obj.contains("osversion")){
    ui->label_app_os->setText(obj.value("osversion").toString());
    ui->label_app_os->setVisible(true); ui->label_9->setVisible(true);
  }else{
    ui->label_app_os->setVisible(false); ui->label_9->setVisible(false);
  }
  if(obj.contains("pkgsize")){
    ui->label_app_dlsize->setText( BtoHR(obj.value("pkgsize").toString().toDouble()) );
    ui->label_app_dlsize->setVisible(true); ui->label_10->setVisible(true);
  }else{
    ui->label_app_dlsize->setVisible(false); ui->label_10->setVisible(false);
  }
  ui->label_app_isize->setText( BtoHR(obj.value("flatsize").toString().toDouble()) );
  ui->label_app_license->setText(ArrayToStringList(obj.value("licenses").toArray()).join(", ") );
  // - Description Tab
  ui->text_app_description->setPlainText( obj.value("desc").toString() );
  // - Screenshots Tab
  QStringList screens = obj.keys().filter("screen");
  if(screens.isEmpty() && ui->tabWidget_app->currentWidget()==ui->tab_app_screenshot){
    ui->tabWidget_app->setCurrentWidget(ui->tab_app_description); //make sure screenshots tab is not visible
  }else if(!screens.isEmpty()){
    screens.sort();
    //Turn the keys into URLS
    for(int i=0; i<screens.length(); i++){ screens[i] = obj.value(screens[i]).toString(); } //replace key with value
    ui->label_app_ssnum->setWhatsThis(screens.join("::::::") );
    showScreenshot(1); //show the first screenshot
  }
  ui->tabWidget_app->setTabEnabled( ui->tabWidget_app->indexOf(ui->tab_app_screenshot), !screens.isEmpty());
  // - Details Tab
  ui->tree_app_details->clear();
  if(obj.contains("options")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Build Options"));
    QStringList opts = obj.value("options").toObject().keys();
    for(int i=0; i<opts.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, opts[i]+" = "+obj.value("options").toObject().value(opts[i]).toString());
    }
  }
  if(obj.contains("dependencies")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Dependencies"));
    QStringList tmpl = ArrayToStringList(obj.value("dependencies").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  
  }
  if(obj.contains("reverse_dependencies")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Required By"));
    QStringList tmpl = ArrayToStringList(obj.value("reverse_dependencies").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  
  }
  if(obj.contains("conflicts")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Conflicts"));
    QStringList tmpl = ArrayToStringList(obj.value("conflicts").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  
  }
  if(obj.contains("shlibs_required")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Shared Libraries (Required)"));
    QStringList tmpl = ArrayToStringList(obj.value("shlibs_required").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  	  
  }
  if(obj.contains("shlibs_provided")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Shared Libraries (Provided)"));
    QStringList tmpl = ArrayToStringList(obj.value("shlibs_provided").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  
  }
  if(obj.contains("provides")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Provides"));
    QStringList tmpl = ArrayToStringList(obj.value("provides").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  
  }
  if(obj.contains("requires")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Requires"));
    QStringList tmpl = ArrayToStringList(obj.value("requires").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  
  }
  if(obj.contains("users")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Users"));
    QStringList tmpl = ArrayToStringList(obj.value("users").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  
  }
  if(obj.contains("groups")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Groups"));
    QStringList tmpl = ArrayToStringList(obj.value("groups").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  	  
  }
  if(obj.contains("config_files")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Configuration Files"));
    QStringList tmpl = ArrayToStringList(obj.value("config_files").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  
  }
  if(obj.contains("files")){
    QTreeWidgetItem *cat = new QTreeWidgetItem(ui->tree_app_details, QStringList() << tr("Files"));
    QStringList tmpl = ArrayToStringList(obj.value("files").toArray());
    for(int i=0; i<tmpl.length(); i++){
      QTreeWidgetItem *tmp = new QTreeWidgetItem(cat);
	tmp->setText(0, tmpl[i]);
    }	  	  
  }
  ui->tree_app_details->sortItems(0, Qt::AscendingOrder);
  
  //Now Update the install/remove/status widgets as needed
  // - first see if there is a pending status for it
  if( origin_pending.contains(origin) ){
    ui->label_app_status->setText(tr("Pending.."));
    ui->label_app_status->setVisible( true ) ;
  }else{
    ui->label_app_status->setText("");
    ui->label_app_status->setVisible(false);
  }
  if(ui->label_app_status->text().isEmpty()){
    //No pending status - check if it is installed or not
    bool installed = origin_installed.contains(origin);
    ui->tool_app_install->setVisible(!installed);
    ui->tool_app_uninstall->setVisible(installed);
  }else{
    ui->tool_app_install->setVisible(false);
    ui->tool_app_uninstall->setVisible(false);
  }
  
  //Now ensure this page is visible (since the info is only loaded on demand)
  ui->tabWidget->setCurrentWidget(ui->tab_repo);
  ui->stacked_repo->setCurrentWidget(ui->page_pkg);
}

void pkg_page::update_repo_app_lists(QScrollArea *scroll, QJsonObject(obj) ){
  if(scroll->widget()==0){ scroll->setWidget( new QWidget(scroll) ); }
  if(scroll->widget()->layout()==0){ 
    scroll->widget()->setLayout( new QVBoxLayout(scroll->widget()) ); 
    scroll->widget()->layout()->setContentsMargins(0,0,0,0);
    scroll->widget()->layout()->setSpacing(2);
  }
  QStringList origins = obj.keys(); //item ID's we need to show
  if(scroll!=ui->scroll_search){ origins.sort(Qt::CaseInsensitive); } //don't sort search results (ordered by priority)
  QStringList used;
  //Go through the the current widgets in the area and remove/update them
  //qDebug() << "Remove old items";
  for(int i=0; i<scroll->widget()->layout()->count(); i++){
    if(scroll->widget()->layout()->itemAt(i)->widget()==0){
      delete scroll->widget()->layout()->takeAt(i);
      i--;
      continue;
    }
    BrowserItem *BI = static_cast<BrowserItem *>( scroll->widget()->layout()->itemAt(i)->widget() );
    if(BI->whatsThis().isEmpty() || !origins.contains(BI->ID()) ){ 
      //qDebug() << "Delete BI";
      scroll->widget()->layout()->takeAt(i)->widget()->deleteLater();
      i--;
    }else{
      //Update this item
      //qDebug() << "Update BI";
      updateBrowserItem(BI, obj.value(BI->ID()).toObject());
      used << BI->ID(); //keep track of which items we already updated
    }
  }
  //Now go through and add any new items as needed
  //qDebug() << "Add new BI's";
  for(int i=0; i<origins.length(); i++){
    if(used.contains(origins[i])){ continue; } //already handled
    //qDebug() << " - Create Item:" << origins[i];
    BrowserItem *BI = new BrowserItem(scroll->widget(), origins[i]);
     connect(BI, SIGNAL(ItemClicked(QString)), this, SLOT(browser_goto_pkg(QString)) );
     connect(BI, SIGNAL(InstallClicked(QString)), this, SLOT(send_repo_installpkg(QString)) );
     connect(BI, SIGNAL(RemoveClicked(QString)), this, SLOT(send_repo_rmpkg(QString)) );
    //qDebug() << " - Update Item";
    updateBrowserItem(BI, obj.value(origins[i]).toObject());
    static_cast<QBoxLayout*>(scroll->widget()->layout())->insertWidget(i, BI); 
  }
  //qDebug() << "Done Adding BI's";
  if(origins.isEmpty()){
    scroll->widget()->layout()->addWidget( new QLabel("<i>"+tr("No Packages Found")+"</i>",scroll->widget()) );
  }
  //Now add a spacer to the bottom (always gets cleaned up by the routine above)
  static_cast<QBoxLayout*>(scroll->widget()->layout())->addStretch();
  //Now update the status tip for the scroll area to show some stats
  scroll->setStatusTip( QString(tr("Number of packages: %1")).arg(QString::number(origins.length())) );
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

//Load an image from a URL
void pkg_page::LoadImageFromURL(QLabel *widget, QString url){
  url = url.remove("\\\\");
  url = url.remove("\\\"");
  //qDebug() << "Try to load image from URL:" << url;
  //Set a temporary image on the widget
  widget->setPixmap( QPixmap(":/icons/black/photo.svg") );
  //Remove any old images which are still loading for this same widget
  QNetworkReply *reply = pendingIcons.key(widget,0);
  if(reply!=0){
    reply->abort();
    pendingIcons.remove(reply);
  }
  //Start the download of the image
  pendingIcons.insert(NMAN->get( QNetworkRequest(QUrl(url)) ), widget );
  
}

void pkg_page::showScreenshot(int num){
  //NOTE: num should be 1-(length)
  QStringList screens = ui->label_app_ssnum->whatsThis().split("::::::");
  if(screens.isEmpty()){ return; } //just in case - should never happen though
  if(num <=0){ num = screens.length(); } //wrap around
  else if(num>screens.length()){ num = 1; } //wrap around
  
  //Update the label
  ui->label_app_ssnum->setText( QString::number(num)+"/"+QString::number(screens.length()) );
  //Now update the buttons
  ui->tool_app_firstss->setEnabled(num>1);
  ui->tool_app_prevss->setEnabled(num>1);
  ui->tool_app_nextss->setEnabled(num<screens.length());
  ui->tool_app_lastss->setEnabled(num<screens.length());
  //Update the screenshot
  ui->label_app_screenshot->setStatusTip(screens[num-1]); //show the URL on mouseover
  LoadImageFromURL(ui->label_app_screenshot, screens[num-1]);
}

//Browser Item Update
void pkg_page::updateBrowserItem(BrowserItem *it, QJsonObject data){
  it->setText("name", data.value("name").toString());
  it->setText("version", data.value("version").toString());
  it->setText("comment",data.value("comment").toString());
  if(data.contains("icon")){ 
    QString url = data.value("icon").toString();
    QLabel* ico = it->iconLabel();
    if(ico->pixmap()==0 || ico->whatsThis()!=url){
	ico->setWhatsThis(url);
	LoadImageFromURL( ico, url); 
    }
  }
  it->iconLabel()->setVisible(data.contains("icon"));
  //See if it is installed/pending
  int stat = 1; //not installed
  if( origin_pending.contains(it->ID()) ){ stat = 2; }
  else if( origin_installed.contains(it->ID()) ){ stat = 0; }
  //qDebug() << "Item Stat:" << it->ID() << stat << origin_pending;
  it->setInteraction(stat);
}

// === PRIVATE SLOTS ===
void pkg_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(id.startsWith(TAG)){ qDebug() << "Got Reply:" << id << name  << namesp << args.toObject().keys(); }
  if(!id.startsWith(TAG) ){ return; }
  bool haserror = (namesp=="error" || name=="error");
  if( id==TAG+"list_local" && args.isObject() ){
    //Got update to the list of locally installed packages
    ui->tab_local->setEnabled(true);
    ui->label_local_loading->setVisible(false);
    if(args.toObject().contains("pkg_info")){ update_local_list( args.toObject().value("pkg_info").toObject() ); }
  }else if(id==TAG+"list_app_info" && args.isObject()){
    ui->page_pkg->setEnabled(true);
    update_repo_app_info(args.toObject().value("pkg_info").toObject());
    browser_update_history();
  }else if(id==TAG+"list_repos" && args.isObject()){
    QStringList repos = ArrayToStringList(args.toObject().value("list_repos").toArray());
    repos.removeAll("local");
    repos.removeAll("");
    ui->combo_repo->clear();
    ui->combo_repo->addItems(repos);
    ui->combo_repo->setWhatsThis(ui->combo_repo->currentText()); //save this for later checks
    send_list_cats(ui->combo_repo->currentText());
    //Now kick off loading the home page data
    browser_update_history();
  }else if(id==TAG+"pkg_search" && args.isObject()){
    update_repo_app_lists(ui->scroll_search, args.toObject().value("pkg_search").toObject());
    ui->stacked_repo->setCurrentWidget(ui->page_search);
    browser_update_history();
  }else if(id==TAG+"list_browse" && args.isObject()){
    update_repo_app_lists(ui->scroll_cat, args.toObject().value("pkg_info").toObject());
    ui->stacked_repo->setCurrentWidget(ui->page_cat);
    browser_update_history();
  }else if(id==TAG+"list_cats" && args.isObject()){
    QStringList cats = ArrayToStringList( args.toObject().value("list_categories").toArray() );
    QStringList viscats = catsToText(cats); //format: <translated string>::::<cat>
    //Now add these categories to the widgets
    ui->combo_search_cat->clear();
    ui->combo_search_cat->addItem(tr("All Categories"));
    repo_catM->clear();
    for(int i=0; i<cats.length(); i++){ 
      ui->combo_search_cat->addItem( viscats[i].section("::::",0,0), viscats[i].section("::::",1,1)); 
      QAction *tmp = repo_catM->addAction(viscats[i].section("::::",0,0));
	tmp->setWhatsThis(viscats[i].section("::::",1,1));
    }
    //Now create the home page
    GenerateHomePage(cats, ui->combo_repo->currentText());
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
  if(ui->tabWidget->currentWidget()!=ui->tab_local || ui->tree_local->indexOfTopLevelItem(it)<0 ){ return; } //stray signal (changing items around?) - ignore it
  QString origin = it->whatsThis(0);
  QString repo = it->whatsThis(1);
  if(repo.isEmpty()){ repo = "local"; }
  send_repo_app_info(origin, "local");
}

// - repo tab
void pkg_page::browser_goto_pkg(QString origin, QString repo){
  if(repo.isEmpty()){ repo = ui->combo_repo->currentText(); }
  send_repo_app_info(origin, repo);
}

void pkg_page::browser_goto_cat(QAction* act){
  QString cat = ui->page_cat->whatsThis(); //current category
  if(act!=0){ cat = act->whatsThis(); }
  if(cat.isEmpty()){ return; } //nothing to do
  send_start_browse(cat);
}

void pkg_page::update_repo_changed(){
  if(ui->combo_repo->whatsThis() == ui->combo_repo->currentText()){ return; }
  ui->combo_repo->setWhatsThis(ui->combo_repo->currentText()); //save for later checks
  //Repo Changed - reload the current page with the new repo
  if(!ui->page_pkg->whatsThis().isEmpty() && ui->stacked_repo->currentWidget()==ui->page_pkg){
    send_repo_app_info(ui->page_pkg->whatsThis(), ui->combo_repo->currentText());
  }else if(ui->stacked_repo->currentWidget()==ui->page_search){
    send_start_search(ui->label_search_term->text()); //re-run the last search and update items as needed
  }else{
    send_list_cats(ui->combo_repo->currentText());
  }
}

void pkg_page::icon_available(QNetworkReply *reply){
  //Get the widget for this reply
  qDebug() << "Icon Available:" << reply->url();
  if(!pendingIcons.contains(reply)){ return; }
  QLabel *label = pendingIcons.take(reply);
  //Make sure the label is still valid
  qDebug() << " - Loading icon onto widget:" << reply->url();
  if( !this->isAncestorOf(label) ){ 
    bool bad = true;
    if(ui->scroll_cat->widget()!=0){ bad = bad && !ui->scroll_cat->widget()->isAncestorOf(label); }
    if(ui->scroll_search->widget()!=0){ bad = bad && !ui->scroll_search->widget()->isAncestorOf(label); }
    if(bad){ return;  }
  } //Widget removed while loading the URL
  QImage img = QImage::fromData(reply->readAll());
  if(img.isNull()){ label->setVisible(false); }
  else{
    QSize sz = label->size();
    //qDebug() << "Size:" << sz << label->size() << label->sizeHint();
    label->setPixmap( QPixmap::fromImage(img).scaled( sz, Qt::KeepAspectRatio, Qt::SmoothTransformation) );
  }
  qDebug() << " - Done";
}

void pkg_page::browser_last_ss(){
 int num = ui->label_app_ssnum->text().section("/",-1).toInt();
 showScreenshot(num);
}

void pkg_page::browser_next_ss(){
 int num = ui->label_app_ssnum->text().section("/",0,0).toInt();
 showScreenshot(num+1);	
}

void pkg_page::browser_prev_ss(){
 int num = ui->label_app_ssnum->text().section("/",0,0).toInt();
 showScreenshot(num-1);	
}

void pkg_page::browser_first_ss(){
 showScreenshot(1);	
}

void pkg_page::browser_filter_search_cat(){
  if(ui->stacked_repo->currentWidget()!=ui->page_search){ return; } //some other change (not from user)
  QString search = ui->label_search_term->text(); //last-used search
  send_start_search(search);
}

void pkg_page::browser_go_back(QAction *act){
  QString go = "home";
  if(act!=0){
    go = act->whatsThis();
    //Remove all the actions after this one
    QList<QAction*> acts = repo_backM->actions();
    for(int i=acts.indexOf(act); i<acts.length(); i++){
      if(i<0){i=0;} //just in case - should never happen though
      repo_backM->removeAction(acts[i]);
    }
  }else{
    //use the last action in the history menu
    QList<QAction*> acts = repo_backM->actions();
    if(acts.length()>1){
      go = acts[acts.length()-2]->whatsThis();
      repo_backM->removeAction( acts[acts.length()-1] );
      repo_backM->removeAction( acts[acts.length()-2]);
    }else if(!acts.isEmpty()){
      repo_backM->removeAction( acts.last() );
    }
  }
  if(go.startsWith("cat::")){ send_start_browse(go.section("::",2,-1)); }
  else if(go.startsWith("pkg::")){ browser_goto_pkg(go.section("::",2,-1), go.section("::",1,1)); }
  else if(go.startsWith("search::")){ send_start_search(go.section("::",2,-1)); }
  else{ ui->stacked_repo->setCurrentWidget(ui->page_home); }
}

void pkg_page::browser_update_history(){
  QString go;
  if(ui->stacked_repo->currentWidget()==ui->page_home){
    go = "home";
  }else if(ui->stacked_repo->currentWidget()==ui->page_cat){
    go = "cat::%1::"+ui->page_cat->whatsThis();
  }else if(ui->stacked_repo->currentWidget()==ui->page_pkg){
    go = "pkg::%1::"+ui->page_pkg->whatsThis();
  }else if(ui->stacked_repo->currentWidget()==ui->page_search){
    go = "search::%1::"+ui->label_search_term->text();
  }
  if(go.contains("%1")){ go = go.arg(ui->combo_repo->currentText()); }
  //Now make sure we don't duplicate any history items
  QList<QAction*> acts = repo_backM->actions();
  QString lastgo;
  if(!acts.isEmpty()){ lastgo = acts.last()->whatsThis(); }
  if(go!=lastgo){
    QString txt;
    if(go=="home"){ txt = tr("Home Page"); }
    else{
	if(go.startsWith("cat")){ txt = tr("Browse Category: %1"); }
	else if(go.startsWith("pkg")){ txt = tr("View Package: %1"); }
	else if(go.startsWith("search")){ txt = tr("Search: %1"); }
	txt = txt.arg(go.section("::",2,-1));
    }
    QAction *tmp = repo_backM->addAction(txt);
	tmp->setWhatsThis(go);
  }
}

void pkg_page::browser_home_button_clicked(QString action){
  //FORMAT NOTE for actions:
  //  For search: 		"search::<category>::<search term>"
  //  For category: 	"cat::<category>"
  //  For package:	"pkg::<origin>"
	
  //home button action clicked
  qDebug() << "Home Button Action:" << action;
  if(action.startsWith("search::")){
    //First set the category filter if needed
    QString cat = action.section("::",1,1);
    if(cat.isEmpty()){ ui->combo_search_cat->setCurrentIndex(0); }
    else{
      int index = ui->combo_search_cat->findData(cat);
      if(index<0){ index = 0; }
      ui->combo_search_cat->setCurrentIndex(index);
    }
    send_start_search(action.section("::",2,-1));
    
  }else if(action.startsWith("cat::") ){
    send_start_browse(action.section("::",1,-1));
  }else if(action.startsWith("pkg::") ){
    browser_goto_pkg(action.section("::",1,-1));
  }
}

// - pending tab
void pkg_page::pending_show_log(bool show){
  ui->text_proc_running->setVisible(show);	
}

//GUI -> Core Requests
// - local tab
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

// - repo tab
void pkg_page::send_repo_rmpkg(QString origin){
  if(origin.isEmpty()){ origin = ui->page_pkg->whatsThis(); }
  QJsonObject obj;
    obj.insert("action","pkg_remove");
    obj.insert("recursive","true"); //cleanup orphaned packages
    obj.insert("pkg_origins", origin );
  CORE->communicate(TAG+"pkg_remove", "sysadm", "pkg",obj);	
}

void pkg_page::send_repo_installpkg(QString origin){
  //This is called from the app page or the search/browse pages
  if(origin.isEmpty()){ origin = ui->page_pkg->whatsThis(); } //app page used
  QString repo = ui->combo_repo->currentText();
  QJsonObject obj;
    obj.insert("action","pkg_install");
    obj.insert("pkg_origins", origin );
    if(!repo.isEmpty() && repo!="local"){ obj.insert("repo",repo); }
  CORE->communicate(TAG+"pkg_unlock", "sysadm", "pkg",obj);
}

void pkg_page::send_start_search(QString search){
  if(search.isEmpty()){ search = ui->line_repo_search->text(); }
  if(search.isEmpty()){ return; } //do nothing
  QJsonObject obj;
    obj.insert("action","pkg_search");
    obj.insert("repo",ui->combo_repo->currentText());
    obj.insert("search_term", search );
    if(ui->stacked_repo->currentWidget() == ui->page_cat){ 
      //Currently within a special category - only search here
      int cat = ui->combo_search_cat->findData(ui->page_cat->whatsThis());
      if(cat<0){ cat = 0; }
      ui->combo_search_cat->setCurrentIndex(cat);
      if(cat!=0){ obj.insert("category", ui->page_cat->whatsThis());  }
    }else if(ui->stacked_repo->currentWidget() == ui->page_search && !ui->combo_search_cat->currentData().toString().isEmpty() ){
      obj.insert("category", ui->combo_search_cat->currentData().toString()); 
    }
  CORE->communicate(TAG+"pkg_search", "sysadm", "pkg",obj);
  //Now update the search UI page
  if(search!=ui->label_search_term->text()){ ui->scroll_search->verticalScrollBar()->setValue(0); } //new search - go to top
  ui->label_search_term->setText(search); //save this for later
}

void pkg_page::send_start_browse(QString cat){
  //qDebug() << "Browse Category:" << cat;
  if(cat.isEmpty()){ return; }
  if(ui->page_cat->whatsThis()!=cat){
    ui->page_cat->setWhatsThis(cat); //save this for later
    //Reset the scroll widget to the top
    ui->scroll_cat->verticalScrollBar()->setValue(0);
  }
  ui->label_cat->setText( catsToText(QStringList()<<cat).first().section("::::",0,0) );
  QJsonObject obj;
    obj.insert("action","pkg_info");
    obj.insert("repo",ui->combo_repo->currentText());
    obj.insert("category", cat);
  CORE->communicate(TAG+"list_browse", "sysadm", "pkg",obj);
  //qDebug() << " - Sent request:" << obj;
}
