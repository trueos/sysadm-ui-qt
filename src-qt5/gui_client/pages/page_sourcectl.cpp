//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_sourcectl.h"
#include "ui_page_sourcectl.h" //auto-generated from the .ui file

#define IDTAG QString("page_sourcectl_")

sourcectl_page::sourcectl_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::sourcectl_ui){
  ui->setupUi(this);
/*
 *   ui->page_stat_layout->setStretchFactor(ui->text_up_log, 2);
  connect(ui->push_start_updates, SIGNAL(clicked()), this, SLOT(check_start_updates()) );
  connect(ui->push_stop_updates, SIGNAL(clicked()), this, SLOT(send_stop_updates()) );
  connect(ui->push_settings_save, SIGNAL(clicked()), this, SLOT(send_save_settings()) );
  connect(ui->push_chbranch, SIGNAL(clicked()), this, SLOT(send_change_branch()) );
  connect(ui->list_branches, SIGNAL(currentRowChanged(int)), this, SLOT(check_current_branch()) );
  connect(ui->tree_updates, SIGNAL(itemSelectionChanged()), this, SLOT(check_current_update()) );
  connect(ui->tree_updates, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(check_current_update_item(QTreeWidgetItem*)) );
  connect(ui->list_logs, SIGNAL(currentRowChanged(int)), this, SLOT(send_read_log()) );
  connect(ui->group_up_details, SIGNAL(toggled(bool)), this, SLOT(check_current_update()) );
  ui->stacked_updates->setCurrentWidget(ui->page_updates); //always start on this page - has the "checking" notice
  ui->tabWidget->setTabEnabled(1, false); //disable the "branches" tab by default - will be enabled if/when branches become available
  ui->tabWidget->setCurrentIndex(0);
  connect(ui->radio_repo_custom, SIGNAL(toggled(bool)), ui->group_settings_customrepo, SLOT(setEnabled(bool)) );
}

sourcectl_page::~sourcectl_page(){
  delete ui;
}

//Initialize the CORE <-->Page connections
void sourcectl_page::setupCore(){
  CORE->registerForEvents(sysadm_client::DISPATCHER, true);
}

//Page embedded, go ahead and startup any core requests
void sourcectl_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Update Manager") );
  //Now run any CORE communications
  send_list_logs();
}


// === PRIVATE SLOTS ===
void sourcectl_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(IDTAG)){ return; } //not to be handled here
  //qDebug() << "Got Updates reply:" << id << namesp << name << args;
  if(id==IDTAG+"listbranch"){
    if(name=="error" || !args.isObject() || !args.toObject().contains("listbranches") ){ return; }
    QJsonObject obj = args.toObject().value("listbranches").toObject();
    QString active;
    QStringList avail = obj.keys();
    //Now figure out which one is currently active
    for(int i=0; i<avail.length(); i++){
      if(obj.value(avail[i]).toString()=="active"){
        active = avail[i];
	break;
      }
    }
    updateBranchList(active, avail);
  }else if(id==IDTAG+"startup" && name!="error"){
    //update started successfully - wait for the dispatcher event to do the next UI update
  }else if(id==IDTAG+"checkup"){
    if(name=="error" || !args.isObject() || !args.toObject().contains("checkupdates") ){ return; }
    QString stat = args.toObject().value("checkupdates").toObject().value("status").toString();
    ui->tree_updates->clear();
    qDebug() << "Got update check:" << stat;
    ui->frame_lastcheck->setVisible( args.toObject().value("checkupdates").toObject().contains("last_check") );
    if(args.toObject().value("checkupdates").toObject().contains("last_check")){
      QString text = tr("Latest Check: %1");
      ui->label_lastcheck->setText( text.arg(QDateTime::fromString(args.toObject().value("checkupdates").toObject().value("last_check").toString() , Qt::ISODate).toString(Qt::DefaultLocaleLongDate)) ); 
    }
    if(stat=="noupdates"){
      ui->stacked_updates->setCurrentWidget(ui->page_stat);
      ui->label_uptodate->setVisible(true);
      ui->label_rebootrequired->setVisible(false);
    }else if(stat=="rebootrequired"){
      ui->frame_lastcheck->setVisible(false);
      ui->stacked_updates->setCurrentWidget(ui->page_stat);
      ui->label_uptodate->setVisible(false);
      ui->label_rebootrequired->setVisible(true);
    }else if(stat=="updaterunning"){
      ui->frame_lastcheck->setVisible(false);
      ui->stacked_updates->setCurrentWidget(ui->page_uprunning);
    }else if(stat=="updatesavailable"){
      ui->stacked_updates->setCurrentWidget(ui->page_updates);
      QStringList types = args.toObject().value("checkupdates").toObject().keys();
      QString fulldetails = args.toObject().value("checkupdates").toObject().value("details").toString();
	types.removeAll("status");
	//qDebug() << "Types:" << types;
	for(int i=0; i<types.length(); i++){
	  QJsonObject type = args.toObject().value("checkupdates").toObject().value(types[i]).toObject();
	  QString tname = type.value("name").toString();
	  if(types[i]=="security"){
	    QTreeWidgetItem *tmp = new QTreeWidgetItem();
	      tmp->setText(0, tr("FreeBSD Security Updates"));
	      tmp->setWhatsThis(0, "fbsdupdate");
	      tmp->setCheckState(0,Qt::Unchecked);
              tmp->setToolTip(0,fulldetails);
	    ui->tree_updates->addTopLevelItem(tmp);
	  }else if(types[i]=="majorupgrade"){
	    QString txt = tr("Major OS Update"); QString tag = tname;
	    if(type.contains("version")){ txt.append(": "+type.value("version").toString()); }
	    if(type.contains("tag")){ tag = type.value("tag").toString(); txt.append(" -- "+tag); }
	    QTreeWidgetItem *tmp = new QTreeWidgetItem();
	      tmp->setText(0, txt );
	      tmp->setWhatsThis(0, "standalone-major::"+tag);
	      tmp->setCheckState(0,Qt::Unchecked);
              tmp->setToolTip(0,fulldetails);
	    ui->tree_updates->addTopLevelItem(tmp);
	  }else if(types[i].startsWith("patch")){
	    //See if the patch category has been created yet first
	    QTreeWidgetItem *cat = 0;
	    if(!ui->tree_updates->findItems(tr("Standalone Patches"),Qt::MatchExactly).isEmpty()){
	      cat = ui->tree_updates->findItems(tr("Standalone Patches"),Qt::MatchExactly).first();
	    }else{
	      cat = new QTreeWidgetItem();
		cat->setText(0, tr("Standalone Patches"));
		cat->setCheckState(0,Qt::Unchecked);
	      ui->tree_updates->addTopLevelItem(cat);
	    }
	    //Now create the child patch
	    QStringList txt; QString tag;
	    if(type.contains("size")){ txt << QString(tr("Size: %1")).arg( type.value("size").toString()); }
	    if(type.contains("date")){ txt << QString(tr("Date: %1")).arg(type.value("date").toString()); }
	    if(type.contains("tag")){ tag = type.value("tag").toString(); txt << QString(tr("Tag: %1")).arg(tag); }
	    if(type.contains("details")){txt << QString(tr("Additional Details: %1")).arg(type.value("details").toString()); }
	    if(tag.isEmpty()){ tag = tname; }
	    QTreeWidgetItem *tmp = new QTreeWidgetItem();
	      tmp->setText(0,tname);
	      tmp->setWhatsThis(0,"standalone::"+tag);
	      tmp->setCheckState(0,Qt::Unchecked);
	      tmp->setToolTip(0, txt.join("\n"));
	    cat->addChild(tmp);
	  }else if(types[i]=="packageupdate"){
	    QTreeWidgetItem *tmp = new QTreeWidgetItem();
	      tmp->setText(0, tr("Package Updates"));
	      tmp->setWhatsThis(0, "pkgupdate");
	      tmp->setCheckState(0,Qt::Unchecked);
              tmp->setToolTip(0,fulldetails);
	    ui->tree_updates->addTopLevelItem(tmp);
	  }
	}
    } //end status update type
    ui->tree_updates->sortItems(0,Qt::AscendingOrder);
    if(ui->tree_updates->topLevelItemCount()>0){ ui->tree_updates->setCurrentItem( ui->tree_updates->topLevelItem(0) ); }
    ui->page_updates->setEnabled(true);
    ui->label_checking->setVisible(false);
    check_current_update();

  }else if(id==IDTAG+"stop_updates"){
    //Just finished stopping the current updates
    send_check_updates();
  }else if(id==IDTAG+"list_settings"){
    QJsonObject obj = args.toObject().value("listsettings").toObject();
    int mbe = 5;
    if(obj.contains("maxbe")){ mbe = qRound( obj.value("maxbe").toString().toFloat() ); }
    ui->spin_maxbe->setValue(mbe);
    bool autoup = true;
    if(obj.contains("auto_update")){ autoup = !(obj.value("auto_update").toString().toLower()=="disabled"); }
    ui->check_settings_autoup->setChecked(autoup);
    autoup = false;
    if(obj.contains("auto_update_reboot")){
      int hour = obj.value("auto_update_reboot").toString().toInt(&autoup);
      if(hour>=0 && hour<24){ ui->time_auto_reboot->setTime(QTime(hour,0)); }
    }
    ui->check_auto_reboot->setChecked(autoup);
    QString  repo = "STABLE";
    if(obj.contains("package_set")){ repo = obj.value("package_set").toString().toUpper(); }
    if(repo=="CUSTOM"){ ui->radio_repo_custom->setChecked(true); }
    else if(repo=="UNSTABLE"){ ui->radio_repo_unstable->setChecked(true); }
    else{ ui->radio_repo_stable->setChecked(true); }
    ui->group_settings_customrepo->setEnabled(ui->radio_repo_custom->isChecked());
    ui->line_settings_url->setText( obj.value("package_url").toString() ); //normally empty/nonexistant

  }else if(id==IDTAG+"list_logs"){
    ui->list_logs->clear();
    QStringList logs = args.toObject().value("listlogs").toObject().keys();
    QString label = tr("%1");
    for(int i=0; i<logs.length(); i++){
      QJsonObject info = args.toObject().value("listlogs").toObject().value(logs[i]).toObject();
      QListWidgetItem *it = new QListWidgetItem( label.arg(info.value("name").toString()) );
        it->setWhatsThis(logs[i]);
      ui->list_logs->addItem(it);
    }
  }else if(id==IDTAG+"read_log"){
    ui->text_log->clear();
    QStringList keys = args.toObject().value("readlogs").toObject().keys();
    QString clog;
    if(ui->list_logs->currentItem()!=0){ clog = ui->list_logs->currentItem()->whatsThis(); }
    if(keys.contains(clog)){
      ui->text_log->setWhatsThis(clog);
      ui->text_log->setPlainText( args.toObject().value("readlogs").toObject().value(clog).toString() );
    }

  }else{
    send_list_branches();
    send_check_updates();
    send_list_settings();
    send_list_logs();
  }
}

void sourcectl_page::ParseEvent(sysadm_client::EVENT_TYPE evtype, QJsonValue val){
  if(evtype==sysadm_client::DISPATCHER && val.isObject()){
    //qDebug() << "Got Dispatcher Event:" << val;
    if(val.toObject().value("event_system").toString()=="sysadm/update"){
      QString state = val.toObject().value("state").toString();
      if(state=="finished"){
        send_start_updates(); //see if there is another update waiting to start, refresh otherwise
      }
      //Update the log widget
      ui->text_up_log->setPlainText( ui->text_up_log->toPlainText() + val.toObject().value("update_log").toString() ); //text
      ui->text_up_log->moveCursor(QTextCursor::End);
      ui->text_up_log->ensureCursorVisible();
      //qDebug() << "Got update event:" << state << val;
      ui->stacked_updates->setCurrentWidget(ui->page_uprunning);
    } //end sysadm/update check
  } //end dispatcher event check
}

void sourcectl_page::send_list_logs(){
  QJsonObject obj;
    obj.insert("action","listlogs");
  communicate(IDTAG+"list_logs", "sysadm", "update",obj);
}

void sourcectl_page::send_read_log(){
  if(ui->list_logs->currentItem()==0){ return; }
  QString citem = ui->list_logs->currentItem()->whatsThis();
  QJsonObject obj;
    obj.insert("action","readlogs");
    obj.insert("logs", citem);
  communicate(IDTAG+"read_log", "sysadm", "update",obj);
}


void sourcectl_page::on_pushButton_DownloadPorts_clicked(){
    QJsonObject obj;
     obj.insert("action","downloadports");
     if(up.startsWith("standalone::")){
       obj.insert("target","standalone");
       obj.insert("tag", up.section("standalone::",-1));
     }else{
       obj.insert("target", up);
     }
   qDebug() << "Send downloadports request:" << obj;
   communicate(IDTAG+"startup", "sysadm", "source",obj);
   //Update the UI right away (so the user knows it is working)
   qDebug() << "Sending update request";
     ui->portstree_log->clear();
}

void sourcectl_page::on_pushButton_UpdatePorts_clicked(){

}

void sourcectl_page::on_pushButton_DeletePorts_clicked(){

}

void sourcectl_page::on_pushButton_DownloadSource_clicked(){

}

void sourcectl_page::on_pushButton_UpdateSource_clicked(){

}

void sourcectl_page::on_pushButton_DeleteSource_clicked(){

}

*/
}
