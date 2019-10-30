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
    mDataBaseHelper(),
    mCurrentBox()
{
    ui->setupUi(this);
    ui->indicatelabel_3->setFont(QFont("Microsoft YaHei", 40, 75));
    ui->textBrowser->setFont(QFont("Microsoft YaHei", 12, 50));
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::red);
    ui->indicatelabel_3->setPalette(pa);

    ui->dateEdit->setDate(QDate::currentDate());

    QDir mdir;
    QString filedirPath = QDir::currentPath()+"/Data";
    mdir.mkpath(filedirPath);
    mLocalDataBaseFileName =QDir::toNativeSeparators( filedirPath+"/"+"Barcode_Record.db" );

    QTimer::singleShot(1000, this, SLOT(slot_Start_connectDB()));
    mStepIndex = -1;
    updateTable();
    updateStep();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slot_Start_connectDB()
{
    if( ! openRemoteDataBase(&mDataBaseHelper) ){
        mDataBaseHelper.close();
        mStepIndex = -2;
        /*
        if( openLocalDataBase( &mDataBaseHelper ) ){
            mStepIndex = 0;
        }else{
        }
        */
    }else{
        mStepIndex = 0;
    }

    updateTable();
    updateStep();
}

bool MainWindow::isUsingLocalDB(DataBaseHelper *mdber)
{
    return (mdber->isOpen() && mdber->getDriverName() == "QSQLITE");
}

bool MainWindow::openRemoteDataBase( DataBaseHelper *mdber)
{
    bool res = true;

    if( mdber->isOpen() ){
        if( isUsingLocalDB(mdber) ){
            mdber->close();
        }else{
            ui->importtextBrowser_2->append(tr("当前已连接远程数据库"));
            return true;
        }
    }

    if( ! mdber->openDataBase("QODBC", "DRIVER={SQL SERVER};SERVER=RSERP;DATABASE=AG", "RSERP", "AG", "AG123456")){
        ui->importtextBrowser_2->append(tr("连接远程数据库失败"));
        mdber->close();
        //QMessageBox::warning(this,"Error",tr("打开远程数据库失败"));
        res = false;
    }else{
        ui->importtextBrowser_2->append(tr("已连接远程数据库"));
    }

    updateDBConnectStatus();
    return res;
}

bool MainWindow::openLocalDataBase(DataBaseHelper *mdber)
{

    bool res = true;

    if( mdber->isOpen() ){
        if( !isUsingLocalDB(mdber) ){
            mdber->close();
        }else{
            ui->importtextBrowser_2->append(tr("当前已连接远程数据库"));
            return true;
        }
    }

   if( !mdber->openDataBaseFile(mLocalDataBaseFileName) ){
        ui->importtextBrowser_2->append(tr("连接本地数据库失败"));
        mdber->close();
        //QMessageBox::warning(this,"Error",tr("打开本地数据库失败"));
        res = false;
    }else{
        ui->importtextBrowser_2->append(tr("已连接本地数据库"));
    }

   updateDBConnectStatus();

   return res;
}

