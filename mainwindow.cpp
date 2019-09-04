#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include "qmetatype.h"

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
    QRcodeBytes(),
    mTesterThread(NULL),
    mQRcode(),
    mTxtfile()
{
    ui->setupUi(this);
    update_serial_info();

    mTesterThread = new TesterThread();
    connect(mTesterThread, SIGNAL(error(QString)), this, SLOT(testerThread_error(QString)));
    connect(mTesterThread, SIGNAL(log(QString)), this, SLOT(testerThread_log(QString)));
    connect(mTesterThread, SIGNAL(result(TesterRecord)), this, SLOT(testerThread_result(TesterRecord)));
    connect(mTesterThread, SIGNAL(sendSerialCmd(int,unsigned char*,int)), this, SLOT(testerThread_sendSerialCmd(int,unsigned char*,int)));
    connect(this,SIGNAL(testThread_exit()),mTesterThread, SLOT(testThread_exit()) );
    connect(this,SIGNAL(testThread_start(bool)),mTesterThread, SLOT(testThread_start(bool)) );
    connect(this,SIGNAL(debug_step(bool)),mTesterThread, SLOT(debug_step(bool)) );
    connect(this,SIGNAL(update_tester_data(int,unsigned char*)),mTesterThread, SLOT(update_data(int,unsigned char*)));
    mTesterThread->start();

    ui->ErrorcodepushButton->setText(" ");
    ui->ErrorcodepushButton->setStyleSheet("color: red");
    //ui->ErrorcodepushButton->setStyleSheet("background: rgb(0,255,0)");
    ui->testResultpushButton_2->setText(tr("待测"));
    ui->testResultpushButton_2->setStyleSheet("color: black");

    QDir mdir;
    QString filedirPath = QDir::currentPath()+"/reports";
    QString filename =QDir::toNativeSeparators( filedirPath+"/"+"CSWL_Tester_Data.txt" );
    mdir.mkpath(filedirPath);

    mTxtfile.setFileName(filename);
    if( !mTxtfile.open(QIODevice::ReadWrite | QIODevice::Text) ){
        ui->consoleTextBrowser->append("file open false\n");
        QMessageBox::warning(this,"Error",tr("打开数据文档失败"));
        return;
    }

    mTxtfile.seek( mTxtfile.size() );

    if( mTxtfile.size() ==0 ){
        QString title = "Date,QRcode,ErrorCode,VDD";
        title = title+",VLED_100";
        title = title+",VLED_50";
        title = title+",Rload_current(A)";
        title = title+",LED_Animation";
        for( int i=1; i<=12; i++ ) title = title+",LED"+QString::number(i)+"_100";
        for( int i=1; i<=12; i++ ) title = title+",LED"+QString::number(i)+"_50";
        title = title+"\r";
        mTxtfile.write(title.toLocal8Bit());
        mTxtfile.flush();
    }

    mTesterLedBrightness_cnt = 0;
    mTesterMeterValue_cnt = 0;
    mTesterMeterValue = 0;
    for( int i=0 ;i<12; i++){
        mTesterLedBrightness[i] = 0;
    }
}

MainWindow::~MainWindow()
{
    delete ui;

    if(mTesterThread != NULL){
        emit testThread_exit();
        QThread::msleep(100);
    }
    disconnect(mTesterThread, SIGNAL(error(QString)), this, SLOT(testerThread_error(QString)));
    disconnect(mTesterThread, SIGNAL(log(QString)), this, SLOT(testerThread_log(QString)));
    disconnect(mTesterThread, SIGNAL(result(TesterRecord)), this, SLOT(testerThread_result(TesterRecord)));
    disconnect(mTesterThread, SIGNAL(sendSerialCmd(int,unsigned char*,int)), this, SLOT(testerThread_sendSerialCmd(int,unsigned char*,int)));
    disconnect(this,SIGNAL(testThread_exit()),mTesterThread, SLOT(testThread_exit()) );
    disconnect(this,SIGNAL(testThread_start(bool)),mTesterThread, SLOT(testThread_start(bool)) );
    disconnect(this,SIGNAL(debug_step(bool)),mTesterThread, SLOT(debug_step(bool)) );
    disconnect(this,SIGNAL(update_tester_data(int,unsigned char*)),mTesterThread, SLOT(update_data(int,unsigned char*)));
    delete mTesterThread;

}




