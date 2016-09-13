//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_firewall.h"
#include "ui_page_firewall.h" //auto-generated from the .ui file

#define FWTAG QString("sysadm_firewall_")

firewall_page::firewall_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::firewall_ui){
  ui->setupUi(this);
  //Setup connections
  connect(ui->tool_disable, SIGNAL(clicked()), this, SLOT(send_disable()) );
  connect(ui->tool_enable, SIGNAL(clicked()), this, SLOT(send_enable()) );
  connect(ui->tool_start, SIGNAL(clicked()), this, SLOT(send_start()) );
  connect(ui->tool_stop, SIGNAL(clicked()), this, SLOT(send_stop()) );
  connect(ui->tool_restart, SIGNAL(clicked()), this, SLOT(send_restart()) );
  connect(ui->tool_closeports, SIGNAL(clicked()), this, SLOT(closePortClicked()) );
  connect(ui->tool_open_port, SIGNAL(clicked()), this, SLOT(openPortClicked()) );

  connect(ui->tree_openports, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()) );
  connect(ui->combo_known_ports, SIGNAL(currentIndexChanged(int)), this, SLOT(serviceSelected(int)) );
  connect(ui->combo_type, SIGNAL(currentIndexChanged(int)), this, SLOT(newPortChanged()) );
  connect(ui->spin_portnum, SIGNAL(valueChanged(int)), this, SLOT(newPortChanged()) );

}

firewall_page::~firewall_page(){
  
}

//Initialize the CORE <-->Page connections
void firewall_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void firewall_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Firewall Manager") );
  //Now run any CORE communications
  send_get_knownports();
  send_get_status();
}

void firewall_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(FWTAG)){ return; } //not something this page needs
  bool haserror = (namesp=="error") || (name=="error");
  if(id==FWTAG+"status"){
    bool running = args.toObject().value("is_running").toString()=="true";
    bool enabled = args.toObject().value("is_enabled").toString()=="true";
    ui->tool_start->setEnabled(!running);
    ui->tool_stop->setEnabled(running);
    ui->tool_restart->setEnabled(running);
    ui->tool_enable->setEnabled(!enabled);
    ui->tool_disable->setEnabled(enabled);
    this->setEnabled(true);
  }else if(id==FWTAG+"known_ports"){
    //Save the info for later
    knownPorts = args.toObject();
    send_get_openports(); //now that this list is available - get the currently open ports
    //Now update the combobox with this info
    ui->combo_known_ports->clear();
    ui->combo_known_ports->addItem(tr("Select a Service...")); //initial top-level option (do nothing)
    QStringList ports = knownPorts.keys();
    QHash<QString, QStringList> services;
    for(int i=0; i<ports.length(); i++){
      //put the info into a sortable format
      QJsonObject info = knownPorts.value(ports[i]).toObject();
      QString nameinfo = info.value("name").toString();
      if(!info.value("description").toString().isEmpty()){ nameinfo.append(" ("+info.value("description").toString()+")"); }
      if(services.contains(nameinfo) ){
        QStringList old = services[nameinfo];
          old.append(info.value("port").toString());
        services.insert(nameinfo,  old); //add this port to the list
      }else{
        services.insert(nameinfo, QStringList() << info.value("port").toString()); // new entry for the list
      }
    }
    QStringList serv = services.keys();
    serv.sort(Qt::CaseInsensitive);
    for(int i=0; i<serv.length(); i++){
      ui->combo_known_ports->addItem(serv[i], services[serv[i]].join(",") ); //save the "port" as the user data for the item
    }

  }else if(id==FWTAG+"list_open"){
    //Read off the open ports
    QJsonArray oArr = args.toObject().value("openports").toArray();
    QStringList ports; 
    for(int i=0; i<oArr.count(); i++){ ports << oArr[i].toString(); }
    //Update the current items
    bool firstrun = ui->tree_openports->topLevelItemCount()<1;
    for(int i=0; i<ui->tree_openports->topLevelItemCount(); i++){
      if(ports.contains(ui->tree_openports->topLevelItem(i)->text(0)) ){
        ports.removeAll(ui->tree_openports->topLevelItem(i)->text(0) ); //already shown - remove from the main list
      }else{
        //This port is no longer open - remove it
        delete ui->tree_openports->takeTopLevelItem(i);
        i--;
      }
    }
    //Add in any new items
    for(int i=0; i<ports.length(); i++){
      QTreeWidgetItem *it = new QTreeWidgetItem(ui->tree_openports);
      it->setText(0, ports[i]);
      if(knownPorts.contains(ports[i])){
        it->setText(1, knownPorts.value(ports[i]).toObject().value("name").toString() );
        it->setText(2, knownPorts.value(ports[i]).toObject().value("description").toString() );
      }
      ui->tree_openports->addTopLevelItem(it);
    }    
    if(firstrun && ui->tree_openports->topLevelItemCount()>0){
      //Auto-adjust the column sizes 
      ui->tree_openports->resizeColumnToContents(0);
      ui->tree_openports->resizeColumnToContents(1);
      ui->tree_openports->resizeColumnToContents(2);
    }
    //Re-enable the UI
    ui->tool_closeports->setEnabled(true);
    ui->group_openport->setEnabled(true);
    ui->tree_openports->setEnabled(true);
  }else if(id==FWTAG+"ports_close" || id==FWTAG+"ports_open"){
    //open ports got modified - send out for current list
    send_get_openports();
  }else{
    //status change command returned - send out for the new status
    send_get_status();
  }
}

