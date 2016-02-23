//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "C_Manager.h"
#include "ui_C_Manager.h"

#include "NewConnectionWizard.h"

#include <globals.h>

extern QHash<QString,sysadm_client*> CORES; // hostIP / core

C_Manager::C_Manager() : QMainWindow(), ui(new Ui::C_Manager){
  ui->setupUi(this);
  treeTimer = new QTimer(this);
    treeTimer->setSingleShot(true);
    treeTimer->setInterval(500); // 1/2 second
	
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
  connect(treeTimer, SIGNAL(timeout()), this, SLOT(SaveConnectionInfo()) );
  connect(ui->tree_conn->model(), SIGNAL(rowsInserted(const QModelIndex&,int,int)), this, SLOT(tree_items_changed()) );
	
  LoadConnectionInfo();
  verify_cert_inputs();
  //Connect some signals/slots
  connect(ui->line_cert_country, SIGNAL(textEdited(const QString&)), this, SLOT(verify_cert_inputs()) );
  connect(ui->line_cert_state, SIGNAL(textEdited(const QString&)), this, SLOT(verify_cert_inputs()) );
  connect(ui->line_cert_email, SIGNAL(textEdited(const QString&)), this, SLOT(verify_cert_inputs()) );
  connect(ui->line_cert_nick, SIGNAL(textEdited(const QString&)), this, SLOT(verify_cert_inputs()) );
	
  //Show the proper page
  if( !QFile::exists(SSLFile()) ){
    ui->actionView_Connections->setEnabled( false );
    ui->actionSetup_SSL->trigger();
  }else{
    ui->actionView_Connections->trigger();
  }
}

C_Manager::~C_Manager(){
	
}

// === PRIVATE ===
void C_Manager::LoadConnectionInfo(){
  ui->tree_conn->clear();
  //Load the files/settings and put it into the tree
  QStringList dirs = settings->allKeys().filter("C_Groups/");
  dirs.prepend("C_Groups"); //also need the "base" dir
  dirs.sort(); //this will ensure that we decend through the tree progressively
  //qDebug() << "Load Dirs:" << dirs;
  for(int i=0; i<dirs.length(); i++){
    //Create the item for this dir
    QTreeWidgetItem *item = 0;
    if(dirs[i]!="C_Groups"){ //real sub-dir
      item = new QTreeWidgetItem(); 
      item->setText(0, dirs[i].section("/",-1)); //dirs only have the "text" field set
      item->setIcon(0,QIcon(":/icons/black/box.svg") );
      //Now add the item to the proper parent
      QTreeWidgetItem *parent = FindItemParent(dirs[i]);
      if(parent==0){
        ui->tree_conn->addTopLevelItem(item);
      }else{
        parent->addChild(item);
      }
    }
    //Load all the connections within this dir
    QStringList conns = settings->value(dirs[i]).toStringList();
    //qDebug() << "Check Connections:" << dirs[i] << conns;
    for(int c=0; c<conns.length(); c++){
      if(conns[c].simplified().isEmpty()){ continue; }
       QTreeWidgetItem *tmp = new QTreeWidgetItem();
	    tmp->setText(0, settings->value("Hosts/"+conns[c],conns[c]).toString() + " ("+conns[c]+")");
	    tmp->setWhatsThis(0, conns[c]);
	    tmp->setIcon(0,QIcon(":/icons/black/globe.svg") );
        if(item==0){ ui->tree_conn->addTopLevelItem(tmp); }
	else{ item->addChild(tmp); }
    }
    QApplication::processEvents();
  }
  on_tree_conn_itemSelectionChanged();
}

void C_Manager::SaveConnectionInfo(){
  //First clear the current tree from the settings file
  QStringList dirs = settings->allKeys().filter("C_Groups/");
  for(int i=0; i<dirs.length(); i++){ settings->remove(dirs[i]); }
  //Now go through the top-level item and let it save down through the tree
  QStringList topConns;
  for(int i=0; i<ui->tree_conn->topLevelItemCount(); i++){
    if(ui->tree_conn->topLevelItem(i)->whatsThis(0).isEmpty()){
      saveGroupItem(ui->tree_conn->topLevelItem(i)); //will recursively go through dirs
    }else{
      topConns << ui->tree_conn->topLevelItem(i)->whatsThis(0);
    }
  }
  //Now save these top-level items
  settings->setValue("C_Groups",topConns);
  //Let the tray know about the changes
  emit SettingsChanged();
  //Make sure the buttons are updated as needed
  on_tree_conn_itemSelectionChanged(); 
}

