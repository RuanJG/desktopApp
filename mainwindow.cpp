#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include "QThread"
#include "QCloseEvent"
#include <QDebug>
#include <QFileDialog>



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    IapMaster(),
    ui(new Ui::MainWindow),
    mSerialport(NULL),
    mSerialMutex(),
    mDecoder(),
    mEncoder(),
    mExcel(),
    mExcelCurrentRow(1),
    mTimer()
{
    ui->setupUi(this);
    ui->currentMaxLineEdit->setText(tr("1000"));
    ui->currentMinLineEdit->setText(tr("0"));
    ui->VolumeMaxlineEdit->setText(tr("70"));
    ui->VolumeMinlineEdit->setText(tr("0"));
    on_setcurrentButton_clicked();
    on_setVolumeButton_clicked();

    update_serial_info();

    saveRecordToExcel(0,"0",0,USER_RES_ERROR_FLAG);

    initIap();
}

MainWindow::~MainWindow()
{
    if( mExcel.IsValid() )
        mExcel.Close();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    on_save_close_Button_clicked();
    event->accept();
}

void MainWindow::on_save_close_Button_clicked()
{
    stopIap();
    close_serial();
    //TODO save file
    if( mExcel.IsValid())
        mExcel.Close();
    qApp->quit();
}

void MainWindow::update_serial_info()
{
    ui->SerialcomboBox->clear();

    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    foreach (const QSerialPortInfo &serialPortInfo, serialPortInfoList) {
        if( serialPortInfo.portName() != NULL){
            ui->SerialcomboBox->addItem(serialPortInfo.portName());
        }


       qDebug() << endl
            << QObject::tr("Port: ") << serialPortInfo.portName() << endl
            << QObject::tr("Location: ") << serialPortInfo.systemLocation() << endl
            << QObject::tr("Description: ") << serialPortInfo.description() << endl
            << QObject::tr("Manufacturer: ") << serialPortInfo.manufacturer() << endl
            << QObject::tr("Vendor Identifier: ") << (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : QByteArray()) << endl
            << QObject::tr("Product Identifier: ") << (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : QByteArray()) << endl
            << QObject::tr("Busy: ") << (serialPortInfo.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) << endl;

    }

}



void MainWindow::on_pushButton_3_clicked()
{
    update_serial_info();
}





bool MainWindow::open_serial_dev(QString portname)
{
    qDebug()<< "serial open " << portname ;

    mSerialport = new QSerialPort();
    mSerialport->setPortName(portname);
    mSerialport->setBaudRate(QSerialPort::Baud57600,QSerialPort::AllDirections);
    mSerialport->setDataBits(QSerialPort::Data8);
    mSerialport->setParity(QSerialPort::NoParity);
    mSerialport->setStopBits(QSerialPort::OneStop);

    bool res = mSerialport->open(QIODevice::ReadWrite);
    qDebug()<< (res?tr("ok"):tr("false")) << endl;

    return res;
}

bool MainWindow::open_serial()
{
    if( mSerialport != NULL){
        close_serial();
    }

    if( ui->SerialcomboBox->currentText() == NULL)
        return false;

    mSerialMutex.lock();
    bool res = open_serial_dev(ui->SerialcomboBox->currentText());
    mSerialMutex.unlock();

    if( ! res ){
        close_serial();
        return false;
    }


    connect( mSerialport,SIGNAL(readyRead()),this, SLOT(solt_mSerial_ReadReady()) );
    return true;
}

void MainWindow::close_serial()
{
    if( mSerialport != NULL){
        mSerialMutex.lock();
        if( mSerialport->isOpen() )
            mSerialport->close();
        delete mSerialport;
        mSerialport = NULL;
        mSerialMutex.unlock();
        qDebug()<< "serial closed" << endl;
    }
}

void MainWindow::solt_mSerial_ReadReady()
{
    if(mSerialport == NULL){
        qDebug() << "on_Serial_ReadReady: Something bad happend !!" << endl;
        return;
    }

    mSerialMutex.lock();
    QByteArray arr = mSerialport->readAll();
    mSerialMutex.unlock();

    handle_Serial_Data( arr );
}


