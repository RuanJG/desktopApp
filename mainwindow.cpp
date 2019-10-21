#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QTimer>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mStepIndex(0),
    mBarcodeDataBase(),
    mCurrentBox()
{
    ui->setupUi(this);
    ui->indicatelabel_3->setFont(QFont("Microsoft YaHei", 40, 75));
    ui->textBrowser->setFont(QFont("Microsoft YaHei", 12, 50));
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::red);
    ui->indicatelabel_3->setPalette(pa);

    updateTable();
    updateStep();

    QDir mdir;
    QString filedirPath = QDir::currentPath()+"/Data";
    QString dbfilename =QDir::toNativeSeparators( filedirPath+"/"+"Barcode_Record.db" );
    mdir.mkpath(filedirPath);

    //if( !mBoxes.setFilename(filename) ){
    //    QMessageBox::warning(this,"Error",tr("打开数据文档失败"));
    //}
    if( !mBarcodeDataBase.openDataBaseFile(dbfilename) ){
        QMessageBox::warning(this,"Error",tr("打开数据库失败"));
    }
    ui->dateEdit->setDate(QDate::currentDate());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateTable()
{
    ui->textBrowser->clear();
    if( mCurrentBox.boxQRcode.length()>0 )
        ui->textBrowser->append(tr("包装箱条形码：")+mCurrentBox.boxQRcode+"\n");
    for( int i=0 ;i< mCurrentBox.RRCcodeList.size(); i++){
        ui->textBrowser->append(tr("成品")+QString::number(i+1)+"  条形码:  "+mCurrentBox.RRCcodeList[i]+"\n");
    }
}

void MainWindow::updateStep()
{
    switch( mStepIndex ){
    case 0:
        ui->indicatelabel_3->setText(tr("1，扫描包装箱的条形码"));
        break;

    case 1:
        ui->indicatelabel_3->setText(tr("2，扫描第")+QString::number(mCurrentBox.RRCcodeList.size()+1)+"个产品的条形码");
        break;
    case 2:
        ui->indicatelabel_3->setText(tr("3，将以上")+QString::number(mCurrentBox.RRCcount)+tr("个产品打包"));
        break;
    case 3:
        ui->indicatelabel_3->setText(tr("保存数据失败，重启软件，重新扫描 ！"));
        break;
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *e)
{
    if(e->type() == QEvent::KeyPress  && ui->tabWidget->currentIndex() == 0)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        mStringData.append(keyEvent->text().toLocal8Bit());
        qDebug()<< mStringData.toHex();
        if( mStepIndex == 0){
            //prase boxcode:   0x2 [20] 0x4
            int index = mStringData.indexOf(0x2);
            if( index > 0 ) mStringData.remove(0,index);
            if ( index == -1 ) mStringData.clear();

            if( mStringData.length()>= (BOXCODE_LENGTH+2) ){
                if( mStringData.at(BOXCODE_LENGTH+1)!=0x04){
                    mStringData.clear();
                }
                mStringData.remove(0,1);
                mStringData.remove(BOXCODE_LENGTH,mStringData.length()-BOXCODE_LENGTH);

                mCurrentBox.reset();
                mCurrentBox.boxQRcode = QString::fromLocal8Bit(mStringData);

                QString crecord;
                QList<UnitsBox> boxlist = mBarcodeDataBase.findBoxbyBoxCode(mCurrentBox.boxQRcode);

                if(boxlist.size() > 0){
                    //old box, ask for update or rescan
                    bool askforReplace = false;
                    QMessageBox Msg(QMessageBox::Question, tr("警告"), tr("此包装箱已经打包，是否替换更新？"),QMessageBox::Yes | QMessageBox::No, NULL );
                    if( !askforReplace || Msg.exec() == QMessageBox::Yes){
                        //update box record
                        if( ! mBarcodeDataBase.deleteBoxbyBoxcode(mCurrentBox.boxQRcode)){
                            QMessageBox Msg(QMessageBox::Question, tr("警告"), tr("数据库删除记录失败，请重启软件"));
                            mStepIndex = 3;
                            updateStep();
                            return true;
                        }
                        mStepIndex++;
                        updateStep();
                        updateTable();
                    }else{
                        // ignore this box scan, wait for next scan
                    }
                }else{
                    // new box, start next step to scan units
                    mStepIndex++;
                    updateStep();
                    updateTable();
                }
            }
        }else if( mStepIndex == 1){
            //prase units code : 0x2 [13] 0x4
            int index = mStringData.indexOf(0x2);
            if( index > 0 ) mStringData.remove(0,index);
            if ( index == -1 ) mStringData.clear();

            if( mStringData.length() >= 15 ){
                if( mStringData.at(14)==0x04){
                    mStringData.remove(0,1);
                    mStringData.remove(13,mStringData.length()-13);
                    QString barcode = QString::fromLocal8Bit(mStringData);
                    if( 0 > mCurrentBox.RRCcodeList.indexOf(barcode) ){
                        //check whether it is existed ? or ignore
                        QList<UnitsBox> boxlist = mBarcodeDataBase.findBoxbyUnitCode(barcode);
                        if( boxlist.size() > 0 ){
                            //TODO ignore or delete record exited?
                            mCurrentBox.RRCcodeList.append( barcode );
                            mCurrentBox.RRCcount++;
                        }else{
                            mCurrentBox.RRCcodeList.append( barcode );
                            mCurrentBox.RRCcount++;
                        }
                    }
                    updateTable();
                    updateStep();
                    mStringData.clear();
                    if( mCurrentBox.RRCcodeList.size() >= ui->unitsCountspinBox->value()){
                        mCurrentBox.RRCcount = mCurrentBox.RRCcodeList.size();
                        mCurrentBox.packDate =  QDateTime::currentDateTime().toString("yyyyMMdd");
                        if( ! mBarcodeDataBase.append(mCurrentBox) ){
                            QMessageBox::warning(this,"Error",tr("保存数据失败，重启软件，重新扫描"));
                            mStepIndex = 3;
                            updateStep();
                        }else{
                            mStepIndex = 2;
                            updateStep();
                            QTimer::singleShot(5000, this, SLOT(slot_Start_Packing()));
                        }
                    }
                }else{
                    mStringData.clear();
                }
            }
        }

        return true;
    }else{
        return QMainWindow::eventFilter(watched,e);
    }
}