// ===================
//    PRIVATE SLOTS
// ===================
//Status update requests
void firewall_page::send_get_status(){
  this->setEnabled(false);
  QJsonObject obj;
    obj.insert("action","status");
  communicate(FWTAG+"status", "sysadm", "firewall",obj);
}

void firewall_page::send_get_knownports(){
  ui->tool_closeports->setEnabled(false);
  ui->group_openport->setEnabled(false);
  ui->tree_openports->setEnabled(false);
  QJsonObject obj;
    obj.insert("action","known_ports");
  communicate(FWTAG+"known_ports", "sysadm", "firewall",obj);
}

void firewall_page::send_get_openports(){
  QJsonObject obj;
    obj.insert("action","list_open");
  communicate(FWTAG+"list_open", "sysadm", "firewall",obj);
}


//Basic status changes
void firewall_page::send_start(){
  QJsonObject obj;
    obj.insert("action","start");
  communicate(FWTAG+"start", "sysadm", "firewall",obj);
}

void firewall_page::send_stop(){
  QJsonObject obj;
    obj.insert("action","stop");
  communicate(FWTAG+"stop", "sysadm", "firewall",obj);
}

void firewall_page::send_restart(){
  QJsonObject obj;
    obj.insert("action","restart");
  communicate(FWTAG+"restart", "sysadm", "firewall",obj);
}

void firewall_page::send_enable(){
  QJsonObject obj;
    obj.insert("action","enable");
  communicate(FWTAG+"enable", "sysadm", "firewall",obj);
}

void firewall_page::send_disable(){
  QJsonObject obj;
    obj.insert("action","disable");
  communicate(FWTAG+"disable", "sysadm", "firewall",obj);
}

//open/close ports
void firewall_page::send_open_ports(QStringList ports){
  ports.removeAll("");
  if(ports.isEmpty()){ return; }
  ui->tool_closeports->setEnabled(false);
  ui->group_openport->setEnabled(false);
  ui->tree_openports->setEnabled(false);
  QJsonObject obj;
    obj.insert("action","open");
    obj.insert("ports", QJsonArray::fromStringList(ports));
  communicate(FWTAG+"ports_open", "sysadm", "firewall",obj);
}

void firewall_page::send_close_ports(QStringList ports){
  ports.removeAll("");
  if(ports.isEmpty()){ return; }
  ui->tool_closeports->setEnabled(false);
  ui->group_openport->setEnabled(false);
  ui->tree_openports->setEnabled(false);
  QJsonObject obj;
    obj.insert("action","close");
    obj.insert("ports", QJsonArray::fromStringList(ports));
  communicate(FWTAG+"ports_close", "sysadm", "firewall",obj);
}


//UI interaction slots
void firewall_page::selectionChanged(){
  QList<QTreeWidgetItem*> sel = ui->tree_openports->selectedItems();
  ui->tool_closeports->setEnabled( !sel.isEmpty() );
}

void firewall_page::serviceSelected(int index){
  if(index<=0){ return; } //do nothing
  QStringList ports = ui->combo_known_ports->itemData(index).toString().split(",");
  send_open_ports(ports);
  ui->combo_known_ports->setCurrentIndex(0); //reset back to the initial item
}

void firewall_page::openPortClicked(){
  QString port = ui->spin_portnum->cleanText()+"/"+ui->combo_type->currentText();
  send_open_ports(QStringList() << port);
}

void firewall_page::closePortClicked(){
  QList<QTreeWidgetItem*> sel = ui->tree_openports->selectedItems();
  QStringList ports;
  for(int i=0; i<sel.length(); i++){ ports << sel[i]->text(0); }
  send_close_ports(ports);
}

void firewall_page::newPortChanged(){
  //see if the currently-selected port is already opened
  QString port = ui->spin_portnum->cleanText()+"/"+ui->combo_type->currentText();
  ui->tool_open_port->setEnabled( ui->tree_openports->findItems(port, Qt::MatchExactly, 0).isEmpty() );
}