void MainWindow::serial_send_packget( const Chunk &chunk)
{
    mSerialMutex.lock();

    if( mSerialport != NULL && mSerialport->isOpen()){
        Chunk pChunk;
        if ( mEncoder.encode(chunk.getData(),chunk.getSize(),pChunk) ){
            int count = mSerialport->write( (const char*) pChunk.getData(), pChunk.getSize() );
            if( count != pChunk.getSize() ){
                qDebug() << "serial_send_packget: send data to serial false \n" << endl;
            }
        }
    }

    mSerialMutex.unlock();

}



void MainWindow::on_SerialButton_clicked()
{
    if( mSerialport != NULL && mSerialport->isOpen() ){
        close_serial();
        ui->SerialButton->setText(tr("Connect 连接"));
    }else{
        if( open_serial() ){
            ui->SerialButton->setText(tr("Disconnect 断开"));
        }
    }
}



void MainWindow::handle_Serial_Data( QByteArray &bytes )
{
    std::list<Chunk> packgetList;

    //Iap function
    if( this->isIapStarted() ){
        this->iapParse( (unsigned char*) bytes.data(), bytes.count());
        return;
    }

    mDecoder.decode((unsigned char *)bytes.data(),bytes.count(), packgetList);

    qDebug() << "receive: " << bytes.count() << endl;

    std::list<Chunk>::iterator iter;
    for (iter = packgetList.begin(); iter != packgetList.end(); ++iter) {
        const unsigned char *p = iter->getData();
        int cnt = iter->getSize();

        handle_device_message( p,cnt );
    }


}




void MainWindow::handle_device_message( const unsigned char *data, int len )
{
    //head tag
    switch(data[0]){

    case USER_DATA_TAG:{
        //tag[0]+db[0]db[1]db[2]db[3]+current[....]+work_current[0]+error[0]
        if( len != 11 )
            return;

        int db, value,count,error;
        float current;
        memcpy((char*)&db, data+1, 4);
        memcpy( (char*)&current , data+5 , 4);
        count = data[9];
        error = data[10];

        value = current*1000;



        saveRecordToExcel(db, QString::number(current), count, error );
        ui->currentlcdNumber->display(current);
        ui->noiselcdNumber->display(db);

        QPalette pe;
        QString passstr = tr("Pass 合格");
        QString falsestr = tr("False 不良");
        QString errorstr = tr("Test Error");
        if( error & USER_RES_ERROR_FLAG ){
            pe.setColor(QPalette::WindowText,Qt::red);
            ui->currentlabel->setPalette(pe);
            ui->currentlabel->setText(errorstr);
            ui->noiselabel->setPalette(pe);
            ui->noiselabel->setText(errorstr);
        }else{
            if( (error & USER_RES_CURRENT_FALSE_FLAG) != 0 ){
                pe.setColor(QPalette::WindowText,Qt::red);
                ui->currentlabel->setPalette(pe);
                ui->currentlabel->setText(falsestr);
            }else{
                pe.setColor(QPalette::WindowText,Qt::green);
                ui->currentlabel->setPalette(pe);
                ui->currentlabel->setText(passstr);
            }

            if( (error & USER_RES_VOICE_FALSE_FLAG) != 0 ){
                pe.setColor(QPalette::WindowText,Qt::red);
                ui->noiselabel->setPalette(pe);
                ui->noiselabel->setText(falsestr);
            }else{
                pe.setColor(QPalette::WindowText,Qt::green);
                ui->noiselabel->setPalette(pe);
                ui->noiselabel->setText(passstr);
            }

        }


        ui->textBrowser->append("db="+QString::number(db) +"db, current="+QString::number(value)+"mA, count=" + QString::number(count) +
                                ", error="+ QString::number(error) );
        qDebug() << "db=" <<db <<"db, current=" <<current <<endl;

        break;
    }

    case USER_LOG_TAG:{
        QString str = QString::fromUtf8( (const char*)(data+1), len-1);
        ui->textBrowser->append(str);
        break;
    }

    case USER_START_TAG:{
        //ui->textBrowser->clear();
        ui->textBrowser->append("start:");
        ui->currentlcdNumber->display(0);
        ui->noiselcdNumber->display(0);

        QPalette pe;
        pe.setColor(QPalette::WindowText,Qt::black);
        ui->currentlabel->setPalette(pe);
        ui->currentlabel->setText("Testing...");

        pe.setColor(QPalette::WindowText,Qt::black);
        ui->noiselabel->setPalette(pe);
        ui->noiselabel->setText("Testing...");

        break;
    }

    default:{
        ui->textBrowser->append("unknow message!!!");
        break;
    }

    }
}






