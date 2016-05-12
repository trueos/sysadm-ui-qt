//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_pkg.h"
#include "ui_page_pkg.h"

#define LCATS (QStringList()<<"arabic"<<"chinese"<<"french"<<"german"<<"hebrew"<<"hungarian" \
	<<"japanese"<<"korean"<<"polish"<<"portuguese"<<"russian"<<"ukrainian"<<"vietnamese")
	
#define DCATS (QStringList()<<"devel"<<"lang"<<"cad"<<"ports-mgmt"<<"java"<<"converters"<<"databases")
#define NCATS (QStringList()<<"www"<<"irc"<<"ftp"<<"dns"<<"news"<<"mail")
#define UCATS (QStringList()<<"sysutils"<<"deskutils"<<"comms"<<"shells"<<"benchmarks")

void pkg_page::GenerateHomePage(QStringList cats, QString repo){
  //Quick Check to ensure that the page has a widget/layout
  if(ui->scroll_home->widget()==0){ ui->scroll_home->setWidget(new QWidget(this)); }
  if(ui->scroll_home->widget()->layout()==0){ 
    ui->scroll_home->widget()->setLayout( new QGridLayout() ); 
    ui->scroll_home->widget()->layout()->setContentsMargins(0,0,0,0);
    ui->scroll_home->widget()->layout()->setSpacing(2);
  }
  QGridLayout *layout = static_cast<QGridLayout*>(ui->scroll_home->widget()->layout());
  for(int i=0; i<layout->count(); i++){ delete layout->takeAt(i); }
  //POPULATE THE PAGE
  //FORMAT NOTE for actions:
  //  For search: 		"search::<category>::<search term>::<exclude terms>" (exclude terms are space-delimited)
  //  For category: 	"cat::<category>"
  //  For package:	"pkg::<origin>"
  
  //qDebug() << "Creating Home Page...";
  //Create a simple non-interactive widget
  layout->addWidget(CreateBannerItem(QStringList() << ":/icons/custom/pcbsd-banner.png"<<":/icons/custom/pcbsd-banner.png", \
		QStringList() << "search::::pcbsd" << "search::::pcbsd") \
		,0,0,1,2);
  //Create a group of items (horizontal);
  layout->addWidget( CreateGroup(tr("Popular Searches"), QList<QWidget*>() \
    << CreateButtonItem(":/icons/black/globe.svg", tr("Web Browsers"), "search::www::web browser") \
    << CreateButtonItem(":/icons/black/mail.svg", tr("Email Clients"), "search::mail::client ") \
    << CreateButtonItem(":/icons/black/paperclip.svg", tr("Office Suites"), "search::editors::office::-libre") \
    << CreateButtonItem(":/icons/black/desktop2.svg", tr("Desktops"), "search::::desktop meta port") \
    , true), 1, 0, 1, 2);
  //Create a group of items (vertical);
  layout->addWidget( CreateGroup(tr("Popular Categories"), QList<QWidget*>() \
    << CreateButtonItem(":/icons/black/keyboard.svg", tr("Games"), "cat::games") \
    << CreateButtonItem(":/icons/black/movie.svg", tr("Multimedia"), "cat::multimedia") \
    << CreateButtonItem(":/icons/black/music.svg", tr("Audio"), "cat::audio") \
    << CreateButtonItem(":/icons/black/computer.svg", tr("Desktop Utilities"), "cat::deskutils") \
    , true), 2, 0, 1, 2);
  //Now set one of the rows to expand more than the others
  //layout->setRowStretch(0,1);
}

QWidget* pkg_page::CreateBannerItem(QStringList images, QStringList actions){
  //This creates a non-interactive image item for the home page
  HomeSlider *tmp = new HomeSlider(ui->scroll_home->widget(), images, actions);
    connect(tmp, SIGNAL(HomeAction(QString)), this, SLOT(browser_home_button_clicked(QString)) );
  return tmp;
}

QWidget* pkg_page::CreateButtonItem(QString image, QString text, QString action){
  HomeButton *tmp = new HomeButton(ui->scroll_home->widget(), action);
	tmp->setText(text);
	tmp->setIcon( QIcon(image) );
	if(!text.isEmpty()){ tmp->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); }
	connect(tmp, SIGNAL(HomeAction(QString)), this, SLOT(browser_home_button_clicked(QString)) );
  return tmp;
}

