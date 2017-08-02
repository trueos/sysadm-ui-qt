//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_devices.h"
#include "ui_page_devices.h" //auto-generated from the .ui file

devices_page::devices_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::devices_ui){
  ui->setupUi(this);
  connect(ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()) );
  connect(ui->treeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(itemCollapsed(QTreeWidgetItem*)) );
  int isize = 2*ui->treeWidget->fontMetrics().height();
  ui->treeWidget->setIconSize( QSize(isize,isize) );
  //ui->treeWidget->setItemSpacing(isize/2);
}

devices_page::~devices_page(){

}

//Initialize the CORE <-->Page connections
void devices_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void devices_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Device Manager") );
  //Now run any CORE communications
  /*
  QJsonObject obj;
    obj.insert("sampleVariable","sampleValue");
  communicate("someID", "rpc", "query",obj);
  */
  send_get_devices();
}

void devices_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(id!="_sysadm_client_page_device_info"){ return; } //Not for here
  bool error = (namesp=="error" || name=="error");
  //qDebug() << "Got Reply:" << error << args.toObject().value("deviceinfo").toObject();

  //Define the static lists of class->icon maps
  static QStringList knownClasses;
  static QStringList knownSubclasses;
  static QStringList classIcons;
  static QStringList subclassIcons;
  //NOTE: Make sure the associated lists are the same length!!!
  if(knownClasses.isEmpty()){ knownClasses << "simple comms" << "serial bus" << "network" << "multimedia" << "mass storage" << "display" << "bridge"; }
  if(classIcons.isEmpty()){ classIcons << "speech-bubble" << "mark" << "globe" << "music" << "disk" << "photo" << "ticket"; }
  if(knownSubclasses.isEmpty()){ knownSubclasses << "USB" << "SMBus" << "ethernet" << "HDA" << "SATA" << "VGA" << "wifi"; }
  if(subclassIcons.isEmpty()){ subclassIcons << "inbox-download" << "mark2" << "arrow-left-right" << "volume-up" << "disk" << "computer" << "rss"; }

  ui->treeWidget->clear();
  ui->plainTextEdit->setVisible(false); //nothing selected initially
  if(error){ return; }
  //Parse the info list and add it to the tree widget
  QStringList ids = args.toObject().value("deviceinfo").toObject().keys();
  for(int i=0; i<ids.length(); i++){
    QJsonObject tmp = args.toObject().value("deviceinfo").toObject().value(ids[i]).toObject();
    QStringList values = tmp.keys();
      values.removeAll("class"); values.removeAll("subclass"); //specially-handled
    //Find the "class" item (if it exists)
    QTreeWidgetItem *classI = 0;
    for(int c=0; c<ui->treeWidget->topLevelItemCount() && classI==0; c++){
      if(ui->treeWidget->topLevelItem(c)->whatsThis(0) == tmp.value("class").toString()){ classI = ui->treeWidget->topLevelItem(c); }
    }
    // - None found - make a new one
    if(classI == 0){
      classI = new QTreeWidgetItem();
      classI->setText(0, tmp.value("class").toString());
      classI->setWhatsThis(0, tmp.value("class").toString());
      //Set an icon based on known classes
      int index = knownClasses.indexOf(tmp.value("class").toString());
      if( index  >=0){ classI->setIcon(0, QIcon( ":/icons/black/"+classIcons[index]+".svg")); }
      ui->treeWidget->addTopLevelItem(classI);
      ui->treeWidget->sortItems(0, Qt::AscendingOrder);
    }
    //Find the "subclass" item (if it exists)
    QTreeWidgetItem *subclassI = 0;
    if(tmp.value("subclass").toString().isEmpty()){ subclassI = classI; } //no subclass - only go one level deep
    for(int c=0; c<classI->childCount() && subclassI==0; c++){
      if(classI->child(c)->whatsThis(0) == tmp.value("subclass").toString()){ subclassI = classI->child(c); }
    }
    // - None found - make a new one
   if(subclassI == 0){
      subclassI = new QTreeWidgetItem();
      subclassI->setText(0, tmp.value("subclass").toString());
      subclassI->setWhatsThis(0, tmp.value("subclass").toString());
      //Set an icon based on known classes
      int index = knownSubclasses.indexOf(tmp.value("subclass").toString());
      if( index  >=0){ subclassI->setIcon(0, QIcon( ":/icons/black/"+subclassIcons[index]+".svg")); }
      classI->addChild(subclassI);
      classI->sortChildren(0, Qt::AscendingOrder);
    }
    //Now create the new item for this device
    QTreeWidgetItem *tmpI = new QTreeWidgetItem(subclassI, QStringList() << ids[i]);
    QStringList txt;
    for(int v=0; v<values.length(); v++){ txt << QString("%1  :  %2").arg(values[v], tmp.value(values[v]).toString()); }
    tmpI->setWhatsThis(0, txt.join("\n"));
    tmpI->setToolTip(0, txt.join("\n"));
    //Now ensure that the parent item sorts alphabetically
    subclassI->sortChildren(0, Qt::AscendingOrder);
  }
  //Now update the visibility of the items
  ui->label->setVisible(false);
  ui->treeWidget->setEnabled(true);
}

void devices_page::send_get_devices(){
  ui->label->setVisible(true);
  ui->treeWidget->setEnabled(false);
  QJsonObject obj;
    obj.insert("action","deviceinfo");
  communicate("_sysadm_client_page_device_info", "sysadm", "systemmanager",obj);
}

void devices_page::itemSelectionChanged(){
  QList<QTreeWidgetItem*> sel = ui->treeWidget->selectedItems();
  if(sel.isEmpty() || sel.first()->childCount()>0){ ui->plainTextEdit->setVisible(false); }
  else{
    ui->plainTextEdit->setPlainText(sel.first()->whatsThis(0));
    ui->plainTextEdit->setVisible(true);
  }
}

void devices_page::itemCollapsed(QTreeWidgetItem *it){
  QList<QTreeWidgetItem*> sel = ui->treeWidget->selectedItems();
  if(sel.isEmpty()){ return; }
  if(sel.first()->parent()==0){ return; }
  if(sel.first()->parent()==it || sel.first()->parent()->parent()==it){
    ui->plainTextEdit->setVisible(false); //selection collapsed
    sel.first()->setSelected(false);
  }
}
