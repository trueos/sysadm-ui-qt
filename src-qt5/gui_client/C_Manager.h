//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_CONNECTION_MANAGER_UI_H
#define _PCBSD_SYSADM_CLIENT_CONNECTION_MANAGER_UI_H

#include "globals.h"

namespace Ui{
	class C_Manager;
};

class C_Manager : public QMainWindow{
	Q_OBJECT
public:
	C_Manager();
	~C_Manager();

private:
	Ui::C_Manager *ui;
	QActionGroup *radio_acts;
	QTimer *treeTimer;

	void LoadConnectionInfo();

	void checkFilesLoaded();

	//Simplification functions for reading/writing tree widget paths
	QTreeWidgetItem* FindItemParent(QString path);
	void saveGroupItem(QTreeWidgetItem *group); 

	//SSL Library functions for generating the SSL bundle
	bool generateKeyCertBundle(QString bundlefile, QString nickname, QString email, QString passphrase);

private slots:
	void SaveConnectionInfo();

	//Page Changes
	void changePage(QAction*);

	//Connections Page
	void on_tree_conn_itemSelectionChanged();
	void tree_items_changed();
	void on_push_conn_add_clicked();
	void on_push_conn_reset_clicked();
	void on_push_conn_rem_clicked();
	void on_push_conn_export_clicked();
	void on_push_group_add_clicked();
	void on_push_group_rem_clicked();
	void on_push_rename_clicked();

	//SSL Page
	void verify_cert_inputs();
	void on_push_ssl_create_clicked();
	void on_push_ssl_import_clicked();
	void LoadCertView();
	void on_push_ssl_cert_to_file_clicked();
	
signals:
	void SettingsChanged();

};

#endif