void MainWindow::slot_Start_Packing()
{
    //mMsgbox.close();
    mStepIndex = 0;
    updateStep();

    mCurrentBox.boxQRcode.clear();
    mCurrentBox.RRCcodeList.clear();
    updateTable();

    mStringData.clear();
}

void MainWindow::on_reducepushButton_clicked()
{
    int value = ui->unitsCountspinBox->value();
    if( value > 1)
        ui->unitsCountspinBox->setValue(value-1);
    updateTable();
}

void MainWindow::on_AddpushButton_2_clicked()
{
    int value = ui->unitsCountspinBox->value();
    ui->unitsCountspinBox->setValue(value+1);
    updateTable();
}

void MainWindow::on_exportpushButton_clicked()
{
    QDir mdir;
    QString filedirPath = QDir::currentPath()+"/Data";
    QString filename =QDir::toNativeSeparators( filedirPath+"/"+"Barcode_Record.txt" );
    mdir.mkpath(filedirPath);

    QFileDialog *fileDialog = new QFileDialog(this);
    //QStringList nameFilter;
    //nameFilter << "*.txt" << "*.*";
    fileDialog->setWindowTitle(tr("选择导出文件路径"));
    fileDialog->setDirectory(QDir::currentPath());
    //fileDialog->setNameFilters(nameFilter);
    fileDialog->setFileMode(QFileDialog::AnyFile);
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec()) {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;

    if( fileNames.size() <= 0 ) return;

    filename = fileNames.at(0);

    QString POcode = ui->POlineEdit_3->text();
    if( POcode.length() == 0) POcode = "*";

    if( mBarcodeDataBase.exportToTxtfile(filename,POcode) )
    {
        QMessageBox::information(this,tr("数据导出"),tr("数据已保存在")+filename);
    }else{
        QMessageBox::warning(this,tr("数据导出"),tr("数据导出失败"));
    }
}

