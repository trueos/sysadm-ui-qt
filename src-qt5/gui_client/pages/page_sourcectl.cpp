//===========================================
//  PC-BSD source code
//  Copyright (c) 2016, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_sourcectl.h"
#include "ui_page_sourcectl.h" //auto-generated from the .ui file

#define IDTAG QString("page_sourcectl_")

sourcectl_page::sourcectl_page(QWidget *parent, sysadm_client *core) : PageWidget(parent, core), ui(new Ui::sourcectl_ui){
  ui->setupUi(this);

}

sourcectl_page::~sourcectl_page(){
  delete ui;
}

//Initialize the CORE <-->Page connections
void sourcectl_page::setupCore(){
  CORE->registerForEvents(sysadm_client::DISPATCHER, true);
}

//Page embedded, go ahead and startup any core requests
void sourcectl_page::startPage(){
  //Let the main window know the name of this page
  emit ChangePageTitle( tr("Source Manager") );
  //Now run any CORE communications
}


// === PRIVATE SLOTS ===
void sourcectl_page::ParseReply(QString id, QString namesp, QString name, QJsonValue args){
  if(!id.startsWith(IDTAG)){ return; } //not to be handled here
  //qDebug() << "Got Updates reply:" << id << namesp << name << args;
  if(id==IDTAG+"list_logs"){
    ui->list_logs->clear();
    QStringList logs = args.toObject().value("listlogs").toObject().keys();
    QString label = tr("%1");
    for(int i=0; i<logs.length(); i++){
      QJsonObject info = args.toObject().value("listlogs").toObject().value(logs[i]).toObject();
      QListWidgetItem *it = new QListWidgetItem( label.arg(info.value("name").toString()) );
        it->setWhatsThis(logs[i]);
      ui->list_logs->addItem(it);
    }
  }else if(id==IDTAG+"read_log"){
    ui->text_log->clear();
    QStringList keys = args.toObject().value("readlogs").toObject().keys();
    QString clog;
    if(ui->list_logs->currentItem()!=0){ clog = ui->list_logs->currentItem()->whatsThis(); }
    if(keys.contains(clog)){
      ui->text_log->setWhatsThis(clog);
      ui->text_log->setPlainText( args.toObject().value("readlogs").toObject().value(clog).toString() );
    }

  }else{
    send_list_branches();
    send_check_updates();
    send_list_settings();
    send_list_logs();
  }
}

void sourcectl_page::ParseEvent(sysadm_client::EVENT_TYPE evtype, QJsonValue val){
  if(evtype==sysadm_client::DISPATCHER && val.isObject()){
    //qDebug() << "Got Dispatcher Event:" << val;
    if(val.toObject().value("event_system").toString()=="sysadm/update"){
      QString state = val.toObject().value("state").toString();
      if(state=="finished"){
        send_start_updates(); //see if there is another update waiting to start, refresh otherwise
      }
      //Update the log widget
      ui->text_up_log->setPlainText( ui->text_up_log->toPlainText() + val.toObject().value("update_log").toString() ); //text
      ui->text_up_log->moveCursor(QTextCursor::End);
      ui->text_up_log->ensureCursorVisible();
      //qDebug() << "Got update event:" << state << val;
      ui->stacked_updates->setCurrentWidget(ui->page_uprunning);
    } //end sysadm/update check
  } //end dispatcher event check
}

void sourcectl_page::send_list_logs(){
  QJsonObject obj;
    obj.insert("action","listlogs");
  communicate(IDTAG+"list_logs", "sysadm", "update",obj);
}

void sourcectl_page::send_read_log(){
  if(ui->list_logs->currentItem()==0){ return; }
  QString citem = ui->list_logs->currentItem()->whatsThis();
  QJsonObject obj;
    obj.insert("action","readlogs");
    obj.insert("logs", citem);
  communicate(IDTAG+"read_log", "sysadm", "update",obj);
}


void sourcectl_page::on_pushButton_DownloadPorts_clicked(){
    QJsonObject obj;
     obj.insert("action","downloadports");
     if(up.startsWith("standalone::")){
       obj.insert("target","standalone");
       obj.insert("tag", up.section("standalone::",-1));
     }else{
       obj.insert("target", up);
     }
   qDebug() << "Send downloadports request:" << obj;
   communicate(IDTAG+"startup", "sysadm", "source",obj);
   //Update the UI right away (so the user knows it is working)
   qDebug() << "Sending update request";
     ui->portstree_log->clear();
}

void sourcectl_page::on_pushButton_UpdatePorts_clicked(){

}

void sourcectl_page::on_pushButton_DeletePorts_clicked(){

}

void sourcectl_page::on_pushButton_DownloadSource_clicked(){

}

void sourcectl_page::on_pushButton_UpdateSource_clicked(){

}

void sourcectl_page::on_pushButton_DeleteSource_clicked(){

}

*/
}
