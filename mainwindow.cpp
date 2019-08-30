#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mSerialport(NULL),
    mScanerserialport(NULL),
    mSerialMutex(),
    mDecoder(),
    mEncoder(),
    mTimer(),
    mRelayStatus(0),
    QRcodeBytes()
{
    ui->setupUi(this);
    update_serial_info();
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_serialconnectPushButton_clicked()
{
    if( mSerialport != NULL || mScanerserialport != NULL){
       close_serial();
       ui->serialconnectPushButton->setText(tr("连接"));
    }else{
        if( open_serial() )
            ui->serialconnectPushButton->setText(tr("断开"));
    }
}
void MainWindow::on_serialrescanPushButton_2_clicked()
{
    update_serial_info();
}








void MainWindow::update_serial_info()
{
    ui->serialComboBox->clear();
    ui->scanerSerialcomboBox->clear();

    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();


    foreach (const QSerialPortInfo &serialPortInfo, serialPortInfoList) {
        if( serialPortInfo.portName() != NULL){
            ui->serialComboBox->addItem(serialPortInfo.portName());
            ui->scanerSerialcomboBox->addItem(serialPortInfo.portName());
        }

#if 0
        if( serialPortInfo.description() == "Prolific USB-to-Serial Comm Port")
            ui->serialComboBox->setCurrentIndex( ui->serialComboBox->count()-1);
#endif

#if 0
       qDebug() << endl
            << QObject::tr("Port: ") << serialPortInfo.portName() << endl
            << QObject::tr("Location: ") << serialPortInfo.systemLocation() << endl
            << QObject::tr("Description: ") << serialPortInfo.description() << endl
            << QObject::tr("Manufacturer: ") << serialPortInfo.manufacturer() << endl
            << QObject::tr("Vendor Identifier: ") << (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : QByteArray()) << endl
            << QObject::tr("Product Identifier: ") << (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : QByteArray()) << endl
            << QObject::tr("Busy: ") << (serialPortInfo.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) << endl;
#endif
    }

    ui->serialComboBox->addItem(tr("空白"));
    ui->scanerSerialcomboBox->addItem(tr("空白"));
}

bool MainWindow::open_serial()
{

    QString boardSerial = ui->serialComboBox->currentText() ;
    QString scanerSerial = ui->scanerSerialcomboBox->currentText();


    mSerialMutex.lock();

    bool res,res2;
    if( boardSerial != tr("空白") ){
        mSerialport = new QSerialPort();
        mSerialport->setPortName(boardSerial);
        mSerialport->setBaudRate(QSerialPort::Baud115200,QSerialPort::AllDirections);
        mSerialport->setDataBits(QSerialPort::Data8);
        mSerialport->setParity(QSerialPort::NoParity);
        mSerialport->setStopBits(QSerialPort::OneStop);
        res = mSerialport->open(QIODevice::ReadWrite);
    }else{
        res = false;
    }


    if( scanerSerial != tr("空白") && scanerSerial != boardSerial ){
        mScanerserialport = new QSerialPort();
        mScanerserialport->setPortName(scanerSerial);
        mScanerserialport->setBaudRate(QSerialPort::Baud9600,QSerialPort::AllDirections);
        mScanerserialport->setDataBits(QSerialPort::Data8);
        mScanerserialport->setParity(QSerialPort::NoParity);
        mScanerserialport->setStopBits(QSerialPort::OneStop);
        res2 = mScanerserialport->open(QIODevice::ReadWrite);
    }else{
        res2 = false;
    }


    mSerialMutex.unlock();

    if( ! res ){
        if(mSerialport!=NULL)
            delete mSerialport;
    }
    if( ! res2 ){
        if( mScanerserialport!=NULL)
            delete mScanerserialport;
    }

    if( mSerialport != NULL){
        ui->consoleTextBrowser->append("Connect to Control Board successfully");
        connect( mSerialport,SIGNAL(readyRead()),this, SLOT(solt_mSerial_ReadReady()) );
    }

    if( mScanerserialport!=NULL){
        ui->consoleTextBrowser->append("Connect to Scaner successfully");
        connect( mScanerserialport,SIGNAL(readyRead()),this, SLOT(solt_mScanerSerial_ReadReady()) );
    }

    return (res||res2);
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
    }

    if( mScanerserialport != NULL){
        //mSerialMutex.lock();
        if( mScanerserialport->isOpen() )
            mScanerserialport->close();
        delete mScanerserialport;
        mScanerserialport = NULL;
        //mSerialMutex.unlock();
    }
}

void MainWindow::solt_mSerial_ReadReady()
{
    if(mSerialport == NULL){
        //qDebug() << "on_Serial_ReadReady: Something bad happend !!" << endl;
        return;
    }
    mSerialMutex.lock();
    QByteArray arr = mSerialport->readAll();
    mSerialMutex.unlock();

    handle_Serial_Data( arr );
}

