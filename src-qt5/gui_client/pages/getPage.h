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
  list << PageInfo("page_ssl_auth", QObject::tr("Manage SSL Keys"), QObject::tr("SSL Key Manager"), ":/icons/black/lock.svg",QObject::tr("List and Revoke SSL key registrations"), "servermgmt", QStringList() << "rpc/settings");
  list << PageInfo("page_about", QObject::tr("About TrueOS"), QObject::tr("About TrueOS"), ":/icons/black/magnifyingglass.svg",QObject::tr("More information on TrueOS"), "utils", QStringList() << "sysadm/about");
  list << PageInfo("page_lp", QObject::tr("Life Preserver"), QObject::tr("Life Preserver"), ":/icons/custom/lifepreserver.png",QObject::tr("Manage Local and Remote Backups"), "utils", QStringList() << "sysadm/lifepreserver" << "sysadm/zfs");
  list << PageInfo("page_system", QObject::tr("System Manager"), QObject::tr("System Manager"), ":/icons/black/boxfilled.svg",QObject::tr("Information About the System"), "utils", QStringList() << "sysadm/system");
  list << PageInfo("page_updates", QObject::tr("Update Manager"), QObject::tr("Update Manager"), ":/icons/black/sync.svg",QObject::tr("Perform Updates on the System"), "appmgmt", QStringList() << "sysadm/update");
  list << PageInfo("page_pkg", QObject::tr("AppCafe"), QObject::tr("AppCafe"), ":/icons/custom/appcafe.png",QObject::tr("Manage Applications/Packages"), "appmgmt", QStringList() << "sysadm/pkg");
  list << PageInfo("page_users", QObject::tr("User Manager"), QObject::tr("User Manager"), ":/icons/black/user.svg",QObject::tr("Manage Users/Groups"), "sysmgmt", QStringList() << "sysadm/users");
  list << PageInfo("page_services", QObject::tr("Service Manager"), QObject::tr("Service Manager"), ":/icons/black/pressure-reading.svg",QObject::tr("Manage Services"), "sysmgmt", QStringList() << "sysadm/services");
  list << PageInfo("page_firewall", QObject::tr("Firewall Manager"), QObject::tr("Firewall Manager"), ":/icons/black/burn.svg",QObject::tr("Manage Firewall"), "sysmgmt", QStringList() << "sysadm/firewall");
  list << PageInfo("page_moused", QObject::tr("Mouse Settings"), QObject::tr("Mouse Settings"), ":/icons/black/mouse.svg",QObject::tr("Manage pointer device configuration"), "sysmgmt", QStringList() << "sysadm/moused");
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
#include "page_users.h"
#include "page_services.h"
#include "page_firewall.h"
#include "page_moused.h"

static PageWidget* GetNewPage(QString id, QWidget *parent, sysadm_client *core){
  //Find the page that matches this "id"
  PageWidget *page = 0;
  if(id=="page_beadm"){ page =  new beadm_page(parent, core); }
  else if(id=="page_taskmanager"){ page = new taskmanager_page(parent, core); }
  else if(id=="page_iohyve"){ page =  new iohyve_page(parent, core); }
  else if(id=="page_ssl_auth"){ page = new ssl_auth_page(parent, core); }
  else if(id=="page_about"){ page =new about_page(parent, core); }
  else if(id=="page_lp"){ page = new lp_page(parent, core); }
  else if(id=="page_system"){ page = new system_page(parent, core); }
  else if(id=="page_updates"){ page = new updates_page(parent, core); }
  else if(id=="page_pkg"){ page = new pkg_page(parent, core); }
  else if(id=="page_users"){ page = new users_page(parent, core); }
  else if(id=="page_services"){ page = new services_page(parent, core); }
  else if(id=="page_firewall"){ page = new firewall_page(parent, core); }
  else if(id=="page_moused"){ page = new moused_page(parent, core); }
  //Return the main control_panel page as the fallback/default
  if(page==0){
    page = new control_panel(parent, core);
  }else{
    page->setWhatsThis(id); //tag it with the ID
  }
  return page;
}

#endif
