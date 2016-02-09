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
    tree->setAllColumnsShowFocus(true);
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
  CORE->communicate(REQ_ID, "rpc", "query", QJsonValue("simple-query"));
  emit ChangePageTitle( tr("Control Panel") );
}
	
// === PRIVATE SLOTS ===
void control_panel::ItemClicked(QTreeWidgetItem *item, int col){
  if(item->childCount()>0){
    //This is a category - expand/contract it
    if(item->isExpanded()){
      tree->collapseItem(item);
    }else{
      tree->setAnimated(false);
      for(int i=0; i<tree->topLevelItemCount(); i++){
        if(tree->topLevelItem(i)!=item && tree->topLevelItem(i)->isExpanded()){
	  tree->collapseItem(tree->topLevelItem(i));
	}
      }
      tree->setAnimated(true);
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
    QStringList pages = ValidPages();
      pages.sort(); //sort them by category
      //qDebug() << "ids:" << ids << "pages:" << pages;
      QTreeWidgetItem *ccat = 0;
      for(int i=0; i<pages.length(); i++){
        QString cat = pages[i].section("::",0,0);
        QString id = pages[i].section("::",1,1);
        if(ids.contains(id)){
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