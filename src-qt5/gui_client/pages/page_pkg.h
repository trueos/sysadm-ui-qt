//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_PKG_PAGE_H
#define _PCBSD_SYSADM_CLIENT_PKG_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"
#include "page_pkg-BrowserItem.h"

//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class pkg_page_ui; //this is the name of the main widget/object in the QtDesigner form
};

class pkg_page : public PageWidget{
	Q_OBJECT
public:
	pkg_page(QWidget *parent, sysadm_client *core);
	~pkg_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "page_pkg"; } //ID is used to identify which type of page this is
	
private:
	Ui::pkg_page_ui *ui;

	//Internal flags
	bool local_showall, local_advmode, local_hasupdates, local_autocleanmode; //Local tab options
	QMenu *local_viewM, *repo_catM, *repo_catSM, *repo_backM;
	QNetworkAccessManager *NMAN;
	QList<QUrl> imagepending;
	QHash<QUrl, QImage> imagecache; // URL/image
	//Internal lists of origins being handled
	QStringList origin_installed, origin_pending;

	//Core requests
	void send_list_repos();
	void send_list_cats(QString repo);
	void send_local_update();
	void send_local_audit();
	void send_local_check_upgrade();
	void send_repo_app_info(QString origin, QString repo);

	//Parsing Core Replies
	void update_local_list(QJsonObject obj);
	void update_local_audit(QJsonObject obj);
	void update_pending_process(QJsonObject obj);
	void update_repo_app_info(QJsonObject obj);
	void update_repo_app_lists(QScrollArea *scroll, QJsonObject obj); //can use this for both search/browse returns

	//Bytes to human-readable conversion
	QString BtoHR(double bytes);
	//JsonArray to StringList conversion
	QStringList ArrayToStringList(QJsonArray array);
	//Status icon setting routine
	void updateStatusIcon( QTreeWidgetItem *it );
	//Status icon list update
	bool updateStatusList(QStringList *list, QString stat, bool enabled); //returns: changed (true/false);
	//Load an image from a URL
	void LoadImageFromURL(QLabel *widget, QString url);
	void LoadImageFromURL(QTreeWidgetItem *it,QString url);
	//ScreenShot Loading
	void showScreenshot(int num);
	//Browser Item Update
	void updateBrowserItem(BrowserItem *it, QJsonObject data);
	
	//User-interface items (functions defined in page_pkg-extras.cpp)
	// - Stuff for creating a home page
	void GenerateHomePage(QStringList cats, QString repo);
	QWidget* CreateBannerItem(QStringList images, QStringList actions);
	QWidget* CreateButtonItem(QString image, QString text, QString action);
	QWidget* CreateGroup(QString text, QList<QWidget*> items, bool horizontal = true);
	// - Other random stuff
	QStringList catsToText(QStringList cats); //output: <translated name>::::<cat> (pre-sorted by translated names)
	void GenerateCategoryMenu(QMenu *menu, QStringList cats);
	
private slots:
	void ParseReply(QString, QString, QString, QJsonValue);
	void ParseEvent(sysadm_client::EVENT_TYPE, QJsonValue);

	//GUI Updates
	// - local tab
	void update_local_buttons();
	void update_local_pkg_check(bool checked);
	void update_local_viewall(bool checked);
	void update_local_viewadv(bool checked);
	void update_local_viewclean(bool checked);
	void goto_browser_from_local(QTreeWidgetItem *it);
	// - repo tab
	void browser_goto_pkg(QString origin, QString repo = "");
	void browser_goto_cat(QAction *act = 0);
	void update_repo_changed();
	void icon_available(QNetworkReply*);
	void browser_last_ss();
	void browser_next_ss();
	void browser_prev_ss();
	void browser_first_ss();
	void browser_filter_search_cat(QAction *act);
	void browser_go_back(QAction *act = 0);
	void browser_update_history();
        void browser_home_button_clicked(QString action);
	// - pending tab
	void pending_show_log(bool);
	void pending_selection_changed();


	//GUI -> Core Requests
	// - local tab
	void send_local_rmpkgs();
	void send_local_lockpkgs();
	void send_local_unlockpkgs();
	void send_local_upgradepkgs();
	void send_local_cleanpkgs();
	// - repo tab
	void send_start_search(QString search = "", QStringList exclude = QStringList()); //search term input (optional - will pull from current text in input box)
	void send_start_browse(QString cat);
	void send_repo_rmpkg(QString origin = "");
	void send_repo_installpkg(QString origin = "");
};

//Special QToolButton subclass for home page buttons
class HomeButton : public QToolButton{
	Q_OBJECT
signals: 
	void HomeAction(QString);
private slots:
	void buttonclicked(){
	  emit HomeAction(this->whatsThis());
	}
public:
	HomeButton(QWidget *parent, QString action) : QToolButton(parent){
	  this->setWhatsThis(action);
	  connect(this, SIGNAL(clicked()), this, SLOT(buttonclicked()) );
	}
	~HomeButton(){}
};

class HomeSlider : public HomeButton{
	Q_OBJECT
private:
	QTimer *timer;
	QStringList images, actions;
	int cimage;
	QPropertyAnimation *animfade;
	QPropertyAnimation *animshow;
private slots:
	void updateimage(){
	  animfade->setStartValue(this->size() - QSize(10,10));
	  animshow->setEndValue(animfade->startValue());
	  animfade->setEndValue(QSize(0,0));
	  animshow->setStartValue(animfade->endValue());
	  cimage++;
	  if(cimage>=images.length()){ cimage = 0; }
	  animfade->start();
	}
	void fadeFinished(){
	  this->setIcon(QIcon(images[cimage]));
	  if(actions.length() >cimage){ this->setWhatsThis(actions[cimage]); }
	  else{ this->setWhatsThis(""); }
	  animshow->start();
	}
	void showFinished(){
	  if(images.length()>1){ timer->start(); }
	}
public:
	HomeSlider(QWidget *parent, QStringList imgList, QStringList acts, int secs = 10) : HomeButton(parent,""){
	  //this->setScaledContents(true);
	  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	  timer = new QTimer(this);
	    timer->setSingleShot(true);
	    timer->setInterval(secs*1000);
	    connect(timer, SIGNAL(timeout()), this, SLOT(updateimage()) );
	  cimage = -1;
	  images = imgList;
	  actions = acts;
	  animfade = new QPropertyAnimation(this, "iconSize");
	    animfade->setDuration(500); //1/2 second
	    connect(animfade, SIGNAL(finished()), this, SLOT(fadeFinished()) );
	  animshow = new QPropertyAnimation(this, "iconSize");
	    animshow->setDuration(500); //1/2 second
	    connect(animshow, SIGNAL(finished()), this, SLOT(showFinished()) );
	  QTimer::singleShot(100, this, SLOT(updateimage())); //start it up
	}
	~HomeSlider(){}

};
#endif
