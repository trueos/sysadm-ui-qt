//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_updates.h"
#include "ui_page_updates.h" //auto-generated from the .ui file

#define IDTAG QString("page_updates_")

updates_page::updates_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::updates_ui){
  ui->setupUi(this);
  connect(ui->push_chbranch, SIGNAL(clicked()), this, SLOT(send_change_branch()) );
  connect(ui->list_branches, SIGNAL(currentRowChanged(int)), this, SLOT(check_current_branch()) );
  connect(ui->tree_updates, SIGNAL(itemSelectionChanged()), this, SLOT(check_current_update()) );
}

updates_page::~updates_page(){
  delete ui;
}

//Initialize the CORE <-->Page connections
void updates_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void updates_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Update Manager") );
  //Now run any CORE communications
  send_list_branches();
  send_check_updates();
  check_current_branch();
  check_current_update();
}


// === PRIVATE SLOTS ===
void updates_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(IDTAG)){ return; } //not to be handled here
  qDebug() << "Got Updates reply:" << id << namesp << name << args;
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
    
  }else if(id==IDTAG+"checkup"){
    if(name=="error" || !args.isObject() || !args.toObject().contains("checkupdates") ){ return; }
    QString stat = args.toObject().value("checkupdates").toObject().value("status").toString();
    ui->tree_updates->clear();
    if(stat=="noupdates"){
      qDebug() << "No Updates Available";
    }else if(stat=="rebootrequired"){
      qDebug() << "Reboot Required";
    }else if(stat=="noupdates"){
      qDebug() << "No Updates";
    }else if(stat=="updatesavailable"){
      QStringList types = args.toObject().keys();
	types.removeAll("status");
	for(int i=0; i<types.length(); i++){
	  QJsonObject type = args.toObject().value("checkupdates").toObject().value(types[i]).toObject();
	  QString tname = type.value("name").toString();
	  if(types[i]=="security"){
	    QTreeWidgetItem *tmp = new QTreeWidgetItem();
	      tmp->setText(0, tr("FreeBSD Security Updates"));
	      tmp->setWhatsThis(0, tname);
	      tmp->setCheckState(0,Qt::Unchecked);
	    ui->tree_updates->addTopLevelItem(tmp);
	  }else if(types[i]=="majorupgrade"){
	    QString txt = tr("Major OS Update");
	    if(type.contains("version")){ txt.append(": "+type.value("version").toString()); }
	    if(type.contains("tag")){ txt.append(" -- "+type.value("tag").toString()); }
	    QTreeWidgetItem *tmp = new QTreeWidgetItem();
	      tmp->setText(0, txt );
	      tmp->setWhatsThis(0, tname);
	      tmp->setCheckState(0,Qt::Unchecked);
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
	    QString txt;
	    if(type.contains("size")){ txt = "("+type.value("size").toString()+")"; }
	    if(type.contains("date")){ txt.prepend("-"+type.value("date").toString()); }
	    if(type.contains("tag")){ txt.prepend( type.value("tag").toString()); }
	    if(txt.isEmpty()){ txt = tname; }
	    QTreeWidgetItem *tmp = new QTreeWidgetItem();
	      tmp->setText(0,txt);
	      tmp->setWhatsThis(0,tname);
	      tmp->setCheckState(0,Qt::Unchecked);
	      if(type.contains("details")){ tmp->setToolTip(0, type.value("details").toString() ); }
	    cat->addChild(tmp);
	  }else if(types[i]=="packageupdate"){
	    QTreeWidgetItem *tmp = new QTreeWidgetItem();
	      tmp->setText(0, tr("Package Updates"));
	      tmp->setWhatsThis(0, tname);
	      tmp->setCheckState(0,Qt::Unchecked);
	    ui->tree_updates->addTopLevelItem(tmp);
	  }
	}
    }
    check_current_update();
  }else{
    send_list_branches();
    send_check_updates();
  }
}

void updates_page::send_list_branches(){
  QJsonObject obj;
    obj.insert("action","listbranches");
  CORE->communicate(IDTAG+"listbranch", "sysadm", "update",obj);
}

void updates_page::send_change_branch(){
  if(ui->list_branches->currentItem()==0){ return; }
  QString branch = ui->list_branches->currentItem()->whatsThis();
  //Read off the currently-selected branch
  if(branch.isEmpty()){ return; }
  QJsonObject obj;
    obj.insert("action","startupdate");
    obj.insert("target","chbranch");
    obj.insert("branch", branch);
  CORE->communicate(IDTAG+"chbranch", "sysadm", "update",obj);
}

void updates_page::send_check_updates(){
  QJsonObject obj;
    obj.insert("action","checkupdates");
  CORE->communicate(IDTAG+"checkup", "sysadm", "update",obj);
}

void updates_page::check_current_branch(){
  bool ok = true;
  if(ui->list_branches->currentItem()==0 || ui->list_branches->currentItem()->whatsThis().isEmpty()){ ok = false; }
  ui->push_chbranch->setEnabled(ok);
}

void updates_page::check_current_update(){
  //Get the currently-selected item
  QTreeWidgetItem *it = 0;
  if(!ui->tree_updates->selectedItems().isEmpty()){ it = ui->tree_updates->selectedItems().first(); }
  //Update the details box for the current selection
  ui->text_details->setVisible(ui->group_up_details->isChecked());
  if(it!=0){
    ui->group_up_details->setVisible( !it->whatsThis(0).isEmpty() );
    ui->text_details->setPlainText(it->toolTip(0));
  }else{
    ui->group_up_details->setVisible( false );
  }
  //Now figure out if any updates are checked, and enable the button as needed
  ui->push_start_updates->setEnabled(false);
  
}

// === PRIVATE ===
void updates_page::updateBranchList(QString active, QStringList avail){
  ui->list_branches->clear();
  avail.sort();
  for(int i=0; i<avail.length(); i++){
    QListWidgetItem *tmp = new QListWidgetItem();
      tmp->setText(avail[i]);
      if(avail[i]!=active){ tmp->setWhatsThis(avail[i]); }
      else{ 
	QFont font = tmp->font();
	  font.setBold(true);
	  tmp->setFont( font ); 
	  tmp->setText(tmp->text()+" ("+tr("Current Branch")+")");
      }
    ui->list_branches->addItem(tmp);
  }
  check_current_branch();
}