void MainWindow::solt_mScanerSerial_ReadReady()
{
    if(mScanerserialport == NULL){
        //qDebug() << "on_Serial_ReadReady: Something bad happend !!" << endl;
        return;
    }
    //mSerialMutex.lock();
    QByteArray arr = mScanerserialport->readAll();
    //mSerialMutex.unlock();

    QRcodeBytes.append(arr);
    if( 0 <= QRcodeBytes.indexOf('\n') || 0 <= QRcodeBytes.indexOf('\r') ){
        QString log = QString::fromLocal8Bit(QRcodeBytes);
        ui->consoleTextBrowser->append("QR Code :"+log);
        QRcodeBytes.clear();
    }

}

void MainWindow::handle_Serial_Data( QByteArray &bytes )
{
    std::list<Chunk> packgetList;

    mDecoder.decode((unsigned char*)bytes.data(),bytes.size(), packgetList);
    std::list<Chunk>::iterator iter;
    for (iter = packgetList.begin(); iter != packgetList.end(); ++iter) {
        const unsigned char *p = iter->getData();
        char tag = (char) p[0];
        const unsigned char *data = p+1;
        int cnt = iter->getSize()-1;


        if( (int)(tag&0xf0) == PMSG_TAG_LOG){
            QString log = QString::fromLocal8Bit( (const char*)data, cnt );
            ui->consoleTextBrowser->append("Log(0x"+QByteArray(&tag,1).toHex()+"):  "+log);
        }else if( (int)(tag&0xf0) == PMSG_TAG_ACK ){
            QString log = QString::fromLocal8Bit( QByteArray((const char*)data, cnt).toHex() );
            ui->consoleTextBrowser->append("Ack(0x"+QByteArray(&tag,1).toHex()+"):  "+log);
        }else if( (int)(tag&0xf0) == PMSG_TAG_DATA ){
            if( tag == PC_TAG_DATA_LED_BRIGHTNESS){
                if( cnt != 48){
                    ui->consoleTextBrowser->append("LED data error");
                }else{
                    unsigned int led_brighness[12];
                    unsigned char *pd = (unsigned char*)led_brighness;
                    for(int i=0 ;i<48;i++){
                        pd[i] = data[i];
                    }
                    ui->led1lineEdit->setText(QString::number(led_brighness[0]));
                    ui->led2lineEdit->setText(QString::number(led_brighness[1]));
                    ui->led3lineEdit->setText(QString::number(led_brighness[2]));
                    ui->led4lineEdit->setText(QString::number(led_brighness[3]));
                    ui->led5lineEdit->setText(QString::number(led_brighness[4]));
                    ui->led6lineEdit->setText(QString::number(led_brighness[5]));
                    ui->led7lineEdit->setText(QString::number(led_brighness[6]));
                    ui->led8lineEdit->setText(QString::number(led_brighness[7]));
                    ui->led9lineEdit->setText(QString::number(led_brighness[8]));
                    ui->led10lineEdit->setText(QString::number(led_brighness[9]));
                    ui->led11lineEdit->setText(QString::number(led_brighness[10]));
                    ui->led12lineEdit->setText(QString::number(led_brighness[11]));
               }
            }else{
                QString log = QString::fromLocal8Bit( QByteArray((const char*)data, cnt).toHex() );
                ui->consoleTextBrowser->append("Data(0x"+QByteArray(&tag,1).toHex()+"):  "+log);
            }
        }else{
            QString log = QString::fromLocal8Bit( QByteArray((const char*)data, cnt).toHex() );
            ui->consoleTextBrowser->append("Unknow(0x"+QByteArray(&tag,1).toHex()+"):  "+log);
        }
    }
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

void MainWindow::serial_send_PMSG( unsigned char tag, unsigned char* data , int data_len)
{
    Chunk chunk;

    chunk.append(tag);
    chunk.append(data,data_len);
    serial_send_packget(chunk);
}

void MainWindow::scaner_send_cmd( unsigned char *data, int len)
{
    //mSerialMutex.lock();

    if( mScanerserialport != NULL && mScanerserialport->isOpen()){
        mScanerserialport->write((char*) data,len );
    }

    //mSerialMutex.unlock();
}


void MainWindow::on_NormalradioButton_toggled(bool checked)
{
    unsigned char data[1]={0xff};
    if( checked ) {
        serial_send_PMSG(PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        ui->consoleTextBrowser->append("normal mode");
    }
}

void MainWindow::on_TestMode2radioButton_2_toggled(bool checked)
{
    unsigned char data[1]={0xff};
    unsigned char data2[1]={2};
    if( checked ){
        serial_send_PMSG(PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG(PC_TAG_CMD_SWITCHES_TESTMODE,data2, sizeof(data2) );
        ui->consoleTextBrowser->append("T2 mode");
    }
}

void MainWindow::on_TestMode3radioButton_3_toggled(bool checked)
{
    unsigned char data[1]={0xff};
    unsigned char data2[1]={3};
    if( checked ){
        serial_send_PMSG(PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG(PC_TAG_CMD_SWITCHES_TESTMODE,data2, sizeof(data2) );
        ui->consoleTextBrowser->append("T3 mode");
    }
}

void MainWindow::on_Testmode4radioButton_4_toggled(bool checked)
{
    unsigned char data[1]={0xff};
    unsigned char data2[1]={4};
    if( checked ){
        serial_send_PMSG(PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG(PC_TAG_CMD_SWITCHES_TESTMODE,data2, sizeof(data2) );
        ui->consoleTextBrowser->append("T3 mode");
    }
}

void MainWindow::on_V15PoweroncheckBox_2_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        data = mRelayStatus | 0x10;
        ui->consoleTextBrowser->append("Power on");
    }else{
        data = mRelayStatus & (~0x10);
        ui->consoleTextBrowser->append("Power off");
    }
    mRelayStatus = data;
    serial_send_PMSG(PC_TAG_CMD_SWITCH, &data, 1);
}

void MainWindow::on_outputtoShavercheckBox_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        ui->outputtoResistorcheckBox_3->setChecked(false);
        ui->outputtoResistorcheckBox_3->clicked(false);
        data = mRelayStatus | 0x1;
        ui->consoleTextBrowser->append("Shaver on");
    }else{
        data = mRelayStatus & (~0x1);
        ui->consoleTextBrowser->append("Shaver off");
    }
    mRelayStatus = data;
    serial_send_PMSG(PC_TAG_CMD_SWITCH, &data, 1);
}

void MainWindow::on_outputtoResistorcheckBox_3_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        ui->outputtoShavercheckBox->setChecked(false);
        ui->outputtoShavercheckBox->clicked(false);
        data = mRelayStatus | 0x2;
        ui->consoleTextBrowser->append("Resistor on");
    }else{
        data = mRelayStatus & (~0x2);
        ui->consoleTextBrowser->append("Resistor off");
    }
    mRelayStatus = data;
    serial_send_PMSG(PC_TAG_CMD_SWITCH, &data, 1);
}

