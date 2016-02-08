//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_CONTROL_PANEL_PAGE_H
#define _PCBSD_SYSADM_CLIENT_CONTROL_PANEL_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"
#include "ui_control_panel.h"

namespace Ui{
	class control_panel_ui;
};

class control_panel : public PageWidget{
	Q_OBJECT
public:
	control_panel(QWidget *parent, const sysadm_client *core);
	~control_panel();

private:
	Ui::control_panel_ui *ui;

public slots:
	

};
#endif