void MainWindow::saveRecordToExcel(int db, QString current, int count, int error)
{
    bool newsheet = false;

    if( ! mExcel.IsValid() || ! mExcel.IsOpen() ){

        QString filename = "T-REX-Report_"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")+".xlsx";
        filename =QDir::toNativeSeparators( QDir::currentPath()+"\/"+filename );
        //new file ,so alway open sheet 1
        if( !mExcel.Open(filename,1,true) ){
            QMessageBox::warning(this,"Warning",tr("打开Excel文档失败"));
            return;
        }
        newsheet = true;
        mExcelCurrentRow = 1;
    }

    if( mExcelCurrentRow >= mExcel.getMaxRowCount() ){
        unsigned int sheetid = mExcel.getCurrentSheetId();
        if( sheetid >= mExcel.getSheetCount() ){
            QMessageBox::warning(this,"Error",tr("Excel文档己经填满，请保存数据后重新开始程序"));
            return;
        }
        if( !mExcel.openWorkSheet( sheetid+1 ) ){
            QMessageBox::warning(this,"Warning",tr("打开Excel文档的新Sheet页失败"));
            return;
        }
        newsheet = true;
    }

    if( newsheet ){
        mExcelCurrentRow = 1 ; //mExcel.getStartRow();
        int col = 1;
        if( mExcel.SetCellData(mExcelCurrentRow, col++, "Noise(DB)") \
                && mExcel.SetCellData(mExcelCurrentRow, col++, "Current(A)") \
                //&& mExcel.SetCellData(mExcelCurrentRow, col++, "测量次数")
                && mExcel.SetCellData(mExcelCurrentRow, col, "Result"))
        {
            mExcelCurrentRow++;
        }else{
            QMessageBox::warning(this,"Warning",tr("Excel文档写入标题失败"));
            return;
        }
    }


    //不统计通信失败的测试，如果要加入，删除以下判断
    if( error & USER_RES_ERROR_FLAG )
        return;

    //插入到excel表
    QString res = "unknow 未知";
    if( error == 0 ){
        res = tr("Pass 合格");
    }else if( error & USER_RES_ERROR_FLAG ){
        res = tr("Error: 测量无效,通信失败");
    }else{
        res = tr("False: ");
        if( (error & USER_RES_CURRENT_FALSE_FLAG) != 0 )
            res += tr("Current Hight 电流过大,");
        if( (error & USER_RES_VOICE_FALSE_FLAG) != 0 )
            res += tr("Noise Hight 噪声过大,");

    }

    int col = 1;
    if( mExcel.SetCellData(mExcelCurrentRow, col++, db) \
            && mExcel.SetCellData(mExcelCurrentRow, col++, current) \
            //&& mExcel.SetCellData(mExcelCurrentRow, col++, count)
            && mExcel.SetCellData(mExcelCurrentRow, col, res))
    {
        if( error == 0){
            mExcel.setCellBackgroundColor(mExcelCurrentRow,col,QColor(0,255,0));
        }else{
            mExcel.setCellBackgroundColor(mExcelCurrentRow,col,QColor(255,0,0));
        }
        mExcelCurrentRow ++;
    }else{
        QMessageBox::warning(this,"Warning",tr("保存数据失败"));
    }


}








