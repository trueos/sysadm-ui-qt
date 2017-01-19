//===========================================
//  TrueOS source code
//  Copyright (c) 2017, TrueOS Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_moused.h"
#include "ui_page_moused.h" //auto-generated from the .ui file
#include <math.h>

#define MOUSED_TAG QString("sysadm_page_moused_")

moused_page::moused_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::page_moused){
  ui->setupUi(this);	
  //Setup all the combobox options and slider ranges
  ui->slider_acc_exp->setRange(0, 100);
  ui->slider_acc_linear->setRange(0,100);
  ui->slider_drift->setRange(0,1000);
  ui->combo_resolution->clear();
    ui->combo_resolution->addItem(tr("Low"), "low");
    ui->combo_resolution->addItem(tr("Medium-Low"), "medium-low");
    ui->combo_resolution->addItem(tr("Medium-High"), "medium-high");
    ui->combo_resolution->addItem(tr("High"), "high");
  ui->combo_hand->clear();
    ui->combo_hand->addItem(tr("Left Handed"), "left");
    ui->combo_hand->addItem(tr("Right Handed"), "right");

  //Make all the UI connections
  connect(ui->tool_refresh, SIGNAL(clicked()), this, SLOT(send_list_devices()) );
  connect(ui->combo_device, SIGNAL(currentIndexChanged(int)), this, SLOT(send_list_device_settings()) );
  connect(ui->check_device_active, SIGNAL(toggled(bool)), this, SLOT(send_toggle_device_active()) );
  connect(ui->slider_acc_exp, SIGNAL(valueChanged(int)), this, SLOT(slider_acc_exp_changed()) );
  connect(ui->slider_acc_linear, SIGNAL(valueChanged(int)), this, SLOT(slider_acc_linear_changed()) );
  connect(ui->slider_drift, SIGNAL(valueChanged(int)), this, SLOT(slider_drift_changed()) );
  connect(ui->push_apply, SIGNAL(clicked()), this, SLOT(send_save_device_settings()) );

}

moused_page::~moused_page(){
  
}

//Initialize the CORE <-->Page connections
void moused_page::setupCore(){

}

//Page embedded, go ahead and startup any core requests
void moused_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Mouse Settings") );
  send_list_devices();
}

// === PUBLIC SLOTS ===
void moused_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(MOUSED_TAG)){ return; } //not a message for this page
  bool iserror = (namesp.toLower()=="error" || name.toLower()=="error");
  if(iserror){ qDebug() << "API ERROR:" << id << namesp << name << args; }
  qDebug() << "Got Reply:" << id;
  if(id==MOUSED_TAG+"list_devices"){
    QString lastDevice = ui->combo_device->currentData().toString().section("::::",0,0);
    ui->combo_device->clear();
    if(args.toObject().contains("list_devices")){
      QStringList devs = args.toObject().value("list_devices").toObject().keys();
      for(int i=0; i<devs.length(); i++){
        QJsonObject info = args.toObject().value("list_devices").toObject().value(devs[i]).toObject();
        ui->combo_device->addItem(devs[i] +" ("+ info.value("description").toString()+")", devs[i] + "::::"+info.value("active").toString());
      }
    }
    if(!lastDevice.isEmpty()){
      int index = ui->combo_device->findData(lastDevice+"::::", Qt::MatchStartsWith);
      if(index>=0){ ui->combo_device->setCurrentIndex(index); }
    }
    QApplication::processEvents(); //throw away any signals sent by changing the device widget
    this->setEnabled(true);
    send_list_device_settings();

  }else if( id==MOUSED_TAG+"load_settings"){
    if(args.toObject().contains("read_device_options")){
      QJsonObject obj = args.toObject().value("read_device_options").toObject();
      if(obj.value("device").toString() == ui->combo_device->currentData().toString().section("::::",0,0) ){
        updateCurrentSettings(obj);
        QApplication::processEvents(); //throw away any signals send by changing the settings widgets
        ui->frame_settings->setEnabled(ui->check_device_active->isChecked());
      }
    }

  }else if(id == MOUSED_TAG+"toggle_active"){
    send_list_devices();

  }else if(id==MOUSED_TAG+"save_settings"){
    send_list_device_settings();
  }
  
}

// === PRIVATE SLOTS ===
void moused_page::send_list_devices(){
  this->setEnabled(false);
  //qDebug() << "Request Device List";
  QJsonObject obj;
    obj.insert("action","list_devices");
  communicate(MOUSED_TAG+"list_devices", "sysadm", "moused",obj);
}

void moused_page::send_toggle_device_active(){
  if(!this->isEnabled()){ return; } //programmatic change
  bool setactive = ui->check_device_active->isChecked();
  if( (setactive ? "true" : "false") == ui->combo_device->currentData().toString().section("::::",1,1) ){ return; } //nothing to change
  QString device = ui->combo_device->currentData().toString().section("::::",0,0);
  QJsonObject obj;
    obj.insert("action", QString("set_device_%1").arg(setactive ? "active" : "inactive") );
    obj.insert("device", device);
  communicate(MOUSED_TAG+"toggle_active", "sysadm", "moused",obj);
}

