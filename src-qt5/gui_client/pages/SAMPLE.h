//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_SAMPLE_PAGE_H
#define _PCBSD_SYSADM_CLIENT_SAMPLE_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"

namespace Ui{
	class sample_ui; //this is the name of the main widget/object in the QtDesigner form
};

class sample_page : public PageWidget{
	Q_OBJECT
public:
	sample_page(QWidget *parent, const sysadm_client *core);
	~sample_page();

private:
	Ui::sample_ui *ui;

};
#endif