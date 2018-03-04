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
    mTimer(),
    mtestTimer()
{
    ui->setupUi(this);

    update_serial_info();

    initIap();

    ui->iapFilelineEdit->setText(settingRead());
}

MainWindow::~MainWindow()
{
    settingWrite(ui->iapFilelineEdit->text());
    delete ui;
}



void MainWindow::update_serial_info()
{
    ui->SerialcomboBox->clear();

    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();

    foreach (const QSerialPortInfo &serialPortInfo, serialPortInfoList) {
        if( serialPortInfo.portName() != NULL){
            ui->SerialcomboBox->addItem(serialPortInfo.portName());
        }

        if( serialPortInfo.description() == "Prolific USB-to-Serial Comm Port")
            ui->SerialcomboBox->setCurrentIndex( ui->SerialcomboBox->count()-1);

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
        ui->SerialButton->setText(tr("连接串口"));
    }else{
        if( open_serial() ){
            ui->SerialButton->setText(tr("断开串口"));
        }
    }
}



void MainWindow::handle_Serial_Data( QByteArray &bytes )
{
    std::list<Chunk> packgetList;

    if( this->isIapStarted() ){
        //Iap function
        this->iapParse( (unsigned char*) bytes.data(), bytes.count());
    }else{
        // display log
        QString log = QString::fromStdString( bytes.toStdString() );
        ui->textBrowser->append(log);
    }
}


void MainWindow::on_ClearTextBrowButton_clicked()
{
    ui->textBrowser->clear();
}










//************************************************  Iap function


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

void MainWindow::iapResetDevice()
{
//    //QMessageBox::warning(this,"Warnning",tr("选择IAP文件"));
//    close_serial();
//    QThread::msleep(1000);
//    QList<QSerialPortInfo> serialPortInfoList ;

//    while(1){
//        serialPortInfoList  = QSerialPortInfo::availablePorts();
//        if( serialPortInfoList.count() > 0) break;
//        QThread::msleep(1000);
//    }
//    open_serial();

    ui->textBrowser->append("start reset");
    //on_SerialButton_clicked();
    //QThread::msleep(2000);
    //on_SerialButton_clicked();
    ui->textBrowser->append("end reset");
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

QString MainWindow::settingRead()
{
    QSettings settings("IAP", "IAP_Master");

    return settings.value("BinFile").toString();
}

void MainWindow::settingWrite(QString data)
{
    QSettings settings("IAP", "IAP_Master");
    //settings.beginGroup("Setting");
    settings.setValue("BinFile",data);
    //settings.endGroup();
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

void MainWindow::on_sendpushButton_clicked()
{
    QByteArray dataArr;


    dataArr = ui->sendlineEdit->text().toLocal8Bit();


    mSerialMutex.lock();

    if( mSerialport != NULL && mSerialport->isOpen()){
        Chunk pChunk;
        mSerialport->write( dataArr.data(), dataArr.size() );
    }

    mSerialMutex.unlock();

}



