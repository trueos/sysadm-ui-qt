//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "C_Manager.h"
#include "ui_C_Manager.h"

C_Manager::C_Manager() : QMainWindow(), ui(new Ui::C_Manager){
  ui->setupUi(this);
  radio_acts = new QActionGroup(this);
    radio_acts->addAction(ui->actionView_Connections);
    radio_acts->addAction(ui->actionSetup_SSL);
  connect(radio_acts, SIGNAL(triggered(QAction*)), this, SLOT(changePage(QAction*)) );
  //put a spacer between the finished action and the others
  QWidget *tmp = new QWidget(this);
    tmp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  ui->toolBar->insertWidget(ui->actionView_Connections, tmp);
	
  //Setup all the connections
  connect(ui->actionFinished, SIGNAL(triggered()), this, SLOT(close()) );
	
  //Show the ssl page by default
  ui->actionSetup_SSL->trigger();
}

C_Manager::~C_Manager(){
	
}

// === PRIVATE ===
void C_Manager::LoadConnectionInfo(){
  ui->tree_conn->clear();
  //Load the files/settings and put it into the tree
  QStringList dirs = settings->allKeys().filter("C_Groups/");
  dirs.sort(); //this will ensure that we decend through the tree progressively
  for(int i=0; i<dirs.length(); i++){
    //Create the item for this dir
    QTreeWidgetItem *item = new QTreeWidgetItem();
      item->setText(0, dirs[i].section("/",-1)); //dirs only have the "text" field set
      item->setIcon(0,QIcon(":/icons/black/box.svg") );
    //Load all the connections within this dir
    QStringList conns = settings->value(dirs[i]).toStringList();
    for(int c=0; c<conns.length(); c++){
       QTreeWidgetItem *tmp = new QTreeWidgetItem();
	    tmp->setText(0, conns[c]); //this needs to be changed to a nickname/IP later
	    tmp->setWhatsThis(0, conns[c]);
	    tmp->setIcon(0,QIcon(":/icons/black/globe.svg") );
	item->addChild(tmp);
    }
    //Now add the item to the proper parent
    QTreeWidgetItem *parent = FindItemParent(dirs[i]);
    if(parent==0){
      ui->tree_conn->addTopLevelItem(item);
    }else{
      parent->addChild(item);
    }
  }
  on_tree_conn_itemSelectionChanged();
}

void C_Manager::SaveConnectionInfo(){
  //First clear the current tree from the settings file
  QStringList dirs = settings->allKeys().filter("C_Groups/");
  for(int i=0; i<dirs.length(); i++){ settings->remove(dirs[i]); }
  //Now go through the top-level item and let it save down through the tree
  QString topConns;
  for(int i=0; i<ui->tree_conn->topLevelItemCount(); i++){
    if(ui->tree_conn->topLevelItem(i)->whatsThis(0).isEmpty()){
      saveGroupItem(ui->tree_conn->topLevelItem(i)); //will recursively go through dirs
    }else{
      topConns = ui->tree_conn->topLevelItem(i)->whatsThis(0);
    }
  }
  //Now save these top-level items
  settings->setValue("C_Groups",topConns);
  //Let the tray know about the changes
  emit SettingsChanged();
}

//Simplification functions for reading/writing tree widget paths
QTreeWidgetItem* C_Manager::FindItemParent(QString path){
  QString ppath = path.section("/",1,-2); //Cut off the C_Groups/ and current dir from the ends
  QList<QTreeWidgetItem*> found = ui->tree_conn->findItems(ppath.section("/",-1), Qt::MatchExactly);
  for(int i=0; i<found.length(); i++){
    QString check = found[i]->text(0);
    QTreeWidgetItem *tmp = found[i];
    while(tmp->parent()!=0){
      tmp = tmp->parent();
      check.prepend(tmp->text(0)+"/");
    }
    if(ppath==check){ return found[i]; } //found the parent item
  }
  return 0; //none found
}

