//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_ssl_auth.h"
#include "ui_page_ssl_auth.h" //auto-generated from the .ui file

ssl_auth_page::ssl_auth_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::page_ssl_auth_ui){
  ui->setupUi(this);	
}

ssl_auth_page::~ssl_auth_page(){
  
}

//Initialize the CORE <-->Page connections
void ssl_auth_page::setupCore(){

}

//Page embedded, go ahead and startup any core requests
void ssl_auth_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("SSL Key Manager") );
  //Now run any CORE communications
  requestList();
}

// === PRIVATE ===
void ssl_auth_page::requestList(){
  QJsonObject obj;
    obj.insert("action","list_ssl_certs");
  communicate("client_ssl_page_list", "rpc", "settings", obj);
}

// === PRIVATE SLOTS ===
void ssl_auth_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  qDebug() << "New Reply:" << id << namesp << name << args;
  if(id=="client_ssl_page_revoke"){
    requestList();
  }else if(id=="client_ssl_page_list" && name!="error"){
    if(args.isObject()){
      ui->treeWidget->clear();
      //Tree Format: <user>  ->  <SSL ID's>  ->  <optional info (variable/value)>
      QStringList users = args.toObject().keys();
      for(int u=0; u<users.length(); u++){
	if( !args.toObject().value(users[u]).isObject()){ continue; }
	QJsonObject UO = args.toObject().value(users[u]).toObject(); //user object
	QTreeWidgetItem *uItem = new QTreeWidgetItem(QStringList() << users[u]);
	ui->treeWidget->addTopLevelItem(uItem);
	QStringList ids = UO.keys();
	for(int i=0; i<ids.length(); i++){
	  QString info;
	  if(UO.value(ids[i]).isObject()){
	    QJsonObject tmpO = UO.value(ids[i]).toObject();
	    QStringList vars = tmpO.keys();
	    for(int v=0; v<vars.length(); v++){ vars[i].append(" = "+tmpO.value(vars[v]).toString() ); }
	    info = vars.join("\n");
	  }else if(UO.value(ids[i]).isString()){
	    info = UO.value(ids[i]).toString();
	  }
	  //Now create the tree item
	  QTreeWidgetItem *tmp = new QTreeWidgetItem();
	    QString txt = QString(tr("SSL Certificate %1")).arg(uItem->childCount()+1);
	    if(info.contains("Nickname:")){ txt.append(" ("+info.section("Nickname:",1,1).section("\n",0,0).simplified()+")"); }
	    tmp->setText(0,txt);
	    tmp->setToolTip(0, info);
	    tmp->setWhatsThis(0, users[u]+"::::"+ids[i]);
	  uItem->addChild(tmp); //add to the parent 
	}//end id loop
      } //end user loop
    }
  }
}

void ssl_auth_page::on_treeWidget_currentItemChanged(QTreeWidgetItem *cur, QTreeWidgetItem*){
  if(cur==0 || cur->childCount()>0){
    ui->text_info->clear();
    ui->push_revoke->setEnabled(false);
  }else{
    ui->text_info->setText(cur->toolTip(0));
    ui->push_revoke->setEnabled(true);	  
  }
}

void ssl_auth_page::on_treeWidget_itemActivated(QTreeWidgetItem *cur, int){
  if(cur==0){ return; }
  if(cur->childCount()>0){
    if(cur->isExpanded()){ ui->treeWidget->collapseItem(cur); }
    else{ ui->treeWidget->expandItem(cur); }
  }
}

void ssl_auth_page::on_push_revoke_clicked(){
  QTreeWidgetItem *cur = ui->treeWidget->currentItem();
  if(cur==0 || cur->whatsThis(0).isEmpty() ){ return; }
  QString user = cur->whatsThis(0).section("::::",0,0);
  QString pub_key = cur->whatsThis(0).section("::::",1,-1);
  //Now request that this key be revoked
  QJsonObject obj;
    obj.insert("action","revoke_ssl_cert");
    obj.insert("user",user);
    obj.insert("pub_key",pub_key);
  communicate("client_ssl_page_revoke","rpc","settings",obj);
}
