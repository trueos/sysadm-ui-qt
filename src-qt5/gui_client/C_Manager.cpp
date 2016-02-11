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
  dirs.sort();
  
  on_tree_conn_itemSelectionChanged();
}

void C_Manager::SaveConnectionInfo(){
	
}

//Simplification functions for reading/writing tree widget paths
QTreeWidgetItem* C_Manager::FindItemParent(QString path){
	
}

void C_Manager::saveGroupItem(QTreeWidgetItem *group){
	
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
	
}

void C_Manager::on_push_group_rem_clicked(){
	
}

//SSL Page
void C_Manager::on_push_ssl_create_clicked(){
	
}

void C_Manager::on_push_ssl_import_clicked(){
	
}
