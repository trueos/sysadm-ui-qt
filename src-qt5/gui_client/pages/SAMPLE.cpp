//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "SAMPLE.h"
#include "ui_SAMPLE.h" //auto-generated from the .ui file

sample_page::sample_page(QWidget *parent, const sysadm_client *core) : PageWidget(parent, core), ui(new Ui::control_panel_ui){
  ui->setupUi(this);	
}

sample_page::~sample_page(){
  
}
