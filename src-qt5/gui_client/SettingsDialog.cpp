//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

QFont sys_font;

SettingsDialog::SettingsDialog() : QMainWindow(), ui(new Ui::SettingsDialog){
  ui->setupUi(this);
  loadCurrentSettings();
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
  qApp->setStyleSheet(style);
  //Fonts
  sys_font = QApplication::font(); //save this for later
  if(settings->value("preferences/useCustomFont",false).toBool()){
    QFont custom;
    custom.fromString(settings->value("preferences/CustomFont").toString());
    QApplication::setFont(custom);
  }
  
}

// === PRIVATE ===
void SettingsDialog::loadCurrentSettings(){
  // - styles
  ui->combo_styles->clear();
  ui->combo_styles->addItem(tr("None"),"");
  QDir rsc(":/styles");
  QStringList styles = rsc.entryList(QStringList() << "*.qss", QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
  for(int i=0; i<styles.length(); i++){
    ui->combo_styles->addItem( styles[i].section(".qss",0,0), rsc.absoluteFilePath(styles[i]) );
  }
  int cur = ui->combo_styles->findText(settings->value("preferences/style","").toString() );
  if(cur<0){ cur = 0; }
  ui->combo_styles->setCurrentIndex(cur);
  // - font
  QFont curF = QApplication::font();
  qDebug() << "Loaded current font:" << curF.toString();
  ui->spin_font_pt->setValue( curF.pointSize() );
  ui->combo_font->setCurrentFont(curF);
  ui->group_font->setChecked( settings->value("preferences/useCustomFont",false).toBool() );
  
  //Now setup the signals/slots
  connect(ui->push_finished, SIGNAL(clicked()), this, SLOT(close()) );
  connect(ui->combo_font, SIGNAL(currentFontChanged(const QFont&)), this, SLOT(fontchanged()) );
  connect(ui->spin_font_pt, SIGNAL(valueChanged(int)), this, SLOT(fontchanged()) );
  connect(ui->group_font, SIGNAL(toggled(bool)), this, SLOT(fontchanged()) );
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
  qApp->setStyleSheet(style);
}

void SettingsDialog::fontchanged(){
  if(ui->group_font->isChecked()){
    QFont sel = ui->combo_font->currentFont();
    sel.setPointSize(ui->spin_font_pt->value());
    qDebug() << "Setting Font:" << sel.toString();
    QApplication::setFont(sel);
    settings->setValue("preferences/useCustomFont", true);
    settings->setValue("preferences/CustomFont", sel.toString() );
  }else{
    settings->setValue("preferences/useCustomFont", false);
    qDebug() << "Setting Font (system):" << sys_font.toString();
    QApplication::setFont(sys_font);
  }
  ui->setupUi(this); //re-load the designer form
  loadCurrentSettings(); //re-load the contents of the form
  //Also update any other windows which are visible
  emit updateWindows();
}