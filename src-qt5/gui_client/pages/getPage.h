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


//Simplification function for creating a PAGEINFO structure
static PAGEINFO PageInfo(QString ID, QString i_name, QString i_title, QString i_icon, QString i_comment, QString i_cat, QStringList i_sys){
  PAGEINFO page;
  page.id = ID; page.name = i_name; page.title = i_title; page.icon = i_icon;
  page.comment = i_comment; page.category = i_cat; page.req_systems = i_sys;
  return page;
}

//List all the known pages
// **** Add new page entries here ****
static QList<PAGEINFO> KnownPages(){
  // Valid Groups: ["appmgmt", "sysmgmt", "connect", "utils"]
  QList<PAGEINFO> list;
  //Reminder: <ID>, <name>, <title>, <icon>, <comment>, <category>, <server subsytem list>
  list << PageInfo("page_beadm", QObject::tr("Boot Environment Manager"), QObject::tr("Boot Environment Manager"),":/icons/black/disk.svg",QObject::tr("Manage operating system snapshots"),"sysmgmt",QStringList() << "sysadm/beadm");
  list << PageInfo("page_taskmanager", QObject::tr("Task Manager"), QObject::tr("Task Manager"), ":/icons/black/flag.svg", QObject::tr("Monitor system tasks"), "sysmgmt", QStringList() << "sysadm/systemmanager");
  return list;
}

//Add any sub-pages here
#include "control_panel.h"
#include "page_beadm.h"
#include "page_taskmanager.h"

static PageWidget* GetNewPage(QString id, QWidget *parent, sysadm_client *core){
  //Find the page that matches this "id"
  if(id=="page_beadm"){ return new beadm_page(parent, core); }
  else if(id=="page_taskmanager"){ return new taskmanager_page(parent, core); }
	  
  //Return the main control_panel page as the fallback/default
  return new control_panel(parent, core);
}

#endif