void MainWindow::on_dbFileChoisepushButton_2_clicked()
{
    //choise db file to import
    QFileDialog *fileDialog = new QFileDialog(this);
    QStringList nameFilter;
    nameFilter << "*.db" << "*.*";
    fileDialog->setWindowTitle(tr("选择SQLite DB数据库文件"));
    fileDialog->setDirectory(QDir::currentPath());
    fileDialog->setNameFilters(nameFilter);
    //设置可以选择多个文件,默认为只能选择一个文件QFileDialog::ExistingFiles
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    //设置视图模式
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec()) {
        fileNames = fileDialog->selectedFiles();
    }
    if( fileNames.size() > 0 ) ui->dbfilelineEdit->setText(fileNames.at(0));
    delete fileDialog;

}

void MainWindow::on_dbImportpushButton_clicked()
{
    // read the db file, merger into our database

    if( ui->dbfilelineEdit->text().length()==0)
        on_dbFileChoisepushButton_2_clicked();

    QString filename = ui->dbfilelineEdit->text();
    if( filename.length() == 0 ) return;

    QSqlDatabase mDataBase;
    mDataBase = QSqlDatabase::addDatabase("QSQLITE","test");
    mDataBase.setDatabaseName(filename);
    if( ! mDataBase.open() ){
        qDebug()<< "Error:%s"+ mDataBase.lastError().text();
        return ;
    }

    //QString cmd = QString("select * from barcode");
    QString cmd = QString("select * from BarcodeTable");
    QSqlQuery sql_query(mDataBase);
    sql_query.setForwardOnly(true);
    if(!sql_query.exec(cmd) ){
        QMessageBox::warning(this,"Error",tr("查询导入的数据库失败"));
        return;
    }

    UnitsBox box;
    QList<UnitsBox> boxlist;
    bool ok;
    int count = 0;
    ui->importtextBrowser_2->clear();

    while(sql_query.next()) {
        /*
        box.boxQRcode = sql_query.record().value("boxBarcode").toString();
        box.RRCcodeList = sql_query.record().value("unitsBarcode").toString().split(QRegExp("[,\t]"),QString::SkipEmptyParts);
        box.RRCcodeList.removeAt(0);
        box.RRCcount = box.RRCcodeList.size();
        box.packDate = "20190924";
        */


        box.boxQRcode = sql_query.record().value("Boxcode").toString();
        box.RRCcodeList = sql_query.record().value("RRCcodeList").toString().split(',');
        box.RRCcount = sql_query.record().value("Quantity").toInt(&ok);
        if( !ok ) box.RRCcount = -1;
        box.packDate = sql_query.record().value("PackDate").toString();
        box.poCode =  sql_query.record().value("POcode").toString();
        box.delieverDate =  sql_query.record().value("DelieverDate").toString();

        boxlist.clear();
        boxlist = mBarcodeDataBase.findBoxbyBoxCode(box.boxQRcode);
        if( boxlist.size() > 0){
            if( boxlist.at(0).poCode != box.poCode){
                if( !mBarcodeDataBase.updateBox(box) ){
                    ui->importtextBrowser_2->append(box.boxQRcode+": 导入到数据库里失败");
                }else{
                    ui->importtextBrowser_2->append(box.boxQRcode+": 更新到数据库");
                    count++;
                }
            }else{
                //ignore
                ui->importtextBrowser_2->append(box.boxQRcode+tr(" ：已存在，忽略这次更新"));
            }
        }else{
            mBarcodeDataBase.append(box);
            count++;
            ui->importtextBrowser_2->append(tr("%1 添加箱号%2 ").arg(QString::number(count),box.boxQRcode));
        }
    }
    mDataBase.close();
    ui->importtextBrowser_2->append(tr("updated %1 records from import db file").arg(QString::number(count)));
}



void MainWindow::on_boxlistfilechoisepushButton_3_clicked()
{
    //choise db file to import
    QFileDialog *fileDialog = new QFileDialog(this);
    QStringList nameFilter;
    nameFilter << "*.txt" << "*.*";
    fileDialog->setWindowTitle(tr("选择订单的出货箱号列表文件"));
    fileDialog->setDirectory(QDir::currentPath());
    fileDialog->setNameFilters(nameFilter);
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec()) {
        fileNames = fileDialog->selectedFiles();
    }
    if( fileNames.size() > 0 ) ui->boxcodelisttxtfilelineEdit_2->setText(fileNames.at(0));
    delete fileDialog;
}