void MainWindow::update_serial_info()
{
    int serialIndex = 0;
    int scanerIndex = 0;

    ui->serialComboBox->clear();
    ui->scanerSerialcomboBox->clear();

    ui->serialComboBox->addItem(tr("无"));
    ui->scanerSerialcomboBox->addItem(tr("无"));
    serialIndex = 0;
    scanerIndex = 0;

    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    foreach (const QSerialPortInfo &serialPortInfo, serialPortInfoList) {
        if( serialPortInfo.portName() != NULL){
            ui->serialComboBox->addItem(serialPortInfo.portName());
            ui->scanerSerialcomboBox->addItem(serialPortInfo.portName());

            if( serialPortInfo.vendorIdentifier() == 0x27DD){
                scanerIndex = ui->scanerSerialcomboBox->count()-1;
            }
            if( serialPortInfo.vendorIdentifier() == 0x067B){
                serialIndex = ui->serialComboBox->count()-1;
            }
        }
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

    ui->serialComboBox->setCurrentIndex(serialIndex);
    ui->scanerSerialcomboBox->setCurrentIndex(scanerIndex);

    if( serialIndex != 0 && scanerIndex!= 0) on_serialconnectPushButton_clicked();
}

bool MainWindow::open_serial()
{

    QString boardSerial = ui->serialComboBox->currentText() ;
    QString scanerSerial = ui->scanerSerialcomboBox->currentText();


    mSerialMutex.lock();

    bool res,res2;
    if( boardSerial != tr("无") ){
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


    if( scanerSerial != tr("无") && scanerSerial != boardSerial ){
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
        setAlloff();
        QThread::msleep(100);
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
    int index = QRcodeBytes.indexOf('\n');
    int index2 = QRcodeBytes.indexOf('\r');
    if( 0 < index && 0 < index2 ){
        QByteArray arr1 = QRcodeBytes.remove(index,1);
        mQRcode = QString::fromLocal8Bit(arr1.remove(index2,1));
        ui->consoleTextBrowser->append("QR Code :"+mQRcode);
        QRcodeBytes.clear();

        emit update_tester_data(PC_TAG_DATA_QRCODE, (unsigned char*)mQRcode.toLocal8Bit().data());
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
                    unsigned char *pd = (unsigned char*)mLedBrightness;
                    for(int i=0 ;i<48;i++){
                        pd[i] = data[i];
                    }
#if 0
                    for( int j=0; j< 12; j++){
                        mTesterLedBrightness[j] += mLedBrightness[j];
                    }
                    mTesterLedBrightness_cnt++;
                    if( mTesterLedBrightness_cnt >= mTesterThread->LedBrightness_update_mean_cnt){
                        for( int j=0; j< 12; j++){
                            mTesterLedBrightness[j] /= mTesterLedBrightness_cnt;
                            mTesterThread->LedBrightness[j] = mTesterLedBrightness[j];
                            mTesterLedBrightness[j] = 0;
                        }
                        mTesterThread->LedBrightness_update = true;
                        //emit update_tester_data(tag, (unsigned char*)mTesterLedBrightness);
                        mTesterLedBrightness_cnt = 0;
                    }
#else
                    emit update_tester_data(tag, pd);
#endif
                    ui->led1lineEdit->setText(QString::number(mLedBrightness[0]));
                    ui->led2lineEdit->setText(QString::number(mLedBrightness[1]));
                    ui->led3lineEdit->setText(QString::number(mLedBrightness[2]));
                    ui->led4lineEdit->setText(QString::number(mLedBrightness[3]));
                    ui->led5lineEdit->setText(QString::number(mLedBrightness[4]));
                    ui->led6lineEdit->setText(QString::number(mLedBrightness[5]));
                    ui->led7lineEdit->setText(QString::number(mLedBrightness[6]));
                    ui->led8lineEdit->setText(QString::number(mLedBrightness[7]));
                    ui->led9lineEdit->setText(QString::number(mLedBrightness[8]));
                    ui->led10lineEdit->setText(QString::number(mLedBrightness[9]));
                    ui->led11lineEdit->setText(QString::number(mLedBrightness[10]));
                    ui->led12lineEdit->setText(QString::number(mLedBrightness[11]));
               }
            }else if(tag == PC_TAG_DATA_VMETER){
                if( cnt == 4){
                    float Vol;
                    memcpy( (char*)&Vol , data , 4);
#if 0
                    mTesterMeterValue += Vol;
                    mTesterMeterValue_cnt++;
                    if( mTesterMeterValue_cnt >= mTesterThread->VMeter_update_mean_cnt ){
                        mTesterMeterValue /=  mTesterMeterValue_cnt;
                        //emit update_tester_data(tag, (unsigned char*)&mTesterMeterValue);
                        mTesterThread->VMeter_value = mTesterMeterValue;
                        mTesterThread->VMeter_update = true;
                        mTesterMeterValue_cnt = 0;
                        mTesterMeterValue = 0;
                    }
#else
                    emit update_tester_data(tag, (unsigned char*)&Vol);
#endif
                    if( ui->measureLEDcheckBox_7->isChecked()){
                        ui->VledlineEdit_14->setText(QString::number(Vol,'f',4));
                    }else if( ui->meausureVDDcheckBox_6->isChecked()){
                        ui->VddlineEdit_13->setText(QString::number(Vol,'f',4));
                    }else if( ui->outputtoVmetercheckBox_4->isChecked()){
                        ui->VRloadled4lineEdit_2->setText(QString::number(Vol,'f',4));
                    }else{
                        ui->consoleTextBrowser->append("Vmeter:"+QString::number(Vol,'f',4)+"V");
                    }
                }else{
                    ui->consoleTextBrowser->append("Vmeter data error");
                }
            }else if(tag == PC_TAG_DATA_BUTTON){
                if( cnt==1 && data[0]==1){
                    on_startTestpushButton_clicked();
                }else{
                    ui->consoleTextBrowser->append("Start Button data");
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


void MainWindow::sendcmd(int tag, int data)
{
    Chunk chunk;
    chunk.append(tag);
    chunk.append(data);
    serial_send_packget(chunk);
}









void MainWindow::testerThread_sendSerialCmd(int id, unsigned char* data, int len)
{
    if( id == TesterThread::BORAD_ID ){
        Chunk chunk;
        chunk.append(data,len);
        serial_send_packget(chunk);
    }else if( id == TesterThread::SCANER_ID){
        scaner_send_cmd(data,len);
    }else{
        ui->consoleTextBrowser->append("Error: testerThread_sendSerialCmd: Unknow cmd send to ");
    }
}
void MainWindow::testerThread_log(QString str)
{
    ui->autoTesterConsoletextBrowser->append(str);
}
void MainWindow::testerThread_result(TesterRecord res)
{
    if( res.errorCode == TesterThread::ERRORCODE::E0 ){
        ui->ErrorcodepushButton->setText(" ");
        ui->testResultpushButton_2->setText(tr("测试通过"));
        ui->testResultpushButton_2->setStyleSheet("color: green");
    }else{
        ui->ErrorcodepushButton->setText("E"+QString::number(res.errorCode));
        ui->testResultpushButton_2->setText(tr("测试失败"));
        ui->testResultpushButton_2->setStyleSheet("color: red");
    }
    if( mTxtfile.isOpen() ){
        QString record;
        record = record+res.date+",";
        record = record+res.QRcode+",";
        record = record+"E"+QString::number(res.errorCode)+",";
        record = record+QString::number(res.VDD,'f',4)+",";
        record = record+QString::number(res.VLedFull,'f',4)+",";
        record = record+QString::number(res.VLedMid,'f',4)+",";
        record = record+QString::number(res.RloadCurrent,'f',4)+",";
        record = record+(res.LedAnimation?"OK":"NG")+",";
        for( int i=0; i<12; i++ ){
            record = record+QString::number(res.LedFullLevel[i])+",";
        }
        for( int i=0; i<12; i++ ){
            record = record+QString::number(res.LedMidLevel[i])+",";
        }
        record = record+"\r";
        mTxtfile.write(record.toLocal8Bit());
        mTxtfile.flush();
    }

    ui->autoTesterConsoletextBrowser->append("Tester Result : E"+QString::number(res.errorCode));
    ui->erroecodeStringlabel_24->setText( res.errorCodeString );

    ui->startTestpushButton->setText(tr("开始"));
}
void MainWindow::testerThread_error(QString errorStr)
{
    ui->ErrorcodepushButton->setText(" ");
    ui->testResultpushButton_2->setText(tr("测试错误"));
    ui->testResultpushButton_2->setStyleSheet("color: red");
    ui->autoTesterConsoletextBrowser->append("Tester Error: "+errorStr);
    QMessageBox::warning(this,tr("测试错误"),errorStr);
    ui->startTestpushButton->setText(tr("开始"));
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
        //data = mRelayStatus | 0x8;
        setConnectOutVmeter();
        //sendcmd(PC_TAG_CMD_SWITCH,0x38);
        ui->consoleTextBrowser->append("Output to Meter on");
    }else{
        //data = mRelayStatus & (~0x8);
        //sendcmd(PC_TAG_CMD_SWITCH,0x30);
        setDisconnectOutVmeter();
        ui->consoleTextBrowser->append("Output to Meter off");
    }
    //mRelayStatus = data;
    //serial_send_PMSG(PC_TAG_CMD_SWITCH, &data, 1);
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


void MainWindow::on_VmeterStartcheckBox_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        data = 0x01;
        ui->consoleTextBrowser->append("Vmeter On");
    }else{
        data = 0x00;
        ui->consoleTextBrowser->append("Vmeter Off");
    }
    serial_send_PMSG(PC_TAG_CMD_VMETER_READ, &data, 1);
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



void MainWindow::on_DebugpushButton_clicked()
{
    emit debug_step(true);
}

void MainWindow::on_startTestpushButton_clicked()
{
    if( mTesterThread->mStartTest ){
        ui->ErrorcodepushButton->setText(" ");
        ui->testResultpushButton_2->setText(tr("中止测试"));
        ui->testResultpushButton_2->setStyleSheet("color: black");
        emit testThread_start(false);
        ui->startTestpushButton->setText(tr("开始"));
    }else{
        ui->ErrorcodepushButton->setText(" ");
        ui->testResultpushButton_2->setText(tr("测试中..."));
        ui->testResultpushButton_2->setStyleSheet("color: black");
        emit testThread_start(true);
        ui->startTestpushButton->setText(tr("中止"));
    }

}

void MainWindow::on_AutoTestClearLogpushButton_clicked()
{
    ui->autoTesterConsoletextBrowser->clear();
}

void MainWindow::on_ClearLogpushButton_2_clicked()
{
    ui->consoleTextBrowser->clear();
}

void MainWindow::on_debugModeCheckBox_clicked(bool checked)
{
    mTesterThread->mDebugMode = checked;
}
