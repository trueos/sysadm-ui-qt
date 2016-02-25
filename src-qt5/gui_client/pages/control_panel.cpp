//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "control_panel.h"
#include "getPage.h"

#define REQ_ID "cp_client_auto_query_id"


control_panel::control_panel(QWidget *parent, sysadm_client *core) : PageWidget(parent, core){
  tree = new QTreeWidget(this);
    //Now configure the widget so it functions properly
    tree->setAnimated(true); //smoothly expand/hide sub-items
    tree->setRootIsDecorated(false); // Don't show the dropdown arrows
    tree->setHeaderHidden(true); // Don't show the column header (only 1 column)
    tree->setExpandsOnDoubleClick(false);
    tree->setItemsExpandable(true);
    tree->setAllColumnsShowFocus(false);
    tree->setMouseTracking(true); //make sure mouse-hover highlights items
    tree->setDragDropMode(QTreeView::NoDragDrop);
    tree->setDragEnabled(false);
    tree->setFrameShape(QFrame::NoFrame);
    int icosize = 2.3*tree->fontMetrics().height();
    tree->setIconSize(QSize(icosize,icosize));
  connect(tree, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(ItemClicked(QTreeWidgetItem*, int)) );
  connect(tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(ItemClicked(QTreeWidgetItem*, int)) );
  
  //Now add the tree widget to the page
  this->setLayout(new QVBoxLayout(this));
  this->layout()->setContentsMargins(0,0,0,0);
  this->layout()->addWidget(tree);
}

control_panel::~control_panel(){
  
}

void control_panel::setupCore(){
  connect(CORE, SIGNAL(newReply(QString, QString, QString, QJsonValue)), this, SLOT(parseReply(QString, QString, QString, QJsonValue)) );
}
	
void control_panel::startPage(){
  pages = KnownPages();
  CORE->communicate(REQ_ID, "rpc", "query", QJsonValue("simple-query"));
  emit ChangePageTitle( tr("Control Panel") );
}

// === PRIVATE ===
void control_panel::setupPageButton(QString id, QTreeWidgetItem *item){
  PAGEINFO info;
  for(int i=0; i<pages.length(); i++){
    if(pages[i].id==id){ info = pages[i]; break;}
  }
  if(info.id.isEmpty()){ return; }
  //*** Setup an icon/text for this page ***
  item->setText(0, info.name);
  item->setIcon(0, QIcon(info.icon));
  item->setToolTip(0, info.comment);
}

void control_panel::setupCategoryButton(QString cat, QTreeWidgetItem *item){
  QFont tmp = item->font(0);
    tmp.setWeight(QFont::Bold);
  item->setFont(0,tmp);
  if(cat=="appmgmt"){ 
    item->setText(0, QObject::tr("Application Management")); 
    item->setIcon(0, QIcon(":/icons/black/case.svg")); 
    item->setStatusTip(0, QObject::tr("App Management Status") );
    item->setToolTip(0, item->statusTip(0));
  }else if(cat=="sysmgmt"){
    item->setText(0, QObject::tr("System Management")); 
    item->setIcon(0, QIcon(":/icons/black/computer.svg")); 
  }else if(cat=="connect"){
    item->setText(0, QObject::tr("Connection")); 
    item->setIcon(0, QIcon(":/icons/black/globe.svg")); 
  }else if(cat=="servermgmt"){
    item->setText(0, QObject::tr("SysAdm Server Settings")); 
    item->setIcon(0, QIcon(":/icons/black/preferences2.svg")); 	
  }else{ //utils
    item->setText(0, QObject::tr("Utilities")); 
    item->setIcon(0, QIcon(":/icons/black/grid.svg")); 
  }
}

// === PRIVATE SLOTS ===
void control_panel::ItemClicked(QTreeWidgetItem *item, int col){
  if(item->childCount()>0){
    //This is a category - expand/contract it
    if(item->isExpanded()){
      tree->collapseItem(item);
    }else{
      for(int i=0; i<tree->topLevelItemCount(); i++){
        if(tree->topLevelItem(i)!=item && tree->topLevelItem(i)->isExpanded()){
	  tree->collapseItem(tree->topLevelItem(i));
	  QTime time; time.start();
	  while(time.elapsed() < 400){
	    QApplication::processEvents();
	  }
	}
      }
      tree->expandItem(item);
    }
  }else if(!item->whatsThis(col).isEmpty()){
    //This is an actual page/app - go ahead and launch it
    emit ChangePage(item->whatsThis(col));
  }else{
    //Should never happen - output some debug info
    qDebug() << "Empty item clicked:" << col << item->text(0);
  }
}

void control_panel::parseReply(QString id, QString namesp, QString name, QJsonValue args){
  //qDebug() << "CP Reply:" << id << namesp << name << args;
  if(id!=REQ_ID || name=="error" || namesp=="error"){ return; }
  //Got the reply to our request
  tree->clear();
  //qDebug() << " - args:" << args;
  if(args.isObject()){
    //qDebug() <<" - object";
    QJsonObject obj = args.toObject();
    QStringList ids = obj.keys();
    QStringList kpages;
    for(int i=0; i<pages.length(); i++){
      kpages << pages[i].category+"::::"+pages[i].name+"::::"+pages[i].id+"::::"+pages[i].req_systems.join("::");
    }
    kpages.sort(); //sort them by category/name
      //qDebug() << "ids:" << ids << "pages:" << pages;
      QTreeWidgetItem *ccat = 0;
      for(int i=0; i<kpages.length(); i++){
        QString cat = kpages[i].section("::::",0,0);
        QString id = kpages[i].section("::::",2,2);
	QStringList req = kpages[i].section("::::",3,3).split("::");
	bool ok = true;
	for(int j=0; j<req.length(); j++){
	 ok = ok && ids.contains(req[j]);
	}
        if(ok){
	  //Found an existing key - go ahead and create the item for it
	  if(ccat==0 || ccat->whatsThis(0)!=cat){
	    //Need to create the category first
	    ccat = new QTreeWidgetItem();
	      ccat->setWhatsThis(0,cat);
	      setupCategoryButton(cat, ccat);
	    tree->addTopLevelItem(ccat);
	  }
	  //Now create the child item for this page
	  QTreeWidgetItem *it = new QTreeWidgetItem();
	    it->setWhatsThis(0,id);
	    setupPageButton(id, it);
	  ccat->addChild(it);
        } //end key check
      }
  } //end of object check
}