void MainWindow::updateDBConnectStatus(){

    if( !mDataBaseHelper.isOpen()){
        ui->connectLocalDBpushButton->setText(tr("连接本地数据库"));
        ui->connectRemoteDBpushButton_2->setText(tr("连接远程数据库"));
        mStepIndex = -2;
    }else{
        if( isUsingLocalDB(&mDataBaseHelper) ){
            ui->connectLocalDBpushButton->setText(tr("断开本地数据库"));
            ui->connectRemoteDBpushButton_2->setText(tr("连接远程数据库"));
        }else{
            ui->connectLocalDBpushButton->setText(tr("连接本地数据库"));
            ui->connectRemoteDBpushButton_2->setText(tr("断开远程数据库"));
        }
        mStepIndex =0;
    }

    mStringData.clear();
    updateTable();
    updateStep();
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

    case -2:
        ui->indicatelabel_3->setText(tr("连接数据库失败\n请检查网络，或设置为离线测试"));
        break;

    case -1:
        ui->indicatelabel_3->setText(tr("正在连接数据库,请稍等..."));
        break;

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
                QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyBoxCode(mCurrentBox.boxQRcode);

                if(boxlist.size() > 0){
                    //old box, ask for update or rescan
                    bool askforReplace = false;
                    QMessageBox Msg(QMessageBox::Question, tr("警告"), tr("此包装箱已经打包，是否替换更新？"),QMessageBox::Yes | QMessageBox::No, NULL );
                    if( !askforReplace || Msg.exec() == QMessageBox::Yes){
                        //update box record
                        if( ! mDataBaseHelper.deleteBoxbyBoxcode(mCurrentBox.boxQRcode)){
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
            //prase units code : 0x2 [BARCODE_LENGTH] 0x4
            int index = mStringData.indexOf(0x2);
            if( index > 0 ) mStringData.remove(0,index);
            if ( index == -1 ) mStringData.clear();

            if( mStringData.length() >= (BARCODE_LENGTH+2) ){
                if( mStringData.at(BARCODE_LENGTH+1)==0x04){
                    mStringData.remove(0,1);
                    mStringData.remove(BARCODE_LENGTH,mStringData.length()-BARCODE_LENGTH);
                    QString barcode = QString::fromLocal8Bit(mStringData);
                    if( 0 > mCurrentBox.RRCcodeList.indexOf(barcode) ){
                        //check whether it is existed ? or ignore
                        QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyUnitCode(barcode);
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
                        if( ! mDataBaseHelper.append(mCurrentBox) ){
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

    if( mDataBaseHelper.exportToTxtfile(filename,POcode) )
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

    ui->importtextBrowser_2->append(tr("开始合并数据..."));
    QMessageBox::information(this,tr("提示"),tr("数据合并时需要一定时间，请耐心等待，不要关闭软件"));
    mergeLocalDBToRemote(filename);
 }

bool MainWindow::mergeLocalDBToRemote(QString localDBFileNmae)
{
    static int counter = 0;

    ui->importtextBrowser_2->clear();

    QSqlDatabase localDataBase;
    localDataBase = QSqlDatabase::addDatabase("QSQLITE",QString("importdb%1").arg(++counter));
    localDataBase.setDatabaseName(localDBFileNmae);
    if( ! localDataBase.open() ){
        ui->importtextBrowser_2->append("mergeLocalDBToRemote Error: "+ localDataBase.lastError().text());
        localDataBase = QSqlDatabase();
        QSqlDatabase::removeDatabase(QString("importdb%1").arg(counter));
        QMessageBox::warning(this,"mergeLocalDBToRemote Error:",tr("打开本地数据库失败"));
        return false;
    }

    QString cmd = QString("select * from BarcodeTable");
    QSqlQuery sql_query(localDataBase);
    sql_query.setForwardOnly(true);
    if(!sql_query.exec(cmd) ){
        ui->importtextBrowser_2->append("mergeLocalDBToRemote Error:"+tr("查询本地数据库失败"));
        QMessageBox::warning(this,"mergeLocalDBToRemote Error:",tr("查询本地数据库失败"));
        localDataBase.close();
        return false;
    }

    UnitsBox box;
    UnitsBox existsBox;
    QList<UnitsBox> boxlist;
    bool ok;
    int count = 0;

    while(sql_query.next()) {
        //本地数据库的 Boxcode , RRCcodeList , Quantity, PackDate 将更新到远程数据库中
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
        boxlist = mDataBaseHelper.findBoxbyBoxCode(box.boxQRcode);
        if( boxlist.size() > 0){
            //update box
            if( boxlist.size()>1){
                ui->importtextBrowser_2->append("mergeLocalDBToRemote Error:"+tr("远程数据库查询到两个相同箱号，数据库有错误！"));
                QMessageBox::warning(this,"mergeLocalDBToRemote Error:",tr("远程数据库查询到两个相同箱号，数据库有错误！"));
                localDataBase.close();
                return false;
            }

            existsBox = boxlist.at(0);
            existsBox.packDate = box.packDate;
            existsBox.RRCcodeList = box.RRCcodeList;
            existsBox.RRCcount = box.RRCcount;

            if( !mDataBaseHelper.updateBox(existsBox) ){
                ui->importtextBrowser_2->append("mergeLocalDBToRemote Error: 箱（"+existsBox.boxQRcode+"）更新到数据库失败");
                QMessageBox::warning(this,"mergeLocalDBToRemote Error:",tr("箱（%1）更新到数据库失败").arg(existsBox.boxQRcode));
                localDataBase.close();
                return false;
            }else{
                count++;
                ui->importtextBrowser_2->append(tr("%1 更新箱号%2 ").arg(QString::number(count),existsBox.boxQRcode));
            }
        }else{
            //append box
            if( ! mDataBaseHelper.append(box) ){
                ui->importtextBrowser_2->append("mergeLocalDBToRemote Error: 箱（"+box.boxQRcode+"）添加到数据库失败");
                QMessageBox::warning(this,"mergeLocalDBToRemote Error:",tr("箱（%1）添加到数据库失败").arg(box.boxQRcode));
                localDataBase.close();
                return false;
            }else{
                count++;
                ui->importtextBrowser_2->append(tr("%1 添加箱号%2 ").arg(QString::number(count),box.boxQRcode));
            }
        }
    }
    localDataBase.close();
    ui->importtextBrowser_2->append(tr("共成功合并 %1 条记录").arg(QString::number(count)));
    QMessageBox::warning(this,tr("完成"),tr("共成功合并 %1 条记录").arg(QString::number(count)));
    return true;
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
    QString Barcode = "";

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

    ui->importtextBrowser_2->append(tr("开始合并数据..."));
    QMessageBox::information(this,tr("提示"),tr("数据合并时需要一定时间，请耐心等待，不要关闭软件"));

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
            }else if( str.length() ==  BARCODE_LENGTH){
                Barcode = str;
            }else{
                //ignore useless imformation
            }
        }
        if( BoxCode.length() != BOXCODE_LENGTH){
            if( Barcode.length() == BARCODE_LENGTH && POcode.length() != 0){
                //the txt file is [barcode,date,po] each line, we make up the boxcode = 0000000barcode
                BoxCode = "0000000"+Barcode;
            }else{
                continue;
            }
        }

        totallyCount++;
        //save to database, [boxcode,date,po] each line
        QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyBoxCode(BoxCode);
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
                if( !mDataBaseHelper.updateBox(box) ){
                    ui->importtextBrowser_2->append(QString::number(totallyCount)+","+BoxCode+",添加订单号到数据库失败");
                    QMessageBox::warning(this,tr("错误"),tr("更新订单信息到数据库失败，请检查错误再重试"));
                    txtfile.close();
                    return;
                }else{
                    importCount++;
                }
            }

        }else{
            if( Barcode.length() == BARCODE_LENGTH && POcode.length() != 0){
                UnitsBox boxnew;
                //the txt file is [barcode,date,po] each line
                boxnew.boxQRcode = BoxCode;
                boxnew.RRCcodeList = Barcode.split(",");
                boxnew.RRCcount = boxnew.RRCcodeList.size();
                boxnew.delieverDate = DelievryDate;
                boxnew.poCode = POcode;
                boxnew.packDate = DelievryDate;
                if( !mDataBaseHelper.append(boxnew) ){
                    ui->importtextBrowser_2->append(QString::number(totallyCount)+","+BoxCode+",添加订单号到数据库失败");
                    QMessageBox::warning(this,tr("错误"),tr("更新订单信息到数据库失败，请检查错误再重试"));
                    txtfile.close();
                    return;
                }else{
                    //ui->importtextBrowser_2->append(QString::number(totallyCount)+","+BoxCode);
                    importCount++;
                }
            }else{
                //no record !!
                 ui->importtextBrowser_2->append(QString::number(totallyCount)+","+BoxCode+",生产线上没有这个箱号的记录");
                 QMessageBox::warning(this,tr("错误"),tr("数据库中没有箱号（%1）的记录，请先通知生产更新数据到服务器").arg(BoxCode));
                 txtfile.close();
                 return;
            }

        }
    }

    ui->importtextBrowser_2->append(tr("共读到%1个箱号，成功导入%2个箱号到数据库中").arg(QString::number(totallyCount),QString::number(importCount)));
    txtfile.close();
    QMessageBox::warning(this,tr("完成"),tr("共读到%1个箱号，成功导入%2个箱号到数据库中").arg(QString::number(totallyCount),QString::number(importCount)));
}

void MainWindow::on_boxcheckpushButton_clicked()
{
    QString boxCode = ui->boxcodeforchecklineEdit->text();
    if( boxCode.length() != BOXCODE_LENGTH) {
        QMessageBox::warning(this,"Error",tr("输入的箱号不对，请重新输入"));
        return;
    }
    QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyBoxCode(boxCode);
    UnitsBox box;
    QString rrccodestring;
    ui->importtextBrowser_2->append(tr("箱号%1 共有%2条记录：").arg(boxCode,QString::number(boxlist.size())));
    for( int i=0; i< boxlist.size(); i++) {
        box = boxlist.at(i);
        rrccodestring = mDataBaseHelper.RRCcodeListToString( box);
        ui->importtextBrowser_2->append(tr("产品数量：%1\n产品条形码：%2\n打包日期：%3\n订单号：%4\n发货日期：%5\n\n").arg(QString::number(box.RRCcount),rrccodestring,box.packDate,box.poCode,box.delieverDate));
    }
}

void MainWindow::on_testDBpushButton_clicked()
{

    if( mDataBaseHelper.isOpen()){
        ui->importtextBrowser_2->append(tr("开始测试数据库："));
        testDatabase(&mDataBaseHelper );
    }

    return;

}



void MainWindow::testDatabase( DataBaseHelper *db)
{
    if( ! db->isOpen() ){
        ui->importtextBrowser_2->append("db open failed");
        return;
    }
    UnitsBox box;
    box.boxQRcode="11111222223333344444";
    box.delieverDate="20000102";
    box.packDate="20000101";
    box.poCode="000000";
    box.RRCcodeList = QString("9171900003921,9171900003966,9171910001111,9171900003953,9171900003962,9171900003917").split(',');
    box.RRCcount = box.RRCcodeList.size();
    db->deleteBoxbyBoxcode(box.boxQRcode);

    ui->importtextBrowser_2->append("add box :"+box.boxQRcode);
    if( ! db->append(box) ){
        ui->importtextBrowser_2->append("add box failed");
        return;
    }

    QList<UnitsBox> boxlist;
    ui->importtextBrowser_2->append("find box by boxcode:"+box.boxQRcode);
    boxlist = db->findBoxbyBoxCode(box.boxQRcode);
    if( boxlist.size() != 1){
        ui->importtextBrowser_2->append("add box failed");
        return;
    }
    ui->importtextBrowser_2->append(boxlist.at(0).boxQRcode+","+boxlist.at(0).poCode);


    ui->importtextBrowser_2->append("update box :"+box.boxQRcode);
    box.poCode = "000111";
    if( !db->updateBox(box)){
        ui->importtextBrowser_2->append("update box failed");
        return;
    }


    ui->importtextBrowser_2->append("find box by unitcode:"+box.RRCcodeList.at(2));
    boxlist = db->findBoxbyUnitCode(box.RRCcodeList.at(2));
    if( boxlist.size() != 1){
        ui->importtextBrowser_2->append("find box by unitcode failed");
        return;
    }
    ui->importtextBrowser_2->append(boxlist.at(0).boxQRcode+","+boxlist.at(0).poCode);

    ui->importtextBrowser_2->append("export export1.txt and export2.txt");
    if( !db->exportToTxtfile(QDir::currentPath()+"/export1.txt", "*")){
        ui->importtextBrowser_2->append("export with * failed");
        return;
    }
    if( !db->exportToTxtfile(QDir::currentPath()+"/export2.txt", box.poCode)){
        ui->importtextBrowser_2->append("export with pocode failed");
        return;
    }

    if( !db->deleteBoxbyBoxcode(box.boxQRcode) ){
        ui->importtextBrowser_2->append("delete box failed");
        return;
    }

    ui->importtextBrowser_2->append("find box by boxcode:"+box.boxQRcode);
    boxlist = db->findBoxbyBoxCode(box.boxQRcode);
    if( boxlist.size() == 0){
        ui->importtextBrowser_2->append("can't' find box ");
    }else{
        ui->importtextBrowser_2->append("delete box failed");
        return;
    }
    ui->importtextBrowser_2->append("database operation OK");
    return;
}



void MainWindow::on_connectRemoteDBpushButton_2_clicked()
{
    ui->importtextBrowser_2->append(tr("开始连接数据库..."));
    QMessageBox::information(this,tr("提示"),tr("连接远程数据库有时要较长时间，请耐心等待，不要关闭软件"));
    openRemoteDataBase(& mDataBaseHelper );
}

void MainWindow::on_connectLocalDBpushButton_clicked()
{
    openLocalDataBase(& mDataBaseHelper);
}

void MainWindow::on_mergerLocaldbpushButton_clicked()
{
    if( ! mDataBaseHelper.isOpen() || isUsingLocalDB(& mDataBaseHelper)){
        QMessageBox::warning(this,"Error",tr("请先连接远程数据库"));
        return;
    }

    ui->importtextBrowser_2->append(tr("开始合并数据..."));
    QMessageBox::information(this,tr("提示"),tr("数据合并时需要一定时间，请耐心等待，不要关闭软件"));
    mergeLocalDBToRemote(mLocalDataBaseFileName);
}
