#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include "QThread"
#include "QCloseEvent"
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mSerialMutex(),
    mCoder()
{
    ui->setupUi(this);
    ui->currentMaxLineEdit->setText(tr("1000"));
    ui->currentMinLineEdit->setText(tr("0"));
    ui->VolumeMaxlineEdit->setText(tr("70"));
    ui->VolumeMinlineEdit->setText(tr("0"));
    on_setcurrentButton_clicked();
    on_setVolumeButton_clicked();


    mSerialport = NULL;
    update_serial_info();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::warning(NULL, "warning", "press 'Save and Close' Botton to quit ", QMessageBox::Yes, QMessageBox::Yes);
    event->ignore();
}

void MainWindow::on_save_close_Button_clicked()
{
    close_serial();
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
    mSerialport->setBaudRate(QSerialPort::Baud115200,QSerialPort::AllDirections);
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


    connect( mSerialport,SIGNAL(readyRead()),this, SLOT(on_Serial_ReadReady()) );
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

void MainWindow::on_Serial_ReadReady()
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

void MainWindow::handle_Serial_Data( QByteArray &bytes )
{
    std::list<Chunk> packgetList;

    mCoder.decode((unsigned char *)bytes.data(),bytes.count(), packgetList);
}

















void MainWindow::on_setcurrentButton_clicked()
{
    int max = ui->currentMaxLineEdit->text().toInt();
    int min = ui->currentMinLineEdit->text().toInt();
    if( max > min){
        ui->CurrentprogressBar->setMaximum(max);
        ui->CurrentprogressBar->setMinimum(min);
    }
}

void MainWindow::on_setVolumeButton_clicked()
{
    int max = ui->VolumeMaxlineEdit->text().toInt();
    int min = ui->VolumeMinlineEdit->text().toInt();
    if( max > min){
        ui->VolumeprogressBar->setMaximum(max);
        ui->VolumeprogressBar->setMinimum(min);
    }
}
