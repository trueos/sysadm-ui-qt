//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_pkg-BrowserItem.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDebug>

BrowserItem::BrowserItem(QWidget *parent, QString ID) : QFrame(parent){
  objID = ID;
  this->setToolTip(objID);
  this->setObjectName("BrowserItem");
  this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  this->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken);
  //Create the widgets
  icon = new QLabel(this);
    int sz = icon->fontMetrics().lineSpacing()*2;
    icon->setFixedSize( QSize(sz,sz) );
  version = new QLabel(this);
    QFont fnt = version->font();
	fnt.setItalic(true);
    version->setFont(fnt);
  name = new QLabel(this);
  comment = new QLabel(this);
  tool_install = new QToolButton(this);
    tool_install->setIcon( QIcon(":/icons/black/download.svg") );
    tool_install->setToolTip(tr("Install Package"));
  tool_remove = new QToolButton(this);
    tool_remove->setIcon( QIcon(":/icons/black/trash.svg") );
    tool_remove->setToolTip(tr("Uninstall Package"));
  //Now add them to the main widget
  QHBoxLayout *H1 = new QHBoxLayout(); //row 1
    H1->setContentsMargins(1,1,1,1);
  QHBoxLayout *H2 = new QHBoxLayout(); //row 2
    H2->setContentsMargins(1,1,1,1);
  QGridLayout *V1 = new QGridLayout(); //overall vertical layout
    V1->setContentsMargins(1,1,1,1);
  //H1->addWidget(icon);
  H1->addWidget(name);
  H1->addStretch();
  H1->addWidget(version);
  H1->addStretch();
  H1->addWidget(tool_install);
  H1->addWidget(tool_remove);
  H2->addWidget(comment);
  V1->addWidget(icon, 0,0,2,2);
  V1->addLayout(H1,0,2);
  V1->addLayout(H2,1,2);
  this->setLayout(V1);
  //this->setStyleSheet("BrowserItem{ border: 1px solid black; border-radius: 3px; background: darkgrey; }");
  //Connect any signals/slots
  connect(tool_install, SIGNAL(clicked()), this, SLOT(install_clicked()) );
  connect(tool_remove, SIGNAL(clicked()), this, SLOT(remove_clicked()) );
}

BrowserItem::~BrowserItem(){
	
}

// === PUBLIC ===
QString BrowserItem::ID(){
  return objID;
}

void BrowserItem::setText(QString obj, QString text){
  //obj: "name","version","comment"
  if(obj=="name"){ name->setText(text); }
  else if(obj=="version"){ version->setText(text); }
  else if(obj=="comment"){ comment->setText(text); }
}

QLabel* BrowserItem::iconLabel(){
  return icon;
}

void BrowserItem::setInteraction(int stat){
  //stat: 0-installed, 1-not_installed, 2-pending
  //qDebug() << " - set browser interaction" << objID << stat;
  tool_install->setVisible(stat==1);
  tool_remove->setVisible(stat==0);
}

// === PRIVATE SLOTS ===
void BrowserItem::install_clicked(){
  setInteraction(2); //pending
  emit InstallClicked(objID);
}

void BrowserItem::remove_clicked(){
  setInteraction(2); //pending
  emit RemoveClicked(objID);
}

