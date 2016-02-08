//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "control_panel.h"
#include "ui_control_panel.h"

control_panel::control_panel(QWidget *parent, const sysadm_client *core) : PageWidget(parent, core), ui(new Ui::control_panel_ui){
  ui->setupUi(this);	
}

control_panel::~control_panel(){
  
}