void MainWindow::on_boxlisttxtfileimportpushButton_4_clicked()
{
    //read txt file , update to the Database
    QFile txtfile;
    QString record;
    QStringList recordSection;
    QString BoxCode;
    QString POcode = ui->poCodeimportlineEdit_3->text();
    QString DelievryDate = ui->dateEdit->text();

    if( POcode.length() != POCODE_LENGTH ){
        QMessageBox::warning(this,"Error",tr("请先填写订单号"));
        return ;
    }

    txtfile.setFileName(ui->boxcodelisttxtfilelineEdit_2->text());
    if( !txtfile.open(QIODevice::ReadWrite | QIODevice::Text) ){
        QMessageBox::warning(this,"Error","打开数据文档失败");
        txtfile.close();
        return ;
    }
    txtfile.seek(0);


    QString str;
    int totallyCount = 0;
    int importCount = 0;
    ui->importtextBrowser_2->clear();

    while( txtfile.pos() < txtfile.size() ){
        record = QString(txtfile.readLine());
        recordSection = record.split(QRegExp("[,\t\n\r]"),QString::SkipEmptyParts);
        if( recordSection.size() <= 0){
            continue;
        }
        BoxCode = "";
        POcode = ui->poCodeimportlineEdit_3->text();
        DelievryDate = ui->dateEdit->date().toString("yyyyMMdd");

        for( int i=0; i< recordSection.size(); i++){
            str = recordSection.at(i);
            qDebug()<< str;
            if( str.length() == POCODE_LENGTH ){
                POcode = str;
            }else if( str.length() == BOXCODE_LENGTH ) {
                BoxCode = str;
            }else if( str.length() == DELIEVERY_DATE_LENGTH){
                DelievryDate = str;
            }else{
                //ignore useless imformation
            }
        }
        if( BoxCode.length() != BOXCODE_LENGTH) continue;

        totallyCount++;
        //save to database
        QList<UnitsBox> boxlist = mBarcodeDataBase.findBoxbyBoxCode(BoxCode);
        if( boxlist.size() > 0 ){
            //exist , update the box record
            UnitsBox box = boxlist.at(0);
            if( box.poCode == POcode && box.delieverDate == DelievryDate){
                //has recorded
                //ui->importtextBrowser_2->append(QString::number(count)+","+BoxCode+tr("已经存在数据库中"));
                importCount++;
                continue;
            }else{
                box.poCode = POcode;
                box.delieverDate = DelievryDate;
                if( !mBarcodeDataBase.updateBox(box) ){
                    ui->importtextBrowser_2->append(QString::number(totallyCount)+","+BoxCode+",添加订单号到数据库失败");
                }else{
                    importCount++;
                }
            }

        }else{
           //no record !!
            ui->importtextBrowser_2->append(QString::number(totallyCount)+","+BoxCode+",生产线上没有这个箱号的记录");
        }
    }

    ui->importtextBrowser_2->append(tr("共读到%1个箱号，成功导入%2个箱号到数据库中").arg(QString::number(totallyCount),QString::number(importCount)));
    txtfile.close();
}

void MainWindow::on_boxcheckpushButton_clicked()
{
    QString boxCode = ui->boxcodeforchecklineEdit->text();
    if( boxCode.length() != BOXCODE_LENGTH) {
        QMessageBox::warning(this,"Error",tr("输入的箱号不对，请重新输入"));
        return;
    }
    QList<UnitsBox> boxlist = mBarcodeDataBase.findBoxbyBoxCode(boxCode);
    UnitsBox box;
    QString rrccodestring;
    ui->importtextBrowser_2->append(tr("箱号%1 共有%2条记录：").arg(boxCode,QString::number(boxlist.size())));
    for( int i=0; i< boxlist.size(); i++) {
        box = boxlist.at(i);
        rrccodestring = mBarcodeDataBase.RRCcodeListToString( box);
        ui->importtextBrowser_2->append(tr("产品数量：%1\n产品条形码：%2\n打包日期：%3\n订单号：%4\n发货日期：%5\n\n").arg(QString::number(box.RRCcount),rrccodestring,box.packDate,box.poCode,box.delieverDate));
    }
}