void moused_page::send_list_device_settings(){
  if(!this->isEnabled()){ return; } //in the middle of programmatic changes - don't perform any requests yet
  ui->frame_settings->setEnabled(false);
  ui->check_device_active->setChecked( ui->combo_device->currentData().toString().section("::::",1,1)=="true" );
  QString device = ui->combo_device->currentData().toString().section("::::",0,0);
  if(device.isEmpty()){ return; } //nothing specified yet
  //qDebug() << "Request Device Settings:" << device;
   QJsonObject obj;
    obj.insert("action","read_device_options");
    obj.insert("device",device);
  communicate(MOUSED_TAG+"load_settings", "sysadm", "moused",obj); 
}

void moused_page::send_save_device_settings(){
  qDebug() << "Save Settings";
  QString device = ui->combo_device->currentData().toString().section("::::",0,0);
  QJsonObject obj;
    obj.insert("action","set_device_options");
    obj.insert("device",device);
    obj.insert("accel_exponential",ui->label_acc_exp_val->text());
    obj.insert("accel_linear", ui->label_acc_linear_val->text());
    obj.insert("hand_mode", ui->combo_hand->currentData().toString());
    obj.insert("resolution", ui->combo_resolution->currentData().toString());
    obj.insert("terminate_drift_threshold_pixels",  QString::number(ui->slider_drift->value()) ); //no conversion needed here
    obj.insert("emulate_button_3", ui->check_emulate_button_3->isChecked() ? "true" : "false");
    obj.insert("virtual_scrolling", ui->check_virtual_scroll->isChecked() ? "true" : "false" );
  communicate(MOUSED_TAG+"save_settings", "sysadm", "moused",obj); 

}

//UI slots
void moused_page::settingChanged(){ 
  //update whether the apply button is available
  if(!ui->frame_settings->isEnabled()){ return; } //programmatic changes
  ui->push_apply->setEnabled(true); //make this dynamic later - check that one of the values is actually changed
}

void moused_page::slider_acc_exp_changed(){
  //Convert the slider integer into a double for the backend
  // EQUATION: Y = 0.01*X+1.0 ( 0 <= X <= 100)
  double num = 1.0 + ui->slider_acc_exp->value()*0.01;
  ui->label_acc_exp_val->setText( QString::number(num) );
  settingChanged();

}

void moused_page::slider_acc_linear_changed(){
  //Convert the slider integer into a double for the backend
  // EQUATION: Y = e^(0.0921034 * (X-50))   (0<=X<=100)
   double num = ::exp(0.0921034 * (ui->slider_acc_linear->value()-50) );
    //Round the number to 3 digits
    if(num>=100){ num = qRound(num); }
    else if(num>=10){ num = qRound(num*10.0)/10.0; }
    else if(num>=1){ num = qRound(num*100.0)/100.0; }
    else{ num = qRound(num*1000.0)/1000.0; }
  ui->label_acc_linear_val->setText( QString::number(num) );
  settingChanged(); 
}

void moused_page::slider_drift_changed(){
  if(ui->slider_drift->value()==0){
    ui->label_drift_val->setText( tr("Disabled") );
  }else{
   QString num = QString::number(ui->slider_drift->value());
    ui->label_drift_val->setText( QString(tr("%1 pixels")).arg(num) );
  }
  settingChanged(); 
}

// === PRIVATE ===
void moused_page::updateCurrentSettings(QJsonObject obj){
  //qDebug() << "Update Settings:" << obj;
  double num = obj.value("accel_exponential").toString().toDouble();
  //Convert the double into a slider integer: 
  // EQUATION: X = (Y-1.0)*100   (1.0 <= Y <= 2.0)
  ui->slider_acc_exp->setValue( qRound( (num-1.0)*100) );
  slider_acc_exp_changed();

  num = obj.value("accel_linear").toString().toDouble();
  //Convert the double into a slider integer: 
  // EQUATION:   X = 50 + ln(Y)/0.0921034  (0.01 <= Y <= 2.0)
  num = 50 + ::log(num)/0.0921034;
  ui->slider_acc_linear->setValue( qRound(num) );
  slider_acc_linear_changed();

  num = obj.value("terminate_drift_threshold_pixels").toString().toDouble();
  ui->slider_drift->setValue( qRound(num) );
  slider_drift_changed();

  int index = ui->combo_resolution->findData( obj.value("resolution").toString() );
  if(index>=0){ ui->combo_resolution->setCurrentIndex(index); }

  index = ui->combo_hand->findData( obj.value("hand_mode").toString() );
  if(index>=0){ ui->combo_hand->setCurrentIndex(index); }

  ui->check_emulate_button_3->setChecked( obj.value("emulate_button_3").toString()=="true" );
  ui->check_virtual_scroll->setChecked( obj.value("virtual_scrolling").toString()=="true" );
}
