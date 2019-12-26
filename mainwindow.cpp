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
    mCurrentBox(),
    mSetting(qApp->applicationDirPath()+"/Setting.ini",QSettings::IniFormat),
    mImgOri(),
    mImgScaned(),
    mImgShowPO(),
    mImageTimer(),
    mDeliveryCounter(0),
    mDeliverList()
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

    bool mode = mSetting.value("DbLocalMode",true).toBool();
    mSetting.setValue("DbLocalMode",mode);
    mSetting.sync();

    ui->localModecheckBox->setChecked(mode);

    QTimer::singleShot(1000, this, SLOT(slot_Start_connectDB()));
    mStepIndex = -1;
    updateTable();
    updateStep();


    int tabindex = mSetting.value("TabIndex",0).toInt();
    ui->tabWidget->setCurrentIndex(tabindex);

    ui->deliverPOlineEdit->setMaxLength(POCODE_LENGTH);
    ui->deliverPOlineEdit->setReadOnly(true);


    ui->deliverIndicationlabel_11->setFont(QFont("Microsoft YaHei", 40, 75));
    QPalette pa1;
    pa1.setColor(QPalette::WindowText,Qt::blue);
    ui->deliverIndicationlabel_11->setPalette(pa1);
    ui->deliverIndicationlabel_11->setText(tr("数据库未连接..."));

    //mImgOri.load(qApp->applicationDirPath()+"/lable_orig.png");
    //mImgScaned.load(qApp->applicationDirPath()+"/lable_scaned.png");
    //mImgShowPO.load(qApp->applicationDirPath()+"/lable_show_PO.png");
    mImgOri.load(":/img/images/lable_orig.png");
    mImgScaned.load(":/img/images/lable_scaned.png");
    mImgShowPO.load(":/img/images/lable_show_PO.png");
    ui->lable_png_shower->setPixmap(mImgOri);
    mImageAnimationIndex = 0;
    ui->lable_png_shower->setScaledContents(true);
    ui->lable_png_shower->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    ui->lable_png_shower->setScaledContents(true);
    ui->lable_png_shower->setVisible(true);
    //QTimer::singleShot(1000, this, SLOT(slot_show_scanLable()));
    connect(&mImageTimer,SIGNAL(timeout()),this, SLOT(slot_show_scanLable()));
    mImageTimer.start(1000);


    //ui->tabWidget->tabBar()->hide();
    //QTimer::singleShot(100, this, SLOT(on_modifydeliverPOpushButton_3_clicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slot_show_scanLable()
{
    if( mImageAnimationIndex == 0){
        if( ui->deliverPOlineEdit->text().length() == 0){
            ui->deliverIndicationlabel_11->setText(tr("按F5\n输入右图所示PO号码"));
            ui->lable_png_shower->setPixmap(mImgShowPO);
        }else{
            ui->deliverIndicationlabel_11->setText(tr("扫描外箱条形码\n如右图所示"));
            ui->lable_png_shower->setPixmap(mImgScaned);
        }
        mImageAnimationIndex = 1;
    }else{
        ui->lable_png_shower->setPixmap(mImgOri);
        mImageAnimationIndex = 0;
    }

    if( !mDataBaseHelper.isOpen()){
        ui->deliverIndicationlabel_11->setText(tr("数据库未连接..."));
    }
    //QTimer::singleShot(1000, this, SLOT(slot_show_scanLable()));
}

