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
  // Valid Groups: ["appmgmt", "sysmgmt", "connect", "utils", "servermgmt"]
  QList<PAGEINFO> list;
  //Reminder: <ID>, <name>, <title>, <icon>, <comment>, <category>, <server subsytem list>
  list << PageInfo("page_beadm", QObject::tr("Boot Environment Manager"), QObject::tr("Boot Environment Manager"),":/icons/black/disk.svg",QObject::tr("Manage operating system snapshots"),"sysmgmt",QStringList() << "sysadm/beadm");
  list << PageInfo("page_taskmanager", QObject::tr("Task Manager"), QObject::tr("Task Manager"), ":/icons/black/flag.svg", QObject::tr("Monitor system tasks"), "sysmgmt", QStringList() << "sysadm/systemmanager");
  list << PageInfo("page_iohyve",QObject::tr("iohyve"), QObject::tr("iohyve VM Manager"), ":/icons/black/desktop.svg", QObject::tr("Manage virtual OS instances"), "utils", QStringList() << "sysadm/iohyve" << "sysadm/network" << "sysadm/zfs");
  list << PageInfo("page_ssl_auth", QObject::tr("Manage SSL Keys"), QObject::tr("SSL Key Manager"), ":/icons/black/lock.svg",QObject::tr("List and Revoke SSL key registrations"), "servermgmt", QStringList() << "sysadm/settings");
  list << PageInfo("page_about", QObject::tr("About PC-BSD"), QObject::tr("About PC-BSD"), ":/icons/black/magnifyingglass.svg",QObject::tr("More information on PC-BSD"), "utils", QStringList() << "sysadm/about");
  list << PageInfo("page_lp", QObject::tr("Life Preserver"), QObject::tr("Life Preserver"), ":/icons/custom/lifepreserver.png",QObject::tr("Manage Local and Remote Backups"), "utils", QStringList() << "sysadm/lifepreserver" << "sysadm/zfs");
  list << PageInfo("page_system", QObject::tr("System Manager"), QObject::tr("System Manager"), ":/icons/black/boxfilled.svg",QObject::tr("Information About the System"), "utils", QStringList() << "sysadm/system");
  list << PageInfo("page_updates", QObject::tr("Update Manager"), QObject::tr("Update Manager"), ":/icons/black/sync.svg",QObject::tr("Perform Updates on the System"), "appmgmt", QStringList() << "sysadm/update");
  list << PageInfo("page_pkg", QObject::tr("AppCafe"), QObject::tr("AppCafe"), ":/icons/custom/appcafe.png",QObject::tr("Manage Applications/Packages"), "appmgmt", QStringList() << "sysadm/pkg");
	return list;
}

//Add any sub-pages here
#include "control_panel.h"
#include "page_beadm.h"
#include "page_taskmanager.h"
#include "page_iohyve.h"
#include "page_ssl_auth.h"
#include "page_about.h"
#include "page_lp.h"
#include "page_system.h"
#include "page_updates.h"
#include "page_pkg.h"

static PageWidget* GetNewPage(QString id, QWidget *parent, sysadm_client *core){
  //Find the page that matches this "id"
  if(id=="page_beadm"){ return new beadm_page(parent, core); }
  else if(id=="page_taskmanager"){ return new taskmanager_page(parent, core); }
  else if(id=="page_iohyve"){ return new iohyve_page(parent, core); }
  else if(id=="page_ssl_auth"){ return new ssl_auth_page(parent, core); }
  else if(id=="page_about"){ return new about_page(parent, core); } 
  else if(id=="page_lp"){ return new lp_page(parent, core); }
  else if(id=="page_system"){ return new system_page(parent, core); }
  else if(id=="page_updates"){ return new updates_page(parent, core); }
  else if(id=="page_pkg"){ return new pkg_page(parent, core); }
  //Return the main control_panel page as the fallback/default
  return new control_panel(parent, core);
}

#endif
