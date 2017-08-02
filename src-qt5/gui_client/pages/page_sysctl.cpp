//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_sysctl.h"
#include "ui_page_sysctl.h" //auto-generated from the .ui file

sysctl_page::sysctl_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::sysctl_ui){
  ui->setupUi(this);
  //connect(ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()) );
  //connect(ui->treeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(itemCollapsed(QTreeWidgetItem*)) );
  int isize = 1.5*ui->treeWidget->fontMetrics().height();
  ui->treeWidget->setIconSize( QSize(isize,isize) );
  //ui->treeWidget->setItemSpacing(isize/2);
  ui->plainTextEdit->setVisible(false); //nothing selected initially
}

sysctl_page::~sysctl_page(){

}

//Initialize the CORE <-->Page connections
void sysctl_page::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(ParseReply(QString, QString, QString, QJsonValue)) );
}

//Page embedded, go ahead and startup any core requests
void sysctl_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("System Control Manager") );
  //Now run any CORE communications
  /*
  QJsonObject obj;
    obj.insert("sampleVariable","sampleValue");
  communicate("someID", "rpc", "query",obj);
  */
  send_get_list();
}

void sysctl_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(id!="_sysadm_client_page_sysctl_info"){ return; } //Not for here
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
  QStringList ids = args.toObject().value("sysctllist").toObject().keys();
  for(int i=0; i<ids.length(); i++){
    QString value = args.toObject().value("sysctllist").toObject().value(ids[i]).toString();
    QTreeWidgetItem *it = 0;
    QStringList ident = ids[i].split(".");
    for(int i=0; i<ident.length(); i++){ it = findItem(it, ident[i]); }
    it->setText(0, ident[ident.length()-1] +"  =  "+value);
  }
  //Now update the visibility of the items
  ui->label->setVisible(false);
  ui->treeWidget->setEnabled(true);
}

void sysctl_page::send_get_list(){
  ui->label->setVisible(true);
  ui->treeWidget->setEnabled(false);
  QJsonObject obj;
    obj.insert("action","sysctllist");
  communicate("_sysadm_client_page_sysctl_info", "sysadm", "systemmanager",obj);
}

QTreeWidgetItem* sysctl_page::findItem(QTreeWidgetItem *parent, QString ident){
  QTreeWidgetItem *it = 0;
  if(parent==0){
    for(int c=0; c<ui->treeWidget->topLevelItemCount() && it==0; c++){
      if(ui->treeWidget->topLevelItem(c)->whatsThis(0) == ident){ it = ui->treeWidget->topLevelItem(c); }
    }
    // - None found - make a new one
    if(it == 0){
      it = new QTreeWidgetItem();
      it->setText(0, ident);
      it->setWhatsThis(0, ident);
      ui->treeWidget->addTopLevelItem(it);
      ui->treeWidget->sortItems(0, Qt::AscendingOrder);
    }
  }else{
    for(int c=0; c<parent->childCount() && it==0; c++){
      if(parent->child(c)->whatsThis(0) == ident){ it = parent->child(c); }
    }
    // - None found - make a new one
    if(it == 0){
      it = new QTreeWidgetItem();
      it->setText(0, ident);
      it->setWhatsThis(0, ident);
      parent->addChild(it);
      parent->sortChildren(0, Qt::AscendingOrder);
    }
  }
  return it;
}
/*void sysctl_page::itemSelectionChanged(){
  QList<QTreeWidgetItem*> sel = ui->treeWidget->selectedItems();
  if(sel.isEmpty() || sel.first()->childCount()>0){ ui->plainTextEdit->setVisible(false); }
  else{
    ui->plainTextEdit->setPlainText(sel.first()->whatsThis(0));
    ui->plainTextEdit->setVisible(true);
  }
}

void sysctl_page::itemCollapsed(QTreeWidgetItem *it){
  QList<QTreeWidgetItem*> sel = ui->treeWidget->selectedItems();
  if(sel.isEmpty()){ return; }
  if(sel.first()->parent()==0){ return; }
  if(sel.first()->parent()==it || sel.first()->parent()->parent()==it){
    ui->plainTextEdit->setVisible(false); //selection collapsed
    sel.first()->setSelected(false);
  }
}*/
