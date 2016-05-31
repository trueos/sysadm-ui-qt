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

// OpenSSL includes
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/err.h>


#define EXPORTFILEDELIM "____^^^^^^^__^^^^^^^_____" //Don't care if this is unreadable - nobody should be viewing this file directly anyway

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
  connect(ui->radio_ssl_bridge, SIGNAL(toggled(bool)), this, SLOT(LoadCertView()) );

  LoadConnectionInfo();
  verify_cert_inputs();
  //Connect some late signals/slots which might be impacted by the default loads
  connect(ui->line_cert_email, SIGNAL(textEdited(const QString&)), this, SLOT(verify_cert_inputs()) );
  connect(ui->line_cert_nick, SIGNAL(textEdited(const QString&)), this, SLOT(verify_cert_inputs()) );
	
  //Show the proper page
  checkFilesLoaded();
  //this->resize(this->sizeHint());
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

void C_Manager::checkFilesLoaded(){
  bool ok = !SSL_cfg.isNull() && !SSL_cfg_bridge.isNull(); //check the validity of the globals
  ui->actionView_Connections->setEnabled( ok );
  ui->tabwidget_ssl_settings->setTabEnabled(1,ok);
  if(!ok ){
    ui->tabwidget_ssl_settings->setCurrentIndex(0);
    ui->actionSetup_SSL->trigger();
  }else{
    ui->actionView_Connections->trigger();
    LoadCertView();
  }
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

// === SSL LIBRARY FUNCTIONS ===
bool C_Manager::generateKeyCertBundle(QString bundlefile, QString nickname, QString email, QString passphrase){
    //Reset/Load some SSL stuff
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

  //qDebug() << "Generate key bundle:" << bundlefile << nickname << email <<  passphrase;
    RSA *rsakey=0;
    X509 *req=0;
    X509_NAME *subj=0;
    EVP_PKEY *pkey=0;
    EVP_MD *digest=0;
	
    //Get a "serial" number
    int serial = 1; //temporary placeholder

    // Generate the RSA key
    rsakey = RSA_generate_key(2048, RSA_F4, NULL, NULL);

    // Create evp obj to hold our rsakey
    if (!(pkey = EVP_PKEY_new())){
        qDebug() << "Could not create EVP object"; return false;
    }
    if (!(EVP_PKEY_set1_RSA(pkey, rsakey))){
        qDebug() << "Could not assign RSA key to EVP object"; return false;
    }
    // create request object
    if (!(req = X509_new())){
        qDebug() << "Failed to create X509_REQ object"; return false;
    }

    X509_set_version(req, NID_X509);

    X509_set_pubkey(req, pkey);

    subj=X509_get_subject_name(req);

    ASN1_INTEGER_set(X509_get_serialNumber(req),serial);

    if (!X509_gmtime_adj(X509_get_notBefore(req), 0)){
       qDebug() << "Error setting start date"; return false;
    }

    if (!X509_gmtime_adj(X509_get_notAfter(req), (long)(31536000*10))){ //10 years
        qDebug() << "Error setting end date"; return false;
    }

    if (X509_set_subject_name(req, subj) != 1){
        qDebug() << "Error adding subject to request"; return false;
    }
    
    X509_NAME_add_entry_by_txt(subj, "C",  MBSTRING_ASC, (unsigned char *)"US", -1, -1, 0);
    X509_NAME_add_entry_by_txt(subj, "O",  MBSTRING_ASC, (unsigned char *)(email.toLocal8Bit().data()), -1, -1, 0);
    X509_NAME_add_entry_by_txt(subj, "CN", MBSTRING_ASC, (unsigned char *)(nickname.toLocal8Bit().data()), -1, -1, 0);
    X509_set_issuer_name(req,subj);
    digest = (EVP_MD *)EVP_sha1();
    if ( 0==X509_sign(req, pkey, digest ) ){
         qDebug() << "Error signing request"; return false;
    }
    
    //PKCS12
    PKCS12 *p12 = PKCS12_new();
    if(p12==NULL){ 
      qDebug() << "Error initializing pkcs12 bundle";
      return false;
    }
    //qDebug() << "pkcs12 passphrase:" << passphrase.toLocal8Bit().data();
    char name[40] = "sysadm-client-";
    strcat(name, QString::number(qrand()%10000).toLocal8Bit().data());
    //qDebug() << "Name:" << name;
    p12 = PKCS12_create(passphrase.toLocal8Bit().data(), name, pkey, req, NULL, 0,0,0,0,0);
    if(p12==NULL){ 
      qDebug() << "Error creating pkcs12 bundle";
      qDebug() << ERR_error_string (ERR_peek_error(), NULL);
      qDebug() << ERR_error_string (ERR_peek_last_error(), NULL); 
      return false;
    }
      BIO * p12Bio = BIO_new(BIO_s_mem());
      i2d_PKCS12_bio(p12Bio,p12);
      BIO_free(p12Bio);
      FILE *fp1 = fopen(bundlefile.toLocal8Bit().data(), "wb") ;
        if (fp1 == 0) {
            qDebug() << "Error opening p12 file";
        }else{
          if( i2d_PKCS12_fp(fp1, p12) ==0 ) {
           qDebug() << "Error writing to p12 file";
	   return false;
          }
          fclose(fp1);
        }
	
    PKCS12_free(p12);
    X509_free(req);
    EVP_PKEY_free(pkey);
    RSA_free(rsakey);
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
    ERR_remove_state(0);
    EVP_cleanup();
    return true;
}

// === PRIVATE SLOTS ===
void C_Manager::changePage(QAction *act){
  if(act==ui->actionView_Connections){ LoadConnectionInfo(); ui->stackedWidget->setCurrentWidget(ui->page_connections); on_tree_conn_itemSelectionChanged(); }
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
    ui->push_conn_reset->setEnabled(false);
  }else{
    ui->push_rename->setEnabled(true);
    ui->push_conn_rem->setEnabled(!sel->whatsThis(0).isEmpty());
    ui->push_conn_reset->setEnabled(!sel->whatsThis(0).isEmpty());
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
  dlg.core->registerForEvents(sysadm_client::SYSSTATE);
  
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

void C_Manager::on_push_conn_reset_clicked(){
  //Get the currently-selected host
  QTreeWidgetItem *sel = 0;
  if(!ui->tree_conn->selectedItems().isEmpty()){ sel = ui->tree_conn->selectedItems().first(); }
  if(sel==0 || sel->whatsThis(0).isEmpty()){ return; }
  QString host = sel->whatsThis(0);
  QString nick = settings->value("Hosts/"+host).toString();
  QString user = settings->value("Hosts/"+host+"/username").toString();
  NewConnectionWizard dlg(this, nick);
    dlg.LoadPrevious(host, user);
  dlg.exec();
  if(!dlg.success){ return; } //cancelled
  //Replace the old core
  if(CORES.contains(dlg.host)){
    CORES[dlg.host]->closeConnection();
    CORES.remove(dlg.host);
  }
  CORES.insert(dlg.host, dlg.core);
  dlg.core->registerCustomCert();
  //update the buttons/tray
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
  //Read in the settings/key files (raw bytes)
  QByteArray sFile, keyFile, bridgeFile;
  QFile file(settings->fileName());
  bool ok = false;
  if(file.open(QIODevice::ReadOnly) ){
    sFile = file.readAll();
    file.close();
    ok = true;
  }
  file.setFileName(SSLFile());
  if(file.open(QIODevice::ReadOnly) ){
    keyFile = file.readAll();
    file.close();
    ok = ok && true;
  }else{
    ok = false;
  }
  file.setFileName(SSLBridgeFile());
  if(file.open(QIODevice::ReadOnly) ){
    bridgeFile = file.readAll();
    file.close();
    ok = ok && true;
  }else{
    ok = false;
  }
  if(!ok){
    QMessageBox::warning(this, tr("Export Error"), tr("Could not read settings and SSL files") );
    return;
  }
  //Base64 encode the files
  sFile = sFile.toBase64();
  keyFile = keyFile.toBase64();
  bridgeFile = bridgeFile.toBase64();
  //Save the encoded bytes/strings to the new file
  file.setFileName(QDir::homePath()+"/sysadm_client.export");
  if(!file.open(QIODevice::WriteOnly)){
    //Error - could not open file
    QMessageBox::warning(this, tr("Export Error"), QString(tr("Could not create export file: %1")).arg(file.fileName()) );
    return;
  }
  file.write(sFile);
  file.write(EXPORTFILEDELIM);
  file.write(keyFile);
  file.write(EXPORTFILEDELIM);
  file.write(bridgeFile);
  file.close();
  //Now alert the user about the new export file
  QMessageBox::information(this, tr("Export Finished"), QString(tr("SysAdm settings export successful.\nFile Location: %1")).arg(file.fileName()) );
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
  bool ok = true;
  //Check email
  QString tmp = ui->line_cert_email->text();
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

  //First get a user-defined passphrase
  QString pass = QInputDialog::getText(this, tr("SSL Passphrase"), tr("Create Password"), QLineEdit::Password);
  if(pass.isEmpty()){ return; }
  QString pass2 = QInputDialog::getText(this, tr("SSL Passphrase"), tr("Retype Password"), QLineEdit::Password );
  while(pass2!=pass && !pass2.isEmpty()){
    pass2 = QInputDialog::getText(this, tr("SSL Passphrase"), tr("(Did not Match) Retype Password"), QLineEdit::Password );
  }
  if(pass2.isEmpty()){ return; }
  QString nick = ui->line_cert_nick->text();
  QString email = ui->line_cert_email->text();
  //Now generate the key/crt file bundle
  bool ok = generateKeyCertBundle(SSLFile(), nick, email, pass);
  if(!ok){ qDebug() << "Could not create/package SSL files"; return;}
  ok = generateKeyCertBundle(SSLBridgeFile(), nick, email, pass2);
  if(!ok){ QFile::remove(SSLFile()); qDebug() << "Could not create/package SSL Bridge files"; return; }
  LoadSSLFile(pass); //go ahead and load the packages into memory for instant usage
  emit SettingsChanged();
  
  //Now update the UI as needed
    //Show the proper page
  checkFilesLoaded();
}

void C_Manager::on_push_ssl_import_clicked(){
  QString filepath = QFileDialog::getOpenFileName(this, tr("Select sysadm export file"), QDir::homePath(), tr("SysAdm settings (*.export)") );
  if(filepath.isEmpty()){ return; } //cancelled
  QFile file(filepath);
  if(!file.open(QIODevice::ReadOnly)){ 
    //Unable to open file (permissions?)
    QMessageBox::warning(this, tr("Import Error"), tr("Could not read selected file. (Check file permissions?)") );	  
    return;
  }
  QByteArray raw = file.readAll();
  file.close();
  //Now split up the raw bytes into the respective files
  if(!raw.contains(EXPORTFILEDELIM)){
    //Invalid file
    QMessageBox::warning(this, tr("Import Error"), tr("File Contents are invalid. This does not appear to be a sysadm export file.") );
    return;
  }
  int delstart = raw.indexOf(EXPORTFILEDELIM);
  QByteArray sFile = raw.left(delstart); raw.remove(0, sFile.length()+QString(EXPORTFILEDELIM).length());
  delstart = raw.indexOf(EXPORTFILEDELIM);
  QByteArray keyFile = raw.left(delstart); raw.remove(0,sFile.length());
  QByteArray bridgeFile = raw.mid(delstart+QString(EXPORTFILEDELIM).length()); //everything after the delimiter
  //Convert the bytes back from base64 encoding
  sFile = QByteArray::fromBase64(sFile);
  keyFile = QByteArray::fromBase64(keyFile);
  bridgeFile = QByteArray::fromBase64(bridgeFile);
  //Now save the files into their proper places
  // - settings file
  file.setFileName(settings->fileName());
  if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ){
    //Could not save settings file (should never happen - this file is always read/write capable in this app by definition)
    QMessageBox::warning(this, tr("Import Error"), tr("Could not overwrite sysadm settings") );
    return; 
  }
  file.write(sFile);
  file.close();
  // - key file
  file.setFileName(SSLFile());
  if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ){
    //Could not save SSL key file (should never happen - this file is always read/write capable in this app by definition)
    QMessageBox::warning(this, tr("Import Error"), tr("Could not overwrite sysadm SSL bundle") );
    return; 
  }
  file.write(keyFile);
  file.close();  
  // - bridge file
  file.setFileName(SSLBridgeFile());
  if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ){
    //Could not save SSL key file (should never happen - this file is always read/write capable in this app by definition)
    QMessageBox::warning(this, tr("Import Error"), tr("Could not overwrite sysadm SSL bundle") );
    return; 
  }
  file.write(bridgeFile);
  file.close();  
  //Now restart the client completely (lots of various things which changed and need to be completely re-loaded)
  QProcess::startDetached(QApplication::arguments()[0], QApplication::arguments());
  QApplication::exit(0);
}