void MainWindow::slot_Start_connectDB()
{

    ui->localModecheckBox->setDisabled(true);

    if( mDataBaseHelper.isOpen() ){
        if( isUsingLocalDB(&mDataBaseHelper) && ui->localModecheckBox->isChecked() ){
            ui->localModecheckBox->setEnabled(true);
            return;
        }
        if( (!isUsingLocalDB(&mDataBaseHelper)) && (!ui->localModecheckBox->isChecked())){
            ui->localModecheckBox->setEnabled(true);
            return;
        }
        mDataBaseHelper.close();
    }


    if( ui->localModecheckBox->isChecked() ){
        if( openLocalDataBase( &mDataBaseHelper ) ){
            mStepIndex = 0;
        }else{
            mDataBaseHelper.close();
            mStepIndex = -2;
        }
    }else{
        if( ! openRemoteDataBase(&mDataBaseHelper) ){
    #if 1 // 1: warn and or reconnect or warn

            mDataBaseHelper.close();
            mStepIndex = -2;
    #if 1 //1: reconnect frequenctly
            QTimer::singleShot(3000, this, SLOT(slot_Start_connectDB()));
    #endif

    #else
            if( openLocalDataBase( &mDataBaseHelper ) ){
                mStepIndex = 0;
            }else{
                mDataBaseHelper.close();
                mStepIndex = -2;
            }
    #endif
        }else{
            mStepIndex = 0;
        }
    }

    ui->localModecheckBox->setEnabled(true);

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
            ui->importtextBrowser_2->append(tr("当前已连接数据库服务器"));
            return true;
        }
    }

    if( ! mdber->openDataBase("QODBC", "DRIVER={SQL SERVER};SERVER=RSERP;DATABASE=AG", "RSERP", "AG", "AG123456")){
        ui->importtextBrowser_2->append(tr("连接数据库服务器失败"));
        mdber->close();
        //QMessageBox::warning(this,"Error",tr("打开数据库服务器失败"));
        res = false;
    }else{
        ui->importtextBrowser_2->append(tr("已连接数据库服务器"));
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
            ui->importtextBrowser_2->append(tr("当前已连接数据库服务器"));
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
        ui->connectRemoteDBpushButton_2->setText(tr("连接数据库服务器"));
        mStepIndex = -2;
    }else{
        if( isUsingLocalDB(&mDataBaseHelper) ){
            ui->connectLocalDBpushButton->setText(tr("断开本地数据库"));
            ui->connectRemoteDBpushButton_2->setText(tr("连接数据库服务器"));
        }else{
            ui->connectLocalDBpushButton->setText(tr("连接本地数据库"));
            ui->connectRemoteDBpushButton_2->setText(tr("断开数据库服务器"));
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
        ui->indicatelabel_3->setText(tr("连接数据库失败\n请检查网络"));
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

QString MainWindow::parseStringFromScaner(QByteArray bytes)
{
    QString res;

    mStringData.append(bytes);
    //qDebug()<< mStringData.toHex();

    //prase boxcode:   0x2 [xxxx] 0x4
    int index = mStringData.indexOf(0x2);
    if ( index == -1 ) {
        mStringData.clear();
        return res;
    }
    if( index > 0 ) mStringData.remove(0,index);

    int end = mStringData.indexOf(0x4);
    if( end == -1 ){
        return res;
    }
    QByteArray resbyte = mStringData.left(end); //0x2 [xxx]
    resbyte.remove(0,1); // remove 0x2
    res = QString::fromLocal8Bit(resbyte);

    mStringData.remove(0,1);//remove 0x4

    return res;
}

void MainWindow::DeliveryTabHandler(QEvent *e)
{
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);

    if( keyEvent->key() == Qt::Key_F5 ){
        QTimer::singleShot(100, this, SLOT(on_modifydeliverPOpushButton_3_clicked()));
    }else {

        if( ui->deliverPOlineEdit->text().length() != POCODE_LENGTH) return;
        QString scanStr = parseStringFromScaner(keyEvent->text().toLocal8Bit());
        if( scanStr.length() != BOXCODE_LENGTH ) return;

        QString boxCode = scanStr;
        QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyBoxCode(boxCode);

        if(boxlist.size() > 0){
            UnitsBox box = boxlist.at(0);
            box.poCode = ui->deliverPOlineEdit->text();
            if( !mDataBaseHelper.updateBox(box)){
                QMessageBox::warning(this,tr("警告"), tr("更新数据库失败，请重启软件重新扫描"));
                return;
            }
            //show information
            QString log;
            if( mDeliverList.indexOf(boxCode) >= 0) {
                log = tr("箱号%1 重复扫描").arg(boxCode);
            }else{
                mDeliverList.append(boxCode);
                mDeliveryCounter++;
                log = tr("%1, 扫描到箱号%2, 成功录入系统").arg(QString::number(mDeliveryCounter)).arg(boxCode);
            }
            ui->deliverBoxListtextBrowser_2->append(log);
        }else{
            QMessageBox::warning(this,tr("警告"), tr("这包装箱没有入库信息，请进入[组装打包录入]，重新扫描产品"));
        }
    }
}

void MainWindow::PackingTabHandler(QEvent *e)
{
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
    QString scanStr = parseStringFromScaner(keyEvent->text().toLocal8Bit());

    if( mStepIndex == 0){

        if( scanStr.length() != BOXCODE_LENGTH ) return;

        mCurrentBox.reset();
        mCurrentBox.boxQRcode = scanStr;
        if( mCurrentBox.boxQRcode.left(6) != "000000"){
            mCurrentBox.poCode = mCurrentBox.boxQRcode.left(6);
        }

        QString crecord;
        QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyBoxCode(mCurrentBox.boxQRcode);

        if(boxlist.size() > 0){
            //old box, ask for update or rescan?
            if( ui->rePackingcheckBox->isChecked() ){
                //update box record
                if( ! mDataBaseHelper.deleteBoxbyBoxcode(mCurrentBox.boxQRcode)){
                    QMessageBox::warning(this,tr("警告"), tr("此包装箱已扫描过，现删除旧记录失败，请重启软件"));
                    mStepIndex = 3;
                    updateStep();
                    return ;
                }
                ui->textBrowser->append(tr("删除：箱号（%1）的旧记录").arg(mCurrentBox.boxQRcode));
                mStepIndex++;
                updateStep();
                updateTable();
            }else{
                QMessageBox::warning(this,tr("警告"), tr("此包装箱已扫描过，请检查是否有重复外箱标签，如果是返工打包，请勾选【返工打包】，再进行扫描"));
            }
        }else{
            // new box, start next step to scan units
            mStepIndex++;
            updateStep();
            updateTable();
        }

    }else if( mStepIndex == 1){

        if( scanStr.length() != BARCODE_LENGTH ) return;
        QString barcode = scanStr;
        if( -1 == mCurrentBox.RRCcodeList.indexOf(barcode) ){
            //check whether it is existed ? or ignore
            QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyUnitCode(barcode);
            if( boxlist.size() > 0 ){
                if( ui->rePackingcheckBox->isChecked() ){
                    //repacking mode
                    if( ! mDataBaseHelper.deleteBoxbyBoxcode(boxlist.at(0).boxQRcode)){
                        //QMessageBox Msg(QMessageBox::Question, tr("警告"), tr("此产品已保存在数据库，现在删除记录失败，请重启软件"));
                        QMessageBox::warning(this,tr("警告"), tr("此产品已保存在数据库，现删除旧记录失败，请重启软件"));
                        mStepIndex = 3;
                        updateStep();
                        return ;
                    }
                    ui->textBrowser->append(tr("删除：箱号（%1）的旧记录").arg(boxlist.at(0).boxQRcode));
                    mCurrentBox.RRCcodeList.append( barcode );
                    mCurrentBox.RRCcount++;
                }else{
                    //assembly mode
                    QMessageBox::warning(this,tr("警告"), tr("此产品已打包，请检查是否有重复条形码标签，如果是返工打包，请勾选【返工打包】，再进行扫描"));
                }

            }else{
                mCurrentBox.RRCcodeList.append( barcode );
                mCurrentBox.RRCcount++;
            }
        }
        updateTable();
        updateStep();
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
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *e)
{
    QString scanStr;

    if(e->type() == QEvent::KeyPress  && ui->tabWidget->currentIndex() == 0)
    {
        if( this->isActiveWindow() ){
            PackingTabHandler(e);
            return true;
        }else{
            return QMainWindow::eventFilter(watched,e);
        }
    }else if ( e->type() == QEvent::KeyPress  && ui->tabWidget->currentIndex() == 1){
        if( this->isActiveWindow() ){
            DeliveryTabHandler(e);
            return true;
        }else{
            return QMainWindow::eventFilter(watched,e);
        }
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
    QStringList nameFilter;
    nameFilter << "*.txt" << "*.*";
    fileDialog->setWindowTitle(tr("选择导出文件路径"));
    fileDialog->setDirectory(QDir::currentPath());
    fileDialog->setNameFilters(nameFilter);
    fileDialog->setFileMode(QFileDialog::AnyFile);
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec()) {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;

    if( fileNames.size() <= 0 ) return;

    filename = fileNames.at(0);

    //QString POcode = ui->POlineEdit_3->text();
    //if( POcode.length() == 0) POcode = "*";

    QString POcode = "*";

    QMessageBox::information(this,tr("提示"),tr("数据导出时需要一定时间，请耐心等待，不要关闭软件"));
    ui->importtextBrowser_2->append(tr("正在读取数据...."));

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
        //本地数据库的 Boxcode , RRCcodeList , Quantity, PackDate 将更新到数据库服务器中
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
                ui->importtextBrowser_2->append("mergeLocalDBToRemote Error:"+tr("数据库服务器查询到两个相同箱号，数据库有错误！"));
                QMessageBox::warning(this,"mergeLocalDBToRemote Error:",tr("数据库服务器查询到两个相同箱号，数据库有错误！"));
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


void MainWindow::findoutNotExitBarcode()
{
    //read txt file , update to the Database
    QFile txtfile;
    QFile savefile;
    QString record;
    QStringList recordSection;
    QString Barcode = "";


    txtfile.setFileName(ui->boxcodelisttxtfilelineEdit_2->text());
    if( !txtfile.open(QIODevice::ReadWrite | QIODevice::Text) ){
        QMessageBox::warning(this,"Error","打开数据文档失败");
        txtfile.close();
        return ;
    }
    txtfile.seek(0);

    savefile.setFileName("NotExitBarcodeList.txt");
    if( !savefile.open(QIODevice::ReadWrite | QIODevice::Text) ){
        QMessageBox::warning(this,"Error","打开保存文档失败");
        savefile.close();
        return ;
    }
    savefile.seek(0);


    QMessageBox::information(this,tr("提示"),tr("对比数据时需要一定时间，请耐心等待，不要关闭软件"));

    QString str;
    ui->importtextBrowser_2->clear();

    while( txtfile.pos() < txtfile.size() ){
        Barcode = "";
        record = tr(txtfile.readLine());
        //qDebug()<<record;
        recordSection = record.split(QRegExp("[, ;\t\n\r]"),QString::SkipEmptyParts);
        if( recordSection.size() <= 0){
            QMessageBox::information(this,tr("提示"),tr("有未知的行，停止对比"));
            break;
        }
        for( int i=0; i< recordSection.size(); i++){
            str = recordSection.at(i);
            if( str.length() ==  BARCODE_LENGTH){
                Barcode = str;
                //qDebug()<< str;
            }else{
                //ignore useless imformation
            }
        }
        if( Barcode.length() == BARCODE_LENGTH ){
            QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyUnitCode(Barcode);
            if( boxlist.size() > 0 ){
                qDebug()<< str+" is exited";
            }else{
                savefile.write(QString(Barcode+"\n").toLocal8Bit());
                qDebug()<< str+" is not exited";
            }
        }
    }
    txtfile.close();
    savefile.flush();
    savefile.close();
    QMessageBox::information(this,tr("提示"),tr("未存在的Barcode保存在%1").arg(savefile.fileName()));
}

void MainWindow::findoutRepeatBarcode()
{
    //read txt file , update to the Database
    QFile txtfile;
    QFile savefile;
    QString record;
    QStringList recordSection;
    QStringList Barcodelist;
    QString Barcode = "";


    txtfile.setFileName(ui->boxcodelisttxtfilelineEdit_2->text());
    if( !txtfile.open(QIODevice::ReadWrite | QIODevice::Text) ){
        QMessageBox::warning(this,"Error","打开数据文档失败");
        txtfile.close();
        return ;
    }
    txtfile.seek(0);

    savefile.setFileName("NotRepeatBarcodeList.txt");
    if( !savefile.open(QIODevice::ReadWrite | QIODevice::Text) ){
        QMessageBox::warning(this,"Error","打开保存文档失败");
        savefile.close();
        return ;
    }
    savefile.seek(0);


    QMessageBox::information(this,tr("提示"),tr("对比数据时需要一定时间，请耐心等待，不要关闭软件"));

    QString str;
    ui->importtextBrowser_2->clear();

    while( txtfile.pos() < txtfile.size() ){
        Barcode = "";
        record = tr(txtfile.readLine());
        //qDebug()<<record;
        recordSection = record.split(QRegExp("[, ;\t\n\r]"),QString::SkipEmptyParts);
        if( recordSection.size() <= 0){
            QMessageBox::information(this,tr("提示"),tr("有未知的行，停止对比"));
            break;
        }
        for( int i=0; i< recordSection.size(); i++){
            str = recordSection.at(i);
            if( str.length() ==  BARCODE_LENGTH){
                Barcode = str;
                //qDebug()<< str;
            }else{
                //ignore useless imformation
            }
        }
        if( Barcode.length() == BARCODE_LENGTH ){
            if( -1 == Barcodelist.indexOf(Barcode) ){
                Barcodelist.append(Barcode);
                savefile.write(QString(Barcode+"\n").toLocal8Bit());
            }else{
                qDebug()<< str+" is repeat";
            }

        }
    }
    txtfile.close();
    savefile.flush();
    savefile.close();
    QMessageBox::information(this,tr("提示"),tr("未存在的Barcode保存在%1").arg(savefile.fileName()));
}

void MainWindow::setPODateNA()
{
    //read txt file , update to the Database
    QFile txtfile;
    QString record;
    QStringList recordSection;
    QString BoxCode;
    QString POcode = ui->poCodeimportlineEdit_3->text();
    QString DelievryDate = ui->dateEdit->text();
    QString Barcode = "";

    //findoutNotExitBarcode();
    //findoutRepeatBarcode();
    //return;

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
        recordSection = record.split(QRegExp("[, ;\t\n\r]"),QString::SkipEmptyParts);
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

        if( BOXCODE_LENGTH != BoxCode.length()){
            continue;
        }

        totallyCount++;


        //save to database, [boxcode,date,po] each line
        QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyBoxCode(BoxCode);
        if( boxlist.size() > 0 ){
            //exist , update the box record
            UnitsBox box= boxlist.at(0);
            box.poCode = "";
            box.delieverDate = "";
            if( !mDataBaseHelper.updateBox(box) ){
                ui->importtextBrowser_2->append(QString::number(totallyCount)+","+BoxCode+",修改到数据库失败");
                QMessageBox::warning(this,tr("错误"),tr("更新订单信息到数据库失败，请检查错误再重试"));
                txtfile.close();
                return;
            }else{
                importCount++;
            }
        }else{
            QMessageBox::warning(this,tr("错误"),tr("没有找到这箱号%1，请检查错误再重试").arg(BoxCode));
            txtfile.close();
            return;
        }
    }

    ui->importtextBrowser_2->append(tr("共读到%1个箱号，成功导入%2个箱号到数据库中").arg(QString::number(totallyCount),QString::number(importCount)));
    txtfile.close();
    QMessageBox::warning(this,tr("完成"),tr("共读到%1个箱号，成功导入%2个箱号到数据库中").arg(QString::number(totallyCount),QString::number(importCount)));

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

    //findoutNotExitBarcode();
    //findoutRepeatBarcode();
    //setPODateNA();
    //return;

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
        recordSection = record.split(QRegExp("[, ;\t\n\r]"),QString::SkipEmptyParts);
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
                //POcode = str;
            }else if( str.length() == BOXCODE_LENGTH ) {
                BoxCode = str;
            }else if( str.length() == DELIEVERY_DATE_LENGTH){
                //DelievryDate = str;
            }else if( str.length() ==  BARCODE_LENGTH){
                //Barcode = str;
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
    QMessageBox::information(this,tr("提示"),tr("连接数据库服务器有时要较长时间，请耐心等待，不要关闭软件"));
    openRemoteDataBase(& mDataBaseHelper );
}

void MainWindow::on_connectLocalDBpushButton_clicked()
{
    openLocalDataBase(& mDataBaseHelper);
}

void MainWindow::on_mergerLocaldbpushButton_clicked()
{
    if( ! mDataBaseHelper.isOpen() || isUsingLocalDB(& mDataBaseHelper)){
        QMessageBox::warning(this,"Error",tr("请先连接数据库服务器"));
        return;
    }

    ui->importtextBrowser_2->append(tr("开始合并数据..."));
    QMessageBox::information(this,tr("提示"),tr("数据合并时需要一定时间，请耐心等待，不要关闭软件"));
    mergeLocalDBToRemote(mLocalDataBaseFileName);
}

void MainWindow::on_localModecheckBox_toggled(bool checked)
{
    mStepIndex = -1;
    updateTable();
    updateStep();
    QTimer::singleShot(500, this, SLOT(slot_Start_connectDB()));

    mSetting.setValue("DbLocalMode",checked);
    mSetting.sync();
}

void MainWindow::on_pushButton_clicked()
{
    QDir mdir;
    QString filedirPath = QDir::currentPath()+"/Data";
    QString filename =QDir::toNativeSeparators( filedirPath+"/"+"Barcode_Record.txt" );
    mdir.mkpath(filedirPath);

    QString POcode = ui->POlineEdit_3->text();
    QFileDialog *fileDialog = new QFileDialog(this);
    //QStringList nameFilter;
    //nameFilter << "*.txt" << "*.*";
    fileDialog->setWindowTitle(tr("选择导出文件路径"));
    fileDialog->setDirectory(QDir::currentPath());
    //fileDialog->setNameFilters(nameFilter);
    fileDialog->fileSelected(POcode+"BarcodeList.txt");
    fileDialog->setFileMode(QFileDialog::AnyFile);
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec()) {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;

    if( fileNames.size() <= 0 ) return;

    filename = fileNames.at(0);


    if( POcode.length() == 0) POcode = "*";

    QMessageBox::information(this,tr("提示"),tr("数据导出时需要一定时间，请耐心等待，不要关闭软件"));
    ui->importtextBrowser_2->append(tr("正在读取数据...."));

    if( mDataBaseHelper.export_PO_Barcode_ToTxtfile(filename,POcode) )
    {
        QMessageBox::information(this,tr("数据导出"),tr("数据已保存在")+filename);
    }else{
        QMessageBox::warning(this,tr("数据导出"),tr("数据导出失败"));
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    //read database backup file , update to the Database

    QFile txtfile;
    QString record;
    QStringList recordSection;

    QFileDialog *fileDialog = new QFileDialog(this);
    QStringList nameFilter;
    QString filename;
    nameFilter << "*.txt" << "*.*";
    fileDialog->setWindowTitle(tr("数据库文件"));
    fileDialog->setDirectory(QDir::currentPath());
    fileDialog->setNameFilters(nameFilter);
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if (fileDialog->exec()) {
        fileNames = fileDialog->selectedFiles();
    }
    delete fileDialog;

    if( fileNames.size() <= 0 ) return;

    filename = fileNames.at(0);


    txtfile.setFileName(filename);
    if( !txtfile.open(QIODevice::ReadWrite | QIODevice::Text) ){
        QMessageBox::warning(this,"Error","打开数据文档失败");
        txtfile.close();
        return ;
    }
    txtfile.seek(0);

    ui->importtextBrowser_2->append(tr("开始读取数据..."));
    QMessageBox::information(this,tr("提示"),tr("数据读取时需要一定时间，请耐心等待，不要关闭软件"));

    QString str;
    int totallyCount = 0;
    int importCount = 0;
    ui->importtextBrowser_2->clear();


    UnitsBox newbox;
    bool ok;

    while( txtfile.pos() < txtfile.size() ){
        record = QString(txtfile.readLine());
        recordSection = record.split(QRegExp("[ ;\t\n\r]"),QString::SkipEmptyParts);
        if( recordSection.size() != 6){
            ui->importtextBrowser_2->append(tr("有不良数据：")+record);
            continue;
        }
        newbox.boxQRcode = recordSection.at(0);
        newbox.RRCcount =  QString(recordSection.at(1)).toInt(&ok);
        newbox.RRCcodeList = QString(recordSection.at(2)).split(QRegExp("[,]"),QString::SkipEmptyParts);
        newbox.packDate = recordSection.at(3);
        newbox.poCode = recordSection.at(4);
        newbox.delieverDate = recordSection.at(5);

        if( !ok || newbox.poCode.length() != POCODE_LENGTH ||  newbox.boxQRcode.length() != BOXCODE_LENGTH\
                || newbox.packDate.length() != DELIEVERY_DATE_LENGTH || newbox.delieverDate.length() != DELIEVERY_DATE_LENGTH \
                || newbox.RRCcodeList.size() != newbox.RRCcount || QString(newbox.RRCcodeList.at(0)).length() != BARCODE_LENGTH){
            ui->importtextBrowser_2->append(tr("有不良数据：")+record);
            continue;
        }

        totallyCount++;
        //save to database, [boxcode,date,po] each line
        QList<UnitsBox> boxlist = mDataBaseHelper.findBoxbyBoxCode(newbox.boxQRcode);
        if( boxlist.size() > 0 ){
            //exist , update the box record
            if( !mDataBaseHelper.updateBox(newbox) ){
                ui->importtextBrowser_2->append(QString::number(totallyCount)+","+newbox.boxQRcode+":更新到数据库失败");
                QMessageBox::warning(this,tr("错误"),tr("更新信息到数据库失败，请检查错误再重试"));
                break;
            }else{
                importCount++;
            }
        }else{
            if( !mDataBaseHelper.append(newbox) ){
                ui->importtextBrowser_2->append(QString::number(totallyCount)+","+newbox.boxQRcode+",添加到数据库失败");
                QMessageBox::warning(this,tr("错误"),tr("添加信息到数据库失败，请检查错误再重试"));
                break;
            }else{
                //ui->importtextBrowser_2->append(QString::number(totallyCount)+","+BoxCode);
                importCount++;
            }
        }
    }

    ui->importtextBrowser_2->append(tr("共读到%1个箱号，成功导入%2个箱号到数据库中").arg(QString::number(totallyCount),QString::number(importCount)));
    txtfile.close();
    QMessageBox::warning(this,tr("完成"),tr("共读到%1个箱号，成功导入%2个箱号到数据库中").arg(QString::number(totallyCount),QString::number(importCount)));

}

void MainWindow::on_localModecheckBox_clicked(bool checked)
{

}

void MainWindow::on_modifydeliverPOpushButton_3_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("输入PO订单号"),
            tr("PO订单号%1位数:").arg(QString::number(POCODE_LENGTH)),
            QLineEdit::Normal,
            "", &ok);

    if( !ok ) return;

    if( text.length() != POCODE_LENGTH ){
        QMessageBox::warning(this,tr("错误"),tr("PO号不合符要求，需要%1位数").arg(QString::number(POCODE_LENGTH)));
        return;
    }

    ui->deliverPOlineEdit->setText(text);
    ui->deliverIndicationlabel_11->setText(tr("扫描外箱条形码\n如右图所示"));

   ui->deliverBoxListtextBrowser_2->append(tr("PO订单号")+text+":");
   mDeliveryCounter = 0;
   mDeliverList.clear();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    mSetting.setValue("TabIndex",index);
    mSetting.sync();
}
