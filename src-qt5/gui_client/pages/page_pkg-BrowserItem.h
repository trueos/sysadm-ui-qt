//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _PCBSD_SYSADM_CLIENT_PKG_BROWSER_ITEM_H
#define _PCBSD_SYSADM_CLIENT_PKG_BROWSER_ITEM_H

#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QMouseEvent>

class BrowserItem : public QWidget{
	Q_OBJECT
public:
	BrowserItem(QWidget *parent, QString ID);
	~BrowserItem();
	
	QString ID();
	void setText(QString obj, QString text); //obj: "name","version","comment"
	QLabel* iconLabel();
	void setInteraction(int stat); //stat: 0-installed, 1-not_installed, 2-pending

private:
	QString objID;
	QLabel *icon, *version, *name, *comment;
	QToolButton *tool_install, *tool_remove;

private slots:
	void install_clicked();
	void remove_clicked();

signals:
	void InstallClicked(QString); //ID output
	void RemoveClicked(QString); //ID output
	void ItemClicked(QString); //ID output

protected:
	void mouseReleaseEvent(QMouseEvent *ev){
	  if(ev->button()==Qt::LeftButton){ emit ItemClicked(objID); }
	  else{ QWidget::mouseReleaseEvent(ev); }
	}

};

#endif