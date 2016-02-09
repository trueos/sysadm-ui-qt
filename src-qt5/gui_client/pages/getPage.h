//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_PAGE_WIDGET_FETCH_H
#define _PCBSD_SYSADM_CLIENT_PAGE_WIDGET_FETCH_H

#include "../globals.h"
#include "../PageWidget.h"


static QStringList ValidPages(){
    //*** Add a page ID for each type of subpage here ***
    // Format: "<group>::ID" where ID is "<namespace>/<name>" of the server subsystem
    // Groups: ["appmgmt", "sysmgmt", "connect", "utils"]
    QStringList known;
    //Add more pages here
    known << "appmgmt::rpc/dispatcher";
    known << "appmgmt::rpc/syscache";
    known << "utils::sysadm/iocage";
    known << "utils::sysadm/lifepreserver";
    known << "connect::sysadm/network";
    known << "sysmgmt::sysadm/systemmanager";
    known << "sysmgmt::sysadm/update";
    //return the known pages
    return known;	
}

static void setupPageButton(QString id, QTreeWidgetItem *item){
  //*** Setup an icon/text for this page ***
  /*if(id=="something"){
    item->setText( tr("Something") );
    item->setIcon(QIcon(":/icons/Black/
  }*/
  //just assign some random icon for the moment
  item->setText(0,id);
  item->setIcon(0,QIcon(":/icons/black/inboxes.svg"));
}

static void setupCategoryButton(QString cat, QTreeWidgetItem *item){
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
  }else{ //utils
    item->setText(0, QObject::tr("Utilities")); 
    item->setIcon(0, QIcon(":/icons/black/preferences.svg")); 
  }
}


//Add any sub-pages here
#include "control_panel.h"

static PageWidget* GetNewPage(QString id, QWidget *parent, sysadm_client *core){
  //Find the page that matches this "id"
  if(id=="sample"){} //do something
	  
  //Return the main control_panel page as the fallback/default
  return new control_panel(parent, core);
}

#endif