void MainWindow::on_outputtoVmetercheckBox_4_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        ui->measuretoVmetercheckBox_5->setChecked(false);
        ui->measuretoVmetercheckBox_5->clicked(false);
        data = mRelayStatus | 0x8;
        ui->consoleTextBrowser->append("Output to Meter on");
    }else{
        data = mRelayStatus & (~0x8);
        ui->consoleTextBrowser->append("Output to Meter off");
    }
    mRelayStatus = data;
    serial_send_PMSG(PC_TAG_CMD_SWITCH, &data, 1);
}

void MainWindow::on_airpresscheckBox_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        data = mRelayStatus | 0x20;
        ui->consoleTextBrowser->append("Air Cylinder on");
    }else{
        data = mRelayStatus & (~0x20);
        ui->consoleTextBrowser->append("Air Cylinder off");
    }
    mRelayStatus = data;
    serial_send_PMSG(PC_TAG_CMD_SWITCH, &data, 1);
}

void MainWindow::on_measuretoVmetercheckBox_5_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        ui->outputtoVmetercheckBox_4->setChecked(false);
        ui->outputtoVmetercheckBox_4->clicked(false);
        data = mRelayStatus | 0x4;
        ui->consoleTextBrowser->append("Measure to Meter on");
    }else{
        data = mRelayStatus & (~0x4);
        ui->consoleTextBrowser->append("Measure to Meter off");
    }
    mRelayStatus = data;
    serial_send_PMSG(PC_TAG_CMD_SWITCH, &data, 1);
}





void MainWindow::on_meausureVDDcheckBox_6_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        ui->measureLEDcheckBox_7->clicked(false);
        ui->measureLEDcheckBox_7->setCheckState(Qt::CheckState::Unchecked);
        data = 0x0C;
        ui->consoleTextBrowser->append("Meausre VDD Voltage On");
    }else{
        data = 0x0f;
        ui->consoleTextBrowser->append("Meausre VDD Voltage Off");
    }
    serial_send_PMSG(PC_TAG_CMD_SWITCHES_CHANNEL, &data, 1);
}

void MainWindow::on_measureLEDcheckBox_7_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        ui->meausureVDDcheckBox_6->clicked(false);
        ui->meausureVDDcheckBox_6->setCheckState(Qt::CheckState::Unchecked);
        data = 0x03;
        ui->consoleTextBrowser->append("Meausre LED Voltage On");
    }else{
        data = 0x0f;
        ui->consoleTextBrowser->append("Meausre LED Voltage Off");
    }
    serial_send_PMSG(PC_TAG_CMD_SWITCHES_CHANNEL, &data, 1);
}

void MainWindow::on_LedCapturecheckBox_3_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        data = 0x01;
        ui->consoleTextBrowser->append("LED capture On");
    }else{
        data = 0x00;
        ui->consoleTextBrowser->append("LED capture Off");
    }
    serial_send_PMSG(PC_TAG_CMD_CAPTURE_EN, &data, 1);
}

void MainWindow::on_measuretoVmetercheckBox_5_clicked()
{

}

void MainWindow::on_scanerStartpushButton_clicked()
{
    unsigned char cmd[3] = {0x16,0x54,0x0D};
    scaner_send_cmd(cmd,sizeof(cmd));
}