void C_Manager::saveGroupItem(QTreeWidgetItem *group){
    //First assemble the full path of the group
    QString path = group->text(0);
    QTreeWidgetItem *tmp = group;
    while(tmp->parent()!=0){
      tmp = tmp->parent();
      path.prepend(tmp->text(0)+"/");
    }
    path.prepend("C_Groups/");
    //Now get all the children of this group which are not groups themselves
    QStringList conns;
    for(int i=0; i<group->childCount(); i++){
      if(group->child(i)->whatsThis(0).isEmpty()){ saveGroupItem(group->child(i)); }
      conns << group->child(i)->whatsThis(0);
    }
    //Now save the info to the settings file
    settings->setValue(path, conns);
}


// === PRIVATE SLOTS ===
void C_Manager::changePage(QAction *act){
  if(act==ui->actionView_Connections){ ui->stackedWidget->setCurrentWidget(ui->page_connections); on_tree_conn_itemSelectionChanged(); }
  else{ ui->stackedWidget->setCurrentWidget(ui->page_ssl); }
}

//Connections Page
void C_Manager::on_tree_conn_itemSelectionChanged(){
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }
  //Now enable/disable the buttons as needed
  if(sel==0){
    ui->push_conn_rem->setEnabled(false);
    ui->push_group_rem->setEnabled(false);
    ui->push_rename->setEnabled(false);
  }else{
    ui->push_rename->setEnabled(true);
    ui->push_conn_rem->setEnabled(!sel->whatsThis(0).isEmpty());
    ui->push_group_rem->setEnabled(sel->whatsThis(0).isEmpty() && sel->childCount()==0);
  }
}

void C_Manager::on_push_conn_add_clicked(){
	
}

void C_Manager::on_push_conn_rem_clicked(){
	
}

void C_Manager::on_push_conn_export_clicked(){
	
}

void C_Manager::on_push_group_add_clicked(){
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }
  QString gname = QInputDialog::getText(this, tr("New Group Name"), tr("Name:"));
  while(!gname.isEmpty() && !ui->tree_conn->findItems(gname, Qt::MatchExactly).isEmpty()){
    gname = QInputDialog::getText(this, tr("Invalid Group Name"), tr("Name:"),QLineEdit::Normal,gname);
  }
  if(gname.isEmpty()){ return; } //cancelled
  //Create the new item
  QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << gname);
    item->setIcon(0,QIcon(":/icons/black/box.svg") );
  //Now add the item in the proper place
  if(sel==0 || !sel->whatsThis(0).isEmpty()){
    //New Top-level group
    ui->tree_conn->addTopLevelItem(item);
  }else{
    //new Subgroup
    sel->addChild(item);
    if(!sel->isExpanded()){ ui->tree_conn->expandItem(sel); }
    sel->setSelected(false);
  }
  item->setSelected(true);
  //Now update the buttons
  on_tree_conn_itemSelectionChanged();
}

void C_Manager::on_push_group_rem_clicked(){
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }
  if(sel==0 || !sel->whatsThis(0).isEmpty() || sel->childCount()>0 ){ return; }
  delete sel;
  //Now update the buttons
  on_tree_conn_itemSelectionChanged();
}

void C_Manager::on_push_rename_clicked(){
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }	
  if(sel==0){ return; }
  QString oname = sel->text(0);
  QString id = sel->whatsThis(0);
  if(!id.isEmpty()){ oname = oname.section("("+id,0,0); }
  QString name = QInputDialog::getText(this, tr("New Name"), tr("Name:"), QLineEdit::Normal, oname);
  while(!name.isEmpty() && ( !ui->tree_conn->findItems(name, Qt::MatchExactly).isEmpty() || !ui->tree_conn->findItems(name+" (*)", Qt::MatchWildcard).isEmpty() ) ){
    name = QInputDialog::getText(this, tr("Invalid Name"), tr("Name:"),QLineEdit::Normal,name);
  }
  if(name.isEmpty()){ return; } //cancelled
  if(!id.isEmpty()){ name = name+" ("+id+")"; }
  sel->setText(0,name);
}

//SSL Page
void C_Manager::on_push_ssl_create_clicked(){
	
}

void C_Manager::on_push_ssl_import_clicked(){
	
}
