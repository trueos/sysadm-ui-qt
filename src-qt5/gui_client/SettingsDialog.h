//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_SETTINGS_MANAGER_UI_H
#define _PCBSD_SYSADM_CLIENT_SETTINGS_MANAGER_UI_H

#include "globals.h"

namespace Ui{
	class SettingsDialog;
};

class SettingsDialog : public QMainWindow{
	Q_OBJECT
public:
	SettingsDialog();
	~SettingsDialog();

	static void InitSettings(); //used on app startup *only*

private:
	Ui::SettingsDialog *ui;
	
	void loadCurrentSettings();

	static QString readfile(QString);

private slots:
	void on_combo_styles_activated(int);

};

#endif