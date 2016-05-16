//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_TASK_PAGE_H
#define _PCBSD_SYSADM_CLIENT_TASK_PAGE_H
#include "../globals.h"
#include "../PageWidget.h"


//==============================================================
//  DON"T FORGET TO ADD YOUR NEW PAGE TO THE "getPage.h" FILE!!!!
//==============================================================

namespace Ui{
	class taskmanager_ui; //this is the name of the main widget/object in the QtDesigner form
};

class taskmanager_page : public PageWidget{
	Q_OBJECT
public:
	taskmanager_page(QWidget *parent, sysadm_client *core);
	~taskmanager_page();

	//Initialize the CORE <-->Page connections
	void setupCore();
	//Page embedded, go ahead and startup any core requests
	void startPage();
		
	QString pageID(){ return "taskmanager"; } //ID is used to identify which type of page this is
	void ParseReply(QString, QString, QString, QJsonValue);

private:
	Ui::taskmanager_ui *ui;

	void parsePIDS(QJsonObject);
	void ShowMemInfo(int active, int cache, int freeM, int inactive, int wired);
	void ShowCPUInfo(int tot, QList<int> percs); //all values are 0-100
	void ShowCPUTempInfo(QStringList temps); //Temperature info
	QTimer *proctimer;

private slots:
	void slotRequestProcInfo();
	void slotRequestMemInfo();
	void slotRequestCPUInfo();
	void slotRequestCPUTempInfo();

	void slot_kill_proc();

};

/*
 * Virtual class for managing the sort of folders/files items. The problem with base class is that it only manages texts fields and
 * we have dates and sizes.
 *
 * On this class, we overwrite the function operator<.
 */

class CQTreeWidgetItem : public QTreeWidgetItem {
public:
    CQTreeWidgetItem(int type = Type) : QTreeWidgetItem(type) {}
    CQTreeWidgetItem(const QStringList & strings, int type = Type) : QTreeWidgetItem(strings, type) {}
    CQTreeWidgetItem(QTreeWidget * parent, int type = Type) : QTreeWidgetItem(parent, type) {}
    CQTreeWidgetItem(QTreeWidget * parent, const QStringList & strings, int type = Type) : QTreeWidgetItem(parent, strings, type) {}
    CQTreeWidgetItem(QTreeWidget * parent, QTreeWidgetItem * preceding, int type = Type) : QTreeWidgetItem(parent, preceding, type) {}
    CQTreeWidgetItem(QTreeWidgetItem * parent, int type = Type) : QTreeWidgetItem(parent, type) {}
    CQTreeWidgetItem(QTreeWidgetItem * parent, const QStringList & strings, int type = Type) : QTreeWidgetItem(parent, strings, type) {}
    CQTreeWidgetItem(QTreeWidgetItem * parent, QTreeWidgetItem * preceding, int type = Type) : QTreeWidgetItem(parent, preceding, type) {}
    virtual ~CQTreeWidgetItem() {}
    inline virtual bool operator<(const QTreeWidgetItem &tmp) const {
      int column = this->treeWidget()->sortColumn();
      // We are in numerical text
      if(column == 3 || column==8 || column==10){
        QString text = this->text(column);
        QString text_tmp = tmp.text(column);
	text=text.remove("%"); text_tmp=text_tmp.remove("%");
	//Turn the two texts into numbers for comparison
        double filesize, filesize_tmp;
        if(text.isEmpty()){ filesize = -1; }
        else{ filesize = text.toDouble(); }
        if(text_tmp.isEmpty()){ filesize_tmp = -1; }
        else{ filesize_tmp = text_tmp.toDouble(); }
	//return the comparison
        return filesize < filesize_tmp;
      }
      // In other cases, we trust base class implementation
      return QTreeWidgetItem::operator<(tmp);
    }
};
#endif
