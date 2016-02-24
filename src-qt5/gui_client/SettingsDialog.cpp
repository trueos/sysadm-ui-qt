//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog() : QMainWindow(), ui(new Ui::SettingsDialog){
  ui->setupUi(this);
  loadCurrentSettings();
  connect(ui->push_finished, SIGNAL(clicked()), this, SLOT(close()) );
}

SettingsDialog::~SettingsDialog(){
	
}


void SettingsDialog::InitSettings(){ //used on app startup *only*
  //Style
  QString style = settings->value("preferences/style","").toString();
  qDebug() << "Initial Style:" << style;
  if(!style.isEmpty()){
    style = SettingsDialog::readfile(":/styles/"+style+".qss");
  }
  static_cast<QApplication*>(QApplication::instance())->setStyleSheet(style);
  
  
}

// === PRIVATE ===
void SettingsDialog::loadCurrentSettings(){
  ui->combo_styles->clear();
  ui->combo_styles->addItem(tr("None"),"");
  QDir rsc(":/styles");
  QStringList styles = rsc.entryList(QStringList() << "*.qss", QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
  bool found = false;
  for(int i=0; i<styles.length(); i++){
    ui->combo_styles->addItem( styles[i].section(".qss",0,0), rsc.absoluteFilePath(styles[i]) );
  }
  int cur = ui->combo_styles->findText(settings->value("preferences/style","").toString() );
  if(cur<0){ cur = 0; }
  ui->combo_styles->setCurrentIndex(cur);
}

QString SettingsDialog::readfile(QString path){
  QFile file(path);
  QString out;
  if(file.open(QIODevice::ReadOnly) ){
    QTextStream in(&file);
    out = in.readAll();
    file.close();
  }
  return out;
}

// === PRIVATE SLOTS ===
void SettingsDialog::on_combo_styles_activated(int index){
  QString style = ui->combo_styles->itemData(index).toString();
  settings->setValue("preferences/style",style.section("/",-1).section(".qss",0,0));
  qDebug() << "Changed Style:" << settings->value("preferences/style").toString();
  if(!style.isEmpty()){
    style = SettingsDialog::readfile(style);
  }
  static_cast<QApplication*>(QApplication::instance())->setStyleSheet(style);
}