void MainWindow::on_setcurrentButton_clicked()
{
    int max = ui->currentMaxLineEdit->text().toInt();
    int min = ui->currentMinLineEdit->text().toInt();
    if( max > min){

        //tag+max+min
        //send to device
        Chunk chunk;
        float Amax,Amin;
        Amax = (float)max/1000;
        Amin = (float)min/1000;
        chunk.append( USER_CMD_CURRENT_MAXMIN_TAG );
        chunk.append( (const unsigned char *)&Amax, 4);
        chunk.append( (const unsigned char *)&Amin, 4);
        serial_send_packget( chunk );
    }
}

void MainWindow::on_setVolumeButton_clicked()
{
    int max = ui->VolumeMaxlineEdit->text().toInt();
    int min = ui->VolumeMinlineEdit->text().toInt();
    if( max > min){
        //send to device
        Chunk chunk;
        chunk.append( USER_CMD_VOICE_MAXMIN_TAG );
        chunk.append( (const unsigned char *)&max, 4);
        chunk.append( (const unsigned char *)&min, 4);
        serial_send_packget( chunk );
    }
}

void MainWindow::on_ClearTextBrowButton_clicked()
{
    ui->textBrowser->clear();

    //saveRecordToExcel(12,"1.23",3,2);

}

void MainWindow::on_restartButton_clicked()
{
    if( mExcel.IsValid())
        mExcel.Close();

    saveRecordToExcel(0,"0",0,USER_RES_ERROR_FLAG);

    ui->currentlcdNumber->display(0);
    ui->noiselcdNumber->display(0);

    QPalette pe;
    pe.setColor(QPalette::WindowText,Qt::black);
    ui->currentlabel->setPalette(pe);
    ui->currentlabel->setText("?");

    pe.setColor(QPalette::WindowText,Qt::black);
    ui->noiselabel->setPalette(pe);
    ui->noiselabel->setText("?");
}









//Iap function


void MainWindow::iapSendBytes(unsigned char *data, size_t len)
{
    mSerialMutex.lock();

    if( mSerialport != NULL && mSerialport->isOpen()){
        size_t count = mSerialport->write( (const char*) data, len );
        if( count != len ){
            qDebug() << "iapSendHandler: send data to serial false \n" << endl;
        }
    }
    mSerialMutex.unlock();
}


void MainWindow::iapEvent(int EventType, std::string value)
{

    if( EventType == IapMaster::EVENT_TYPE_STARTED){
        ui->textBrowser->append("iap started:");
    }else if ( EventType == IapMaster::EVENT_TYPE_FINISH){
        ui->textBrowser->append("iap finished");
        stopIap();
    }else if( EventType == IapMaster::EVENT_TYPE_FALSE){
        ui->textBrowser->append("iap false: "+ QString::fromStdString(value) );
        stopIap();
    }else if( EventType == IapMaster::EVENT_TYPE_STATUS){
        ui->textBrowser->append(QString::fromStdString(value));
    }
}


void MainWindow::iapTickHandler(){

    if( !this->isIapStarted()){
        stopIap();
        return;
    }
    this->iapTick(10);
}


void MainWindow::initIap()
{
    mTimer.stop();
    this->iapStop();
    connect(&mTimer,SIGNAL(timeout()),this,SLOT(iapTickHandler()));
}

bool MainWindow::startIap()
{
    if( ui->iapFilelineEdit->text().isEmpty()){
        QMessageBox::warning(this,"Warnning",tr("选择IAP文件"));
        return false;
    }

    if( ! this->iapStart(ui->iapFilelineEdit->text().toStdString()) )
        return false;

    mTimer.start(10);
    ui->iapButton->setText("iap Stop");
    return true;
}

void MainWindow::stopIap()
{
    mTimer.stop();
    this->iapStop();
    ui->iapButton->setText("iap Start");
}

void MainWindow::on_iapButton_clicked()
{
    if( this->isIapStarted() ){
        stopIap();

    }else{
        startIap();
    }
}

void MainWindow::on_fileChooseButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Choose Iap file"),
        "",
        tr("*.bin"));

    if( ! filename.isEmpty() ){
        ui->iapFilelineEdit->setText(filename);
    }
}
