//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_NEW_CONNECTION_WIZARD_UI_H
#define _PCBSD_SYSADM_CLIENT_NEW_CONNECTION_WIZARD_UI_H

#include "globals.h"

namespace Ui{
	class NewConnectionWizard;
};

class NewConnectionWizard : public QDialog{
	Q_OBJECT
public:
	NewConnectionWizard(QWidget *parent, QString nickname);
	~NewConnectionWizard();

	//Optional - changing/testing a pre-existing connection
	void LoadPrevious(QString host, QString user);

	sysadm_client *core;
	bool success;
	QString host;

private:
	Ui::NewConnectionWizard *ui;
	QString nick;

private slots:
	void checkInputs(); //activity within the user input boxes

	//core signals/slots
	void coreConnected();
	void coreAuthenticated();
	void coreDisconnected();

	//Buttons
	void on_push_start_test_clicked();
	void on_push_finished_clicked();
	void on_push_cancel_clicked();

};

#endif