QWidget* pkg_page::CreateGroup(QString text, QList<QWidget*> items, bool horizontal){
  QGroupBox * tmp = new QGroupBox(ui->scroll_home->widget());
    tmp->setTitle(text);
  if(horizontal){ tmp->setLayout( new QHBoxLayout() ); }
  else{ tmp->setLayout( new QVBoxLayout() ); }
  QBoxLayout *layout = static_cast<QBoxLayout*>( tmp->layout() );
  layout->setContentsMargins(2,2,2,2);
  for(int i=0; i<items.length(); i++){ 
    items[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->addWidget(items[i]); 
  }
  return tmp;
}

QStringList pkg_page::catsToText(QStringList cats){
  QStringList out;
  for(int i=0; i<cats.length(); i++){
    if(cats[i]=="accessibility"){ out << tr("Accessibility")+"::::"+cats[i]; }
    else if(cats[i]=="arabic"){ out << tr("Arabic")+"::::"+cats[i]; }
    else if(cats[i]=="archivers"){ out << tr("Archivers")+"::::"+cats[i]; }
    else if(cats[i]=="astro"){ out << tr("Astro")+"::::"+cats[i]; }
    else if(cats[i]=="audio"){ out << tr("Audio")+"::::"+cats[i]; }
    else if(cats[i]=="benchmarks"){ out << tr("Benchmarks")+"::::"+cats[i]; }
    else if(cats[i]=="biology"){ out << tr("Biology")+"::::"+cats[i]; }
    else if(cats[i]=="cad"){ out << tr("CAD")+"::::"+cats[i]; }
    else if(cats[i]=="chinese"){ out << tr("Chinese")+"::::"+cats[i]; }
    else if(cats[i]=="comms"){ out << tr("Comms")+"::::"+cats[i]; }
    else if(cats[i]=="converters"){ out << tr("Converters")+"::::"+cats[i]; }
    else if(cats[i]=="databases"){ out << tr("Databases")+"::::"+cats[i]; }
    else if(cats[i]=="deskutils"){ out << tr("Desktop Utilities")+"::::"+cats[i]; }
    else if(cats[i]=="devel"){ out << tr("Development")+"::::"+cats[i]; }
    else if(cats[i]=="dns"){ out << tr("DNS")+"::::"+cats[i]; }
    else if(cats[i]=="editors"){ out << tr("Editors")+"::::"+cats[i]; }
    else if(cats[i]=="emulators"){ out << tr("Emulators")+"::::"+cats[i]; }
    else if(cats[i]=="finance"){ out << tr("Finance")+"::::"+cats[i]; }
    else if(cats[i]=="french"){ out << tr("French")+"::::"+cats[i]; }
    else if(cats[i]=="ftp"){ out << tr("FTP")+"::::"+cats[i]; }
    else if(cats[i]=="games"){ out << tr("Games")+"::::"+cats[i]; }
    else if(cats[i]=="german"){ out << tr("German")+"::::"+cats[i]; }
    else if(cats[i]=="graphics"){ out << tr("Graphics")+"::::"+cats[i]; }
    else if(cats[i]=="hebrew"){ out << tr("Hebrew")+"::::"+cats[i]; }
    else if(cats[i]=="hungarian"){ out << tr("Hungarian")+"::::"+cats[i]; }
    else if(cats[i]=="irc"){ out << tr("IRC")+"::::"+cats[i]; }
    else if(cats[i]=="japanese"){ out << tr("Japanese")+"::::"+cats[i]; }
    else if(cats[i]=="java"){ out << tr("Java")+"::::"+cats[i]; }
    else if(cats[i]=="korean"){ out << tr("Korean")+"::::"+cats[i]; }
    else if(cats[i]=="lang"){ out << tr("Languages")+"::::"+cats[i]; }
    else if(cats[i]=="mail"){ out << tr("Mail")+"::::"+cats[i]; }
    else if(cats[i]=="math"){ out << tr("Math")+"::::"+cats[i]; }
    else if(cats[i]=="misc"){ out << tr("Miscellaneous")+"::::"+cats[i]; }
    else if(cats[i]=="multimedia"){ out << tr("Multimedia")+"::::"+cats[i]; }
    else if(cats[i]=="net"){ out << tr("Network")+"::::"+cats[i]; }
    else if(cats[i]=="net-im"){ out << tr("Network-IM")+"::::"+cats[i]; }
    else if(cats[i]=="net-mgmt"){ out << tr("Network-Management")+"::::"+cats[i]; }
    else if(cats[i]=="net-p2p"){ out << tr("Network-P2P")+"::::"+cats[i]; }
    else if(cats[i]=="news"){ out << tr("News")+"::::"+cats[i]; }
    else if(cats[i]=="palm"){ out << tr("Palm")+"::::"+cats[i]; }
    else if(cats[i]=="polish"){ out << tr("Polish")+"::::"+cats[i]; }
    else if(cats[i]=="ports-mgmt"){ out << tr("Ports Management")+"::::"+cats[i]; }
    else if(cats[i]=="portuguese"){ out << tr("Portuguese")+"::::"+cats[i]; }
    else if(cats[i]=="print"){ out << tr("Print")+"::::"+cats[i]; }
    else if(cats[i]=="russian"){ out << tr("Russian")+"::::"+cats[i]; }
    else if(cats[i]=="science"){ out << tr("Science")+"::::"+cats[i]; }
    else if(cats[i]=="security"){ out << tr("Security")+"::::"+cats[i]; }
    else if(cats[i]=="shells"){ out << tr("Shells")+"::::"+cats[i]; }
    else if(cats[i]=="sysutils"){ out << tr("System Utilities")+"::::"+cats[i]; }
    else if(cats[i]=="textproc"){ out << tr("Text Processing")+"::::"+cats[i]; }
    else if(cats[i]=="ukrainian"){ out << tr("Ukrainian")+"::::"+cats[i]; }
    else if(cats[i]=="vietnamese"){ out << tr("Vietnamese")+"::::"+cats[i]; }
    else if(cats[i]=="www"){ out << tr("WWW")+"::::"+cats[i]; }
    else if(cats[i]=="x11"){ out << tr("X11")+"::::"+cats[i]; }
    else if(cats[i]=="x11-clocks"){ out << tr("X11-Clocks")+"::::"+cats[i]; }
    else if(cats[i]=="x11-drivers"){ out << tr("X11-Drivers")+"::::"+cats[i]; }
    else if(cats[i]=="x11-fm"){ out << tr("X11-File Managers")+"::::"+cats[i]; }
    else if(cats[i]=="x11-fonts"){ out << tr("X11-Fonts")+"::::"+cats[i]; }
    else if(cats[i]=="x11-servers"){ out << tr("X11-Servers")+"::::"+cats[i]; }
    else if(cats[i]=="x11-themes"){ out << tr("X11-Themes")+"::::"+cats[i]; }
    else if(cats[i]=="x11-toolkits"){ out << tr("X11-Toolkits")+"::::"+cats[i]; }
    else if(cats[i]=="x11-wm"){ out << tr("X11-Window Managers")+"::::"+cats[i]; }
    else{ out << cats[i]+"::::"+cats[i]; } //no translation available
  }
  out.sort();
  return out;
}

void pkg_page::GenerateCategoryMenu(QMenu *menu, QStringList cats){
  //Note: the "cats" input must be the raw (non-translated) category list
  //qDebug() << "Cats:" << cats;
  menu->clear();
  if(menu == repo_catSM){ menu->addAction("No Category Filter"); menu->addSeparator(); }
  QStringList locale, net, x11, devel, utils, other;
  //Divide up the main list into sub-categories
  for(int i=0; i<cats.length(); i++){
    if(cats[i].startsWith("x11")){ x11 << cats[i]; }
    else if( cats[i].startsWith("net") || NCATS.contains(cats[i]) ){ net << cats[i]; }
    else if( LCATS.contains(cats[i]) ){ locale << cats[i]; }
    else if( DCATS.contains(cats[i]) ){ devel << cats[i]; }
    else if( UCATS.contains(cats[i]) ){ utils << cats[i]; }
    else{ other << cats[i]; }
  }
  //Now assemble the main Menu
  // - First do all the non-categorized items
  other = catsToText(other);
  for(int i=0; i<other.length(); i++){
    QAction *tmpA = menu->addAction(other[i].section("::::",0,0));
	tmpA->setWhatsThis(other[i].section("::::",1,1));
  }
  if(!x11.isEmpty() || !net.isEmpty() || !locale.isEmpty()){
    menu->addSection(tr("Categories"));
  }
  // - Devel specific categories
  if(!devel.isEmpty()){
    devel = catsToText(devel);
    QMenu *tmpM = menu->addMenu(QIcon(":/icons/black/preferences.svg"), tr("Development"));
    for(int i=0; i<devel.length(); i++){
      QAction *tmpA = tmpM->addAction(devel[i].section("::::",0,0));
	tmpA->setWhatsThis(devel[i].section("::::",1,1));
    }
  }
  // - Locale specific categories
  if(!locale.isEmpty()){
    locale = catsToText(locale);
    QMenu *tmpM = menu->addMenu(QIcon(":/icons/black/flag.svg"), tr("Locale Specific"));
    for(int i=0; i<locale.length(); i++){
      QAction *tmpA = tmpM->addAction(locale[i].section("::::",0,0));
	tmpA->setWhatsThis(locale[i].section("::::",1,1));
    }
  }
  // - Network specific categories
  if(!net.isEmpty()){
    net = catsToText(net);
    QMenu *tmpM = menu->addMenu(QIcon(":/icons/black/globe.svg"), tr("Networking"));
    for(int i=0; i<net.length(); i++){
      QAction *tmpA = tmpM->addAction(net[i].section("::::",0,0));
	tmpA->setWhatsThis(net[i].section("::::",1,1));
    }
  }
  // - Utilities
  if(!utils.isEmpty()){
    utils = catsToText(utils);
    QMenu *tmpM = menu->addMenu(QIcon(":/icons/black/bag.svg"), tr("Utilities"));
    for(int i=0; i<utils.length(); i++){
      QAction *tmpA = tmpM->addAction(utils[i].section("::::",0,0));
	tmpA->setWhatsThis(utils[i].section("::::",1,1));
    }
  }
  // - Now all X11 categories
  if(!x11.isEmpty()){
    x11 = catsToText(x11);
    QMenu *tmpM = menu->addMenu(QIcon(":/icons/black/desktop2.svg"), tr("X11"));
    for(int i=0; i<x11.length(); i++){
      QAction *tmpA = tmpM->addAction(x11[i].section("::::",0,0));
	tmpA->setWhatsThis(x11[i].section("::::",1,1));
    }
  }
  if(menu->isEmpty()){
    //Fallback in case no categories are found
    QAction *tmpA = menu->addAction(tr("All Packages"));
      tmpA->setWhatsThis("all");
  }
	
}