//Simplification functions for reading/writing tree widget paths
QTreeWidgetItem* C_Manager::FindItemParent(QString path){
  QString ppath = path.section("/",1,-2); //Cut off the C_Groups/ and current dir from the ends
  //qDebug() << "Item Parent:" << path << ppath;
  QList<QTreeWidgetItem*> found = ui->tree_conn->findItems(ppath.section("/",-1), Qt::MatchExactly | Qt::MatchRecursive);
  //qDebug() << "Matches Found:" << found;
  for(int i=0; i<found.length(); i++){
    QString check = found[i]->text(0);
    QTreeWidgetItem *tmp = found[i];
    while(tmp->parent()!=0){
      tmp = tmp->parent();
      check.prepend(tmp->text(0)+"/");
    }
    //qDebug() << "Check parent path:" << ppath << check;
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

void C_Manager::tree_items_changed(){
  //Don't care about inputs
  if(treeTimer->isActive()){ treeTimer->stop(); }
  treeTimer->start();
}

void C_Manager::on_push_conn_add_clicked(){
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }
  QString gname = QInputDialog::getText(this, tr("New Connection Nickname"), tr("Name:"));
  while(!gname.isEmpty() && !ui->tree_conn->findItems(gname+" (*)", Qt::MatchWildcard | Qt::MatchRecursive).isEmpty() ){
    gname = QInputDialog::getText(this, tr("Invalid Name"), tr("Name:"),QLineEdit::Normal,gname);
  }
  if(gname.isEmpty()){ return; } //cancelled
  //Now run through the new connection wizard
  NewConnectionWizard dlg(this, gname);
  dlg.exec();
  if(!dlg.success){ return; } //cancelled
  CORES.insert(dlg.host, dlg.core);
  dlg.core->registerCustomCert();
  //Create the new item
  QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << (gname+" ("+dlg.host+")"));
    item->setWhatsThis(0,dlg.host);
    item->setIcon(0,QIcon(":/icons/black/globe.svg") );
  //Now add the item in the proper place
  if(sel==0 || !sel->whatsThis(0).isEmpty()){
    //New Top-level connection
    ui->tree_conn->addTopLevelItem(item);
  }else{
    //New Connection in a group
    sel->addChild(item);
    if(!sel->isExpanded()){ ui->tree_conn->expandItem(sel); }
    sel->setSelected(false);
  }
  item->setSelected(true);
  //Now update the buttons
  on_tree_conn_itemSelectionChanged();
  SaveConnectionInfo();
}

void C_Manager::on_push_conn_rem_clicked(){
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }
  if(sel==0 || sel->whatsThis(0).isEmpty()){ return; }
  QString host = sel->whatsThis(0);
  //Turn off the connection for this host first
  if(CORES.contains(host)){ 
    CORES[host]->closeConnection();
    QApplication::processEvents();
    delete CORES.take(host);
  }
  //Clean up the save info for this connection
  settings->remove("Hosts/"+host);
  settings->remove("Hosts/"+host+"/username");
  //Remove the item from the tree
  delete sel;
  //update the buttons/tray
  on_tree_conn_itemSelectionChanged();
  SaveConnectionInfo();  
}

void C_Manager::on_push_conn_export_clicked(){
	
}

void C_Manager::on_push_group_add_clicked(){
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }
  QString gname = QInputDialog::getText(this, tr("New Group Name"), tr("Name:"));
  while(!gname.isEmpty() && !ui->tree_conn->findItems(gname, Qt::MatchExactly | Qt::MatchRecursive).isEmpty()){
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
  SaveConnectionInfo();
}

void C_Manager::on_push_group_rem_clicked(){
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }
  if(sel==0 || !sel->whatsThis(0).isEmpty() || sel->childCount()>0 ){ return; }
  delete sel;
  //Now update the buttons
  on_tree_conn_itemSelectionChanged();
  SaveConnectionInfo();
}

void C_Manager::on_push_rename_clicked(){
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }	
  if(sel==0){ return; }
  QString oname = sel->text(0);
  QString id = sel->whatsThis(0);
  if(!id.isEmpty()){ oname = oname.section("("+id,0,0); }
  QString name = QInputDialog::getText(this, tr("New Name"), tr("Name:"), QLineEdit::Normal, oname);
  while(!name.isEmpty() && ( !ui->tree_conn->findItems(name, Qt::MatchExactly | Qt::MatchRecursive).isEmpty() || !ui->tree_conn->findItems(name+" (*)", Qt::MatchWildcard | Qt::MatchRecursive).isEmpty() ) ){
    name = QInputDialog::getText(this, tr("Invalid Name"), tr("Name:"),QLineEdit::Normal,name);
  }
  if(name.isEmpty()){ return; } //cancelled
  settings->setValue("Hosts/"+id, name);
  if(!id.isEmpty()){ name = name+" ("+id+")"; }
  sel->setText(0,name);
  SaveConnectionInfo();
}