void C_Manager::LoadCertView(){
  if(SSL_cfg.isNull() || SSL_cfg_bridge.isNull()){ return; }
  //qDebug() << "Load Cert View";
  QSslCertificate cert;
  if(ui->radio_ssl_bridge->isChecked()){ cert = SSL_cfg_bridge.localCertificate(); }
  else{ cert = SSL_cfg.localCertificate(); }
  ui->text_ssl_view->setPlainText( cert.toText() );
}

void C_Manager::on_push_ssl_cert_to_file_clicked(){
  qDebug() << "save public cert to file";
  //First load the key and filename based on which cert is being viewed
  QByteArray pubkey;
  QString filename;
  if(ui->radio_ssl_bridge->isChecked()){ 
    pubkey = SSL_cfg_bridge.localCertificate().publicKey().toPem(); 
    filename = "sysadm-client-to-bridge.key";
  }else{ 
    pubkey = SSL_cfg.localCertificate().publicKey().toPem(); 
    filename = "sysadm-client-to-server.key";
  }
  //Now save the key to file
  QString dir = QDir::homePath();
  if(QFile::exists(dir+"/Desktop/") ){ dir.append("/Desktop/"); }
  QFile file(dir+filename);
  if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){ 
    qDebug() << "Could not save file:" << dir+filename;
    return;
  }
  file.write(pubkey);
  file.close();
  qDebug() << "Public Key saved to file:" << dir+filename;
  
}