//SSL Page
void C_Manager::verify_cert_inputs(){
  bool ok = false;
  // Check country code
  ok = (ui->line_cert_country->text().length()==2);
  //Check state
  QString tmp = ui->line_cert_state->text();
    tmp.replace(" ","_");
    ui->line_cert_state->setText(tmp);
  ok = ok && !tmp.isEmpty();
  //Check email
  tmp = ui->line_cert_email->text();
    tmp.replace(" ","_");
    ui->line_cert_email->setText(tmp);
  ok = ok && !tmp.isEmpty() && tmp.contains("@");
  // Check Nickname
  tmp = ui->line_cert_nick->text();
    tmp.replace(" ","_");
    ui->line_cert_nick->setText(tmp);
  ok = ok && !tmp.isEmpty();
	
  ui->push_ssl_create->setEnabled(ok);	
}

void C_Manager::on_push_ssl_create_clicked(){
  //First find the proper openssl binary
  QString bin = "openssl";
  #ifdef __FreeBSD__
    //use the ports/pkg version (LibreSSL?) instead, falling back on the OS-version as needed
    if(QFile::exists("/usr/local/bin/openssl")){ bin = "/usr/local/bin/openssl"; }
  #endif
  //Now generate the temporary file paths
  QDir tempdir = QDir::temp();
  QString keypath = tempdir.absoluteFilePath("sysadm_ssl.key");
  QString certpath = tempdir.absoluteFilePath("sysadm_ssl.crt");
  //Now get a user-defined passphrase
  QString pass = QInputDialog::getText(this, tr("SSL Passphrase"), tr("Create Password"), QLineEdit::Password);
  if(pass.isEmpty()){ return; }
  QString pass2 = QInputDialog::getText(this, tr("SSL Passphrase"), tr("Retype Password"), QLineEdit::Password );
  while(pass2!=pass && !pass2.isEmpty()){
    pass2 = QInputDialog::getText(this, tr("SSL Passphrase"), tr("(Did not Match) Retype Password"), QLineEdit::Password );
  }
  if(pass2.isEmpty()){ return; }
  qDebug() << "New SSL Files:" << bin << keypath << certpath << pass;
  //Now generate the key/crt files
  QString subject =  "/C="+ui->line_cert_country->text()+"/ST="+ui->line_cert_state->text()+"/L=NULL/O=SysAdm-client/OU=SysAdm-client/CN="+ui->line_cert_nick->text()+"/emailAddress="+ui->line_cert_email->text();
  bool ok = (0==QProcess::execute(bin, QStringList() << "req" << "-batch" << "-newkey" << "rsa:2048" << "-nodes" << "-keyout" << keypath << "-new" << "-x509" << "-out" << certpath << "-subj" << subject) );
  if(!ok){ qDebug() << "[ERROR] Could not generate key/crt files"; return;}
  //Now package them as a PKCS12 file with passphrase-encryption
  // - need a temporary file for passing in the encryption key/phrase
  QTemporaryFile tmpfile(tempdir.absoluteFilePath(".XXXXXXXXXXXXXXXXXXX"));
    tmpfile.open();
    QTextStream in(&tmpfile); in << pass; 
    tmpfile.close();
  qDebug() << "Temporary File:" << tmpfile.fileName();
  ok = (0==QProcess::execute(bin, QStringList() << "pkcs12" << "-inkey" << keypath <<"-in" << certpath << "-export" << "-passout" << "file:"+tmpfile.fileName() << "-out" << SSLFile() ) );
  QFile::remove(keypath); QFile::remove(certpath);
  if(!ok){ qDebug() << "Could Not package key/cert files"; return;}
  else{ LoadSSLFile(pass); }//go ahead and load the package into memory for instant usage
  emit SettingsChanged();
  
  //Now update the UI as needed
    //Show the proper page
  ui->actionView_Connections->setEnabled(ok);
  if(ok){ ui->actionView_Connections->trigger(); }
}

void C_Manager::on_push_ssl_import_clicked(){
	
}
