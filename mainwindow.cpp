#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include "qmetatype.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mLeftTester(),
    mRightTester(),
    mTxtfile(),
    mDataMutex(),
    mSetting(qApp->applicationDirPath()+"\/Setting.ini",QSettings::IniFormat),
    mSerialMap()
{
    ui->setupUi(this);

    mRelayStatus=0;


    mLeftTester.id = "left";
    mLeftTester.RelayStatus = 0;
    mLeftTester.Scanerserialport = NULL;
    mLeftTester.Serialport = NULL;
    mLeftTester.TesterThread = new TesterThread();
    connect(mLeftTester.TesterThread, SIGNAL(error(QString)), this, SLOT(leftTesterThread_error(QString)));
    connect(mLeftTester.TesterThread, SIGNAL(log(QString)), this, SLOT(leftTesterThread_log(QString)));
    connect(mLeftTester.TesterThread, SIGNAL(result(TesterRecord)), this, SLOT(leftTesterThread_result(TesterRecord)));
    connect(mLeftTester.TesterThread, SIGNAL(sendSerialCmd(int,QByteArray)), this, SLOT(leftTesterThread_sendSerialCmd(int,QByteArray)));
    connect(this,SIGNAL(leftTestThread_exit()),mLeftTester.TesterThread, SLOT(testThread_exit()) );
    connect(this,SIGNAL(leftTestThread_start(bool)),mLeftTester.TesterThread, SLOT(testThread_start(bool)) );
    connect(this,SIGNAL(leftDebug_step(bool)),mLeftTester.TesterThread, SLOT(debug_step(bool)) );
    connect(this,SIGNAL(leftUpdate_tester_data(int,QByteArray)),mLeftTester.TesterThread, SLOT(update_data(int,QByteArray)));
    mLeftTester.TesterThread->start();

    mRightTester.id = "right";
    mRightTester.RelayStatus = 0;
    mRightTester.Scanerserialport = NULL;
    mRightTester.Serialport = NULL;
    mRightTester.TesterThread = new TesterThread();
    connect(mRightTester.TesterThread, SIGNAL(error(QString)), this, SLOT(rightTesterThread_error(QString)));
    connect(mRightTester.TesterThread, SIGNAL(log(QString)), this, SLOT(rightTesterThread_log(QString)));
    connect(mRightTester.TesterThread, SIGNAL(result(TesterRecord)), this, SLOT(rightTesterThread_result(TesterRecord)));
    connect(mRightTester.TesterThread, SIGNAL(sendSerialCmd(int,QByteArray)), this, SLOT(rightTesterThread_sendSerialCmd(int,QByteArray)));
    connect(this,SIGNAL(rightTestThread_exit()),mRightTester.TesterThread, SLOT(testThread_exit()) );
    connect(this,SIGNAL(rightTestThread_start(bool)),mRightTester.TesterThread, SLOT(testThread_start(bool)) );
    connect(this,SIGNAL(rightDebug_step(bool)),mRightTester.TesterThread, SLOT(debug_step(bool)) );
    connect(this,SIGNAL(rightUpdate_tester_data(int,QByteArray)),mRightTester.TesterThread, SLOT(update_data(int,QByteArray)));
    mRightTester.TesterThread->start();


    ui->ErrorcodepushButton->setText(" ");
    ui->ErrorcodepushButton->setStyleSheet("color: red");
    //ui->ErrorcodepushButton->setStyleSheet("background: rgb(0,255,0)");
    ui->testResultpushButton_2->setText(tr("通讯未连接"));
    ui->testResultpushButton_2->setStyleSheet("color: black");

    ui->ErrorcodepushButton_right->setText(" ");
    ui->ErrorcodepushButton_right->setStyleSheet("color: red");
    //ui->ErrorcodepushButton_right->setStyleSheet("background: rgb(0,255,0)");
    ui->testResultpushButton_right->setText(tr("通讯未连接"));
    ui->testResultpushButton_right->setStyleSheet("color: black");


    update_serial_info();

}

MainWindow::~MainWindow()
{

    if( mLeftTester.Serialport != NULL || mLeftTester.Scanerserialport != NULL )
        on_serialconnectPushButton_clicked();
    if( mRightTester.Serialport != NULL || mRightTester.Scanerserialport != NULL)
        on_serialconnectPushButton_right_clicked();

    if(mLeftTester.TesterThread != NULL){
        emit leftTestThread_exit();
        QThread::msleep(100);
        delete(mLeftTester.TesterThread);
    }
    if( mRightTester.TesterThread != NULL){
        emit rightTestThread_exit();
        QThread::msleep(100);
        delete(mRightTester.TesterThread);
    }

    delete ui;
}


bool MainWindow::saveDataToFile(TesterRecord res){
    QDir mdir;
    QString filedirPath = QDir::currentPath()+"/reports";
    QString filename =QDir::toNativeSeparators( filedirPath+"/"+"CSWL_Tester_Data.txt" );
    mdir.mkpath(filedirPath);

    mDataMutex.lock();

    mTxtfile.setFileName(filename);
    if( !mTxtfile.open(QIODevice::ReadWrite | QIODevice::Text) ){
        ui->consoleTextBrowser->append("file open false\n");
        mTxtfile.close();
        mDataMutex.unlock();
        return false;
    }

    mTxtfile.seek( mTxtfile.size() );

    if( mTxtfile.size() ==0 ){
        QString title = "Date,QRcode,ErrorCode,VDD";
        title = title+",VLED_100";
        title = title+",VLED_50";
        title = title+",Rload_current(A)";
        title = title+",LED_Animation";
        for( int i=1; i<=12; i++ ) title = title+",LED"+QString::number(i)+"_100";
        for( int i=1; i<=12; i++ ) title = title+",LED"+QString::number(i)+"_30";
        title = title+ ",ErrorString";
        title = title+"\r\n";
        mTxtfile.write(title.toLocal8Bit());
        mTxtfile.flush();
    }

    QString record;
    record = record+res.date+",";
    record = record+res.QRcode+",";
    record = record+"E"+QString::number(res.errorCode);
    if( res.errorCodeParameter.length()>0 )
        record += "("+res.errorCodeParameter+"),";
    else
        record += ",";
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
    record = record + res.errorCodeString +",";
    record = record+"\r\n";
    mTxtfile.write(record.toLocal8Bit());
    mTxtfile.flush();

    mTxtfile.close();
    mDataMutex.unlock();
    return true;
}

void MainWindow::update_serial_info()
{
    int serialIndex = 0;
    int scanerIndex = 0;

    if( mLeftTester.Serialport != NULL || mLeftTester.Scanerserialport != NULL )
        on_serialconnectPushButton_clicked();
    if( mRightTester.Serialport != NULL || mRightTester.Scanerserialport != NULL)
        on_serialconnectPushButton_right_clicked();


    ui->serialComboBox->clear();
    ui->scanerSerialcomboBox->clear();
    ui->serialComboBox_right->clear();
    ui->scanerSerialcomboBox_right->clear();

    ui->serialComboBox->addItem(tr("无"));
    ui->scanerSerialcomboBox->addItem(tr("无"));
    ui->serialComboBox_right->addItem(tr("无"));
    ui->scanerSerialcomboBox_right->addItem(tr("无"));


    serialIndex = 0;
    scanerIndex = 0;
    mSerialMap.clear();

    QString leftSerialId, leftScanerID , rightSerialID, rightScanerID, tmpID;
    leftSerialId = mSetting.value("LeftSerialID").toString();
    leftScanerID = mSetting.value("LeftScanerID").toString();
    rightSerialID = mSetting.value("RightSerialID").toString();
    rightScanerID = mSetting.value("RightScanerID").toString();

    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    foreach (const QSerialPortInfo &serialPortInfo, serialPortInfoList) {
        if( serialPortInfo.portName() != NULL){
            ui->serialComboBox->addItem(serialPortInfo.portName());
            ui->scanerSerialcomboBox->addItem(serialPortInfo.portName());
            ui->serialComboBox_right->addItem(serialPortInfo.portName());
            ui->scanerSerialcomboBox_right->addItem(serialPortInfo.portName());

            tmpID = serialPortInfo.portName();// ( serialPortInfo.vendorIdentifier() << 16 ) | serialPortInfo.productIdentifier() );
            mSerialMap.insert(serialPortInfo.portName(), tmpID);
            if( leftSerialId == tmpID){
                ui->serialComboBox->setCurrentIndex(ui->serialComboBox->count()-1);
            }
            if( leftScanerID == tmpID ){
                ui->scanerSerialcomboBox->setCurrentIndex(ui->scanerSerialcomboBox->count()-1);
            }
            if( rightSerialID == tmpID){
                ui->serialComboBox_right->setCurrentIndex(ui->serialComboBox_right->count()-1);
            }
            if( rightScanerID == tmpID){
                ui->scanerSerialcomboBox_right->setCurrentIndex(ui->scanerSerialcomboBox_right->count()-1);
            }
        }
#if 1
       qDebug() << endl
            << QObject::tr("Port: ") << serialPortInfo.portName() << endl
            << QObject::tr("Location: ") << serialPortInfo.serialNumber() << endl
            << QObject::tr("Description: ") << serialPortInfo.description() << endl
            << QObject::tr("Manufacturer: ") << serialPortInfo.manufacturer() << endl
            << QObject::tr("Vendor Identifier: ") << (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : QByteArray()) << endl
            << QObject::tr("Product Identifier: ") << (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : QByteArray()) << endl
            << QObject::tr("Busy: ") << (serialPortInfo.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) << endl;
#endif

    }


    ui->testResultpushButton_right->setText(tr("通讯未连接"));
    ui->testResultpushButton_2->setText(tr("通讯未连接"));

    if( ui->serialComboBox->currentText() != tr("无") && ui->scanerSerialcomboBox->currentText() != tr("无")){
        on_serialconnectPushButton_clicked();
    }
    if( ui->serialComboBox_right->currentText() != tr("无") && ui->scanerSerialcomboBox_right->currentText() != tr("无")){
        on_serialconnectPushButton_right_clicked();
    }

}

bool MainWindow::isLeftTester(TestTargetControler_t *tester)
{
    if( tester->id == "left")
        return true;
    else
        return false;
}

bool MainWindow::open_serial(TestTargetControler_t *tester)
{

    QString boardSerial ;
    QString scanerSerial ;

    if( isLeftTester(tester)){
        boardSerial = ui->serialComboBox->currentText() ;
        scanerSerial = ui->scanerSerialcomboBox->currentText();
    }else{
        boardSerial = ui->serialComboBox_right->currentText() ;
        scanerSerial = ui->scanerSerialcomboBox_right->currentText();
    }

    tester->SerialportMutex.lock();

    bool res,res2;
    if( boardSerial != tr("无") ){
        tester->Serialport = new QSerialPort();
        tester->Serialport->setPortName(boardSerial);
        tester->Serialport->setBaudRate(QSerialPort::Baud115200,QSerialPort::AllDirections);
        tester->Serialport->setDataBits(QSerialPort::Data8);
        tester->Serialport->setParity(QSerialPort::NoParity);
        tester->Serialport->setStopBits(QSerialPort::OneStop);
        res = tester->Serialport->open(QIODevice::ReadWrite);
        if( isLeftTester(tester) ){
            mSetting.setValue("LeftSerialID", mSerialMap.value(boardSerial));
        }else{
            mSetting.setValue("RightSerialID", mSerialMap.value(boardSerial));
        }
    }else{
        res = false;
    }


    if( scanerSerial != tr("无") && scanerSerial != boardSerial ){
        tester->Scanerserialport = new QSerialPort();
         tester->Scanerserialport->setPortName(scanerSerial);
         tester->Scanerserialport->setBaudRate(QSerialPort::Baud9600,QSerialPort::AllDirections);
         tester->Scanerserialport->setDataBits(QSerialPort::Data8);
         tester->Scanerserialport->setParity(QSerialPort::NoParity);
         tester->Scanerserialport->setStopBits(QSerialPort::OneStop);
        res2 =  tester->Scanerserialport->open(QIODevice::ReadWrite);
        if( isLeftTester(tester) ){
            mSetting.setValue("LeftScanerID", mSerialMap.value(scanerSerial));
        }else{
            mSetting.setValue("RightScanerID", mSerialMap.value(scanerSerial));
        }
    }else{
        res2 = false;
    }

    tester->SerialportMutex.unlock();

    if( ! res ){
        if(tester->Serialport!=NULL)
            delete tester->Serialport;
    }
    if( ! res2 ){
        if( tester->Scanerserialport!=NULL)
            delete tester->Scanerserialport;
    }

    if( tester->Serialport != NULL){
        if( isLeftTester(tester)){
            ui->consoleTextBrowser->append(tr("成功连接左测试板"));
            connect( tester->Serialport,SIGNAL(readyRead()),this, SLOT(solt_mSerial_ReadReady()) );
        }else{
            ui->consoleTextBrowser->append(tr("成功连接右测试板"));
            connect( tester->Serialport,SIGNAL(readyRead()),this, SLOT(solt_mSerial_Right_ReadReady()) );
        }
    }

    if( tester->Scanerserialport!=NULL){
        if( isLeftTester(tester)){
            ui->consoleTextBrowser->append(tr("成功连接左扫描枪"));
            connect( tester->Scanerserialport,SIGNAL(readyRead()),this, SLOT(solt_mScanerSerial_ReadReady()) );
        }else{
            ui->consoleTextBrowser->append(tr("成功连接右扫描枪"));
            connect( tester->Scanerserialport,SIGNAL(readyRead()),this, SLOT(solt_mScanerSerial_Right_ReadReady()) );
        }
    }

    return (res||res2);
}

void MainWindow::close_serial(TestTargetControler_t *tester)
{
    if( tester->Serialport != NULL){
        setAlloff();
        QThread::msleep(100);
        tester->SerialportMutex.lock();
        if( tester->Serialport->isOpen() )
            tester->Serialport->close();
        delete tester->Serialport;
        tester->Serialport = NULL;
        tester->SerialportMutex.unlock();
    }

    if( tester->Scanerserialport != NULL){
        //mSerialMutex.lock();
        if( tester->Scanerserialport->isOpen() )
            tester->Scanerserialport->close();
        delete tester->Scanerserialport;
        tester->Scanerserialport = NULL;
        //mSerialMutex.unlock();
    }
}

void MainWindow::solt_mScanerSerial_Right_ReadReady()
{

    if( mRightTester.Scanerserialport == NULL){
        //qDebug() << "on_Serial_ReadReady: Something bad happend !!" << endl;
        return;
    }
    //mSerialMutex.lock();
    QByteArray arr = mRightTester.Scanerserialport->readAll();
    //mSerialMutex.unlock();

    mRightTester.QRcodeBytes.append(arr);
    int index = mRightTester.QRcodeBytes.indexOf('\n');
    int index2 = mRightTester.QRcodeBytes.indexOf('\r');
    if( 0 < index && 0 < index2 ){
        QByteArray arr1 = mRightTester.QRcodeBytes.remove(index,1);
        mRightTester.QRcode = QString::fromLocal8Bit(arr1.remove(index2,1));
        ui->consoleTextBrowser->append("right QR Code :"+mRightTester.QRcode);
        mRightTester.QRcodeBytes.clear();

        emit rightUpdate_tester_data(PC_TAG_DATA_QRCODE, mRightTester.QRcode.toLocal8Bit());
    }
}

void MainWindow::solt_mSerial_Right_ReadReady()
{
    if(mRightTester.Serialport == NULL){
        //qDebug() << "on_Serial_ReadReady: Something bad happend !!" << endl;
        return;
    }
    mRightTester.SerialportMutex.lock();
    QByteArray arr = mRightTester.Serialport->readAll();
    mRightTester.SerialportMutex.unlock();

    handle_Serial_Data(&mRightTester, arr );
}

void MainWindow::solt_mSerial_ReadReady()
{
    if(mLeftTester.Serialport == NULL){
        //qDebug() << "on_Serial_ReadReady: Something bad happend !!" << endl;
        return;
    }
    mLeftTester.SerialportMutex.lock();
    QByteArray arr = mLeftTester.Serialport->readAll();
    mLeftTester.SerialportMutex.unlock();

    handle_Serial_Data( &mLeftTester, arr );
}

void MainWindow::solt_mScanerSerial_ReadReady()
{
    if(mLeftTester.Scanerserialport == NULL){
        //qDebug() << "on_Serial_ReadReady: Something bad happend !!" << endl;
        return;
    }
    //mSerialMutex.lock();
    QByteArray arr = mLeftTester.Scanerserialport->readAll();
    //mSerialMutex.unlock();

    mLeftTester.QRcodeBytes.append(arr);
    int index = mLeftTester.QRcodeBytes.indexOf('\n');
    int index2 = mLeftTester.QRcodeBytes.indexOf('\r');
    if( 0 < index && 0 < index2 ){
        QByteArray arr1 = mLeftTester.QRcodeBytes.remove(index,1);
        mLeftTester.QRcode = QString::fromLocal8Bit(arr1.remove(index2,1));
        ui->consoleTextBrowser->append("left QR Code :"+mLeftTester.QRcode);
        mLeftTester.QRcodeBytes.clear();

        emit leftUpdate_tester_data(PC_TAG_DATA_QRCODE, mLeftTester.QRcode.toLocal8Bit());
    }

}


void MainWindow::handle_Serial_Data( TestTargetControler_t* tester, QByteArray &bytes )
{
    std::list<Chunk> packgetList;
    QString testerTag = isLeftTester(tester)?"Left-":"Right-";


    tester->Decoder.decode((unsigned char*)bytes.data(),bytes.size(), packgetList);
    std::list<Chunk>::iterator iter;
    for (iter = packgetList.begin(); iter != packgetList.end(); ++iter) {
        const unsigned char *p = iter->getData();
        char tag = (char) p[0];
        const unsigned char *data = p+1;
        int cnt = iter->getSize()-1;


        if( (int)(tag&0xf0) == PMSG_TAG_LOG){
            QString log = QString::fromLocal8Bit( (const char*)data, cnt );
            ui->consoleTextBrowser->append(testerTag+"Log(0x"+QByteArray(&tag,1).toHex()+"):  "+log);
        }else if( (int)(tag&0xf0) == PMSG_TAG_ACK ){
            QString log = QString::fromLocal8Bit( QByteArray((const char*)data, cnt).toHex() );
            ui->consoleTextBrowser->append(testerTag+"Ack(0x"+QByteArray(&tag,1).toHex()+"):  "+log);
        }else if( (int)(tag&0xf0) == PMSG_TAG_DATA ){
            if( tag == PC_TAG_DATA_LED_BRIGHTNESS){
                if( cnt != 48){
                    ui->consoleTextBrowser->append(testerTag+" LED data error");
                }else{
                    unsigned char *pd = (unsigned char*) tester->mLedBrightness;
                    for(int i=0 ;i<cnt;i++){
                        pd[i] = data[i];
                    }
                    QByteArray bytes = QByteArray((char*)data,cnt);

                    if( isLeftTester(tester)){
                        emit leftUpdate_tester_data(tag, bytes);
                        ui->led1lineEdit->setText(QString::number(tester->mLedBrightness[0]));
                        ui->led2lineEdit->setText(QString::number(tester->mLedBrightness[1]));
                        ui->led3lineEdit->setText(QString::number(tester->mLedBrightness[2]));
                        ui->led4lineEdit->setText(QString::number(tester->mLedBrightness[3]));
                        ui->led5lineEdit->setText(QString::number(tester->mLedBrightness[4]));
                        ui->led6lineEdit->setText(QString::number(tester->mLedBrightness[5]));
                        ui->led7lineEdit->setText(QString::number(tester->mLedBrightness[6]));
                        ui->led8lineEdit->setText(QString::number(tester->mLedBrightness[7]));
                        ui->led9lineEdit->setText(QString::number(tester->mLedBrightness[8]));
                        ui->led10lineEdit->setText(QString::number(tester->mLedBrightness[9]));
                        ui->led11lineEdit->setText(QString::number(tester->mLedBrightness[10]));
                        ui->led12lineEdit->setText(QString::number(tester->mLedBrightness[11]));
                    }else{
                        emit rightUpdate_tester_data(tag, bytes);
                        ui->led1lineEdit_rigt ->setText(QString::number(tester->mLedBrightness[0]));
                        ui->led2lineEdit_right ->setText(QString::number(tester->mLedBrightness[1]));
                        ui->led3lineEdit_right->setText(QString::number(tester->mLedBrightness[2]));
                        ui->led4lineEdit_right->setText(QString::number(tester->mLedBrightness[3]));
                        ui->led5lineEdit_right->setText(QString::number(tester->mLedBrightness[4]));
                        ui->led6_lineEdit_right->setText(QString::number(tester->mLedBrightness[5]));
                        ui->led7lineEdit_right->setText(QString::number(tester->mLedBrightness[6]));
                        ui->led8lineEdit_right->setText(QString::number(tester->mLedBrightness[7]));
                        ui->led9lineEdit_right->setText(QString::number(tester->mLedBrightness[8]));
                        ui->led10lineEdit_right->setText(QString::number(tester->mLedBrightness[9]));
                        ui->led11lineEdit_right->setText(QString::number(tester->mLedBrightness[10]));
                        ui->led12lineEdit_right->setText(QString::number(tester->mLedBrightness[11]));
                    }


               }
            }else if(tag == PC_TAG_DATA_VMETER){
                if( cnt == 4){
                    float Vol;
                    memcpy( (char*)&Vol , data , 4);
                    QByteArray bytes = QByteArray((char*)data,cnt);

                    if( isLeftTester(tester) ){
                        emit leftUpdate_tester_data(tag, bytes);
                    }else{
                        emit rightUpdate_tester_data(tag, bytes);
                    }
                    if( ui->measureLEDcheckBox_7->isChecked()){
                        if( isLeftTester(tester) ) ui->VledlineEdit_14->setText(QString::number(Vol,'f',4));
                        else ui->VledlineEdit_right->setText(QString::number(Vol,'f',4));
                    }else if( ui->meausureVDDcheckBox_6->isChecked()){
                        if( isLeftTester(tester) ) ui->VddlineEdit_13->setText(QString::number(Vol,'f',4));
                        else ui->VddlineEdit_right->setText(QString::number(Vol,'f',4));
                    }else if( ui->outputtoVmetercheckBox_4->isChecked()){
                        if( isLeftTester(tester) ) ui->VRloadled4lineEdit_2->setText(QString::number(Vol,'f',4));
                        else ui->VRloadled4lineEdit_right->setText(QString::number(Vol,'f',4));
                    }else{
                        ui->consoleTextBrowser->append(testerTag+"Vmeter:"+QString::number(Vol,'f',4)+"V");
                    }
                }else{
                    ui->consoleTextBrowser->append(testerTag+"Vmeter data error");
                }
            }else if(tag == PC_TAG_DATA_BUTTON){
                if( cnt==1 && data[0]==1){
                    on_startTestpushButton_clicked();
                }else{
                    ui->consoleTextBrowser->append("Start Button data error");
                }
            }else{
                QString log = QString::fromLocal8Bit( QByteArray((const char*)data, cnt).toHex() );
                ui->consoleTextBrowser->append(testerTag+"Data(0x"+QByteArray(&tag,1).toHex()+"):  "+log);
            }
        }else{
            QString log = QString::fromLocal8Bit( QByteArray((const char*)data, cnt).toHex() );
            ui->consoleTextBrowser->append(testerTag+"Unknow(0x"+QByteArray(&tag,1).toHex()+"):  "+log);
        }
    }
}

void MainWindow::serial_send_packget( TestTargetControler_t* tester, const Chunk &chunk)
{
    tester->SerialportMutex.lock();

    if( isTesterValiable(tester) ){
        Chunk pChunk;
        if ( tester->Encoder.encode(chunk.getData(),chunk.getSize(),pChunk) ){
            int count = tester->Serialport->write( (const char*) pChunk.getData(), pChunk.getSize() );
            //qDebug() << "cmd: 0x"+QByteArray((const char*) chunk.getData(), chunk.getSize()).toHex();
            qDebug() << "packget: 0x"+QByteArray((const char*) pChunk.getData(), pChunk.getSize()).toHex();
            if( count != pChunk.getSize() ){
                qDebug() << "serial_send_packget: send data to serial false \n" << endl;
            }
        }
    }

    tester->SerialportMutex.unlock();
}

void MainWindow::serial_send_PMSG( TestTargetControler_t* tester, unsigned char tag, unsigned char* data , int data_len)
{
    Chunk chunk;

    chunk.append(tag);
    chunk.append(data,data_len);

    if( isTesterValiable(tester) )
        serial_send_packget(tester, chunk);
}

void MainWindow::scaner_send_cmd( TestTargetControler_t* tester, unsigned char *data, int len)
{
    //mSerialMutex.lock();

    if( isTesterValiable(tester) ){
        tester->Scanerserialport->write((char*) data,len );
    }

    //mSerialMutex.unlock();
}


void MainWindow::sendcmd(int tag, int data)
{
    Chunk chunk;
    chunk.append(tag);
    chunk.append(data);

    if( isTesterValiable(&mLeftTester) )
        serial_send_packget(&mLeftTester, chunk);
    if( isTesterValiable(&mRightTester) )
        serial_send_packget(&mRightTester, chunk);
}




bool MainWindow::isTesterValiable( TestTargetControler_t* tester ){
    if( tester->Serialport!=NULL && tester->Serialport->isOpen() \
            && tester->Scanerserialport!=NULL && tester->Scanerserialport->isOpen()){
        return true;
    }
    return false;
}



void MainWindow::leftTesterThread_sendSerialCmd(int id, QByteArray cmd)
{
    if( id == TesterThread::BORAD_ID ){
        Chunk chunk;
        unsigned char* data = (unsigned char *)cmd.data();
        if( data[0] == PC_TAG_CMD_SWITCH ){
            data[1] &= (~0x20);
            data[1] |= ((mLeftTester.RelayStatus | mRightTester.RelayStatus) & 0x20);
        }
        chunk.append((unsigned char*)cmd.data(),cmd.size());
        //qDebug()<< "left send cmd : 0x"+QByteArray((char*)cmd.data(),cmd.size()).toHex();
        serial_send_packget(&mLeftTester, chunk);
    }else if( id == TesterThread::SCANER_ID){
        scaner_send_cmd(&mLeftTester, (unsigned char*)cmd.data(),cmd.size());
    }else{
        ui->consoleTextBrowser->append("Error: leftTesterThread_sendSerialCmd: Unknow cmd send to ");
    }
}

void MainWindow::rightTesterThread_sendSerialCmd(int id, QByteArray cmd)
{
    if( id == TesterThread::BORAD_ID ){
        Chunk chunk;
        unsigned char* data = (unsigned char *)cmd.data();
        if( data[0] == PC_TAG_CMD_SWITCH ){
            data[1] &= (~0x20);
            data[1] |= ((mLeftTester.RelayStatus | mRightTester.RelayStatus) & 0x20);
        }
        chunk.append(data,cmd.size());
        serial_send_packget(&mRightTester, chunk);
    }else if( id == TesterThread::SCANER_ID){
        scaner_send_cmd(&mRightTester,(unsigned char*)cmd.data(),cmd.size());
    }else{
        ui->consoleTextBrowser->append("Error: righttesterThread_sendSerialCmd: Unknow cmd send to ");
    }
}


void MainWindow::leftTesterThread_log(QString str)
{
    ui->autoTesterConsoletextBrowser->append("left:"+str);
    ui->statuslabel_left->setText(str);
}


void MainWindow::rightTesterThread_log(QString str)
{
    ui->autoTesterConsoletextBrowser->append("right:"+str);
    ui->statuslabel_right->setText(str);
}


void MainWindow::leftTesterThread_result(TesterRecord res)
{
    if( res.errorCode == TesterThread::ERRORCODE::E0 ){
        ui->ErrorcodepushButton->setText(" ");
        ui->testResultpushButton_2->setText(tr("测试通过"));
        ui->testResultpushButton_2->setStyleSheet("color: green");
    }else{
        ui->ErrorcodepushButton->setText("E"+QString::number(res.errorCode));//+res.errorCodeParameter);
        ui->testResultpushButton_2->setText(tr("测试失败"));
        ui->testResultpushButton_2->setStyleSheet("color: red");
    }
    if( !saveDataToFile(res) ){
        QMessageBox::warning(this,tr("错误"),tr("打开数据文件错误，尝试重启软件，再重新测试"));
    }

    ui->autoTesterConsoletextBrowser->append("Left Tester Result : E"+QString::number(res.errorCode));
    ui->autoTesterConsoletextBrowser->append(res.errorCodeString);
    ui->erroecodeStringlabel_24->setText( res.errorCodeString );

    ui->startTestpushButton->setText(tr("开始"));

    if( !mRightTester.TesterThread->mStartTest ){
        ui->airpresscheckBox->setChecked(false);
        ui->airpresscheckBox->clicked(false);
    }
}

void MainWindow::rightTesterThread_result(TesterRecord res)
{
    if( res.errorCode == TesterThread::ERRORCODE::E0 ){
        ui->ErrorcodepushButton_right->setText(" ");
        ui->testResultpushButton_right->setText(tr("测试通过"));
        ui->testResultpushButton_right->setStyleSheet("color: green");
    }else{
        ui->ErrorcodepushButton_right->setText("E"+QString::number(res.errorCode));//+res.errorCodeParameter);
        ui->testResultpushButton_right->setText(tr("测试失败"));
        ui->testResultpushButton_right->setStyleSheet("color: red");
    }

    if( res.QRcode.length() > 0 ){
        if( !saveDataToFile(res) ){
            QMessageBox::warning(this,tr("错误"),tr("保存数据错误，尝试重启软件，再重新测试"));
        }
    }

    ui->autoTesterConsoletextBrowser->append("Right Tester Result : E"+QString::number(res.errorCode));
    ui->erroecodeStringlabel_right->setText( res.errorCodeString );

    ui->startTestpushButton->setText(tr("开始"));

    if( !mLeftTester.TesterThread->mStartTest ){
        ui->airpresscheckBox->setChecked(false);
        ui->airpresscheckBox->clicked(false);
    }
}

void MainWindow::leftTesterThread_error(QString errorStr)
{
    ui->ErrorcodepushButton->setText(" ");
    ui->testResultpushButton_2->setText(tr("测试错误"));
    ui->testResultpushButton_2->setStyleSheet("color: red");
    ui->autoTesterConsoletextBrowser->append("Left Tester Error: "+errorStr);
    ui->erroecodeStringlabel_24->setText(errorStr);
    //QMessageBox::warning(this,tr("测试错误"),errorStr);
    ui->startTestpushButton->setText(tr("开始"));

    if( !mRightTester.TesterThread->mStartTest ){
        ui->airpresscheckBox->setChecked(false);
        ui->airpresscheckBox->clicked(false);
    }
}

void MainWindow::rightTesterThread_error(QString errorStr)
{
    ui->ErrorcodepushButton_right->setText(" ");
    ui->testResultpushButton_right->setText(tr("测试错误"));
    ui->testResultpushButton_right->setStyleSheet("color: red");
    ui->autoTesterConsoletextBrowser->append("Right Tester Error: "+errorStr);
    ui->autoTesterConsoletextBrowser->append(errorStr);
    //QMessageBox::warning(this,tr("测试错误"),errorStr);
    ui->erroecodeStringlabel_right->setText(errorStr);
    ui->startTestpushButton->setText(tr("开始"));

    if( !mLeftTester.TesterThread->mStartTest ){
        ui->airpresscheckBox->setChecked(false);
        ui->airpresscheckBox->clicked(false);
    }
}

void MainWindow::on_serialconnectPushButton_clicked()
{
    if( mLeftTester.Serialport != NULL || mLeftTester.Scanerserialport != NULL)
    {
        close_serial(&mLeftTester);
        ui->serialconnectPushButton->setText(tr("连接"));
        ui->testResultpushButton_2->setText(tr("通讯未连接"));
    }else{
        if( open_serial(&mLeftTester) ){
            ui->serialconnectPushButton->setText(tr("断开"));
            ui->testResultpushButton_2->setText(tr("待测"));
        }
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
        serial_send_PMSG( &mLeftTester, PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG( &mRightTester, PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        ui->consoleTextBrowser->append("normal mode");
    }
}

void MainWindow::on_TestMode2radioButton_2_toggled(bool checked)
{
    unsigned char data[1]={0xff};
    unsigned char data2[1]={2};
    if( checked ){
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCHES_TESTMODE,data2, sizeof(data2) );
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCHES_TESTMODE,data2, sizeof(data2) );
        ui->consoleTextBrowser->append("T2 mode");
    }
}

void MainWindow::on_TestMode3radioButton_3_toggled(bool checked)
{
    unsigned char data[1]={0xff};
    unsigned char data2[1]={3};
    if( checked ){
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCHES_TESTMODE,data2, sizeof(data2) );
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCHES_TESTMODE,data2, sizeof(data2) );
        ui->consoleTextBrowser->append("T3 mode");
    }
}

void MainWindow::on_Testmode4radioButton_4_toggled(bool checked)
{
    unsigned char data[1]={0xff};
    unsigned char data2[1]={4};
    if( checked ){
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCHES_TESTMODE,data2, sizeof(data2) );
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCHES_TESTMODE,data, sizeof(data) );
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCHES_TESTMODE,data2, sizeof(data2) );
        ui->consoleTextBrowser->append("T3 mode");
    }
}

void MainWindow::on_V15PoweroncheckBox_2_clicked(bool checked)
{
    unsigned char data;

    if( isTesterValiable(&mLeftTester) ){
        if( checked ){
            mLeftTester.RelayStatus |= 0x10;
        }else{
            mLeftTester.RelayStatus &= (~0x10);
        }
        data = mLeftTester.RelayStatus;
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Left Power "+QString(checked?"On":"Off"));
    }
    if( isTesterValiable(&mRightTester) ){
        if( checked ){
            mRightTester.RelayStatus |= 0x10;
        }else{
            mRightTester.RelayStatus &= (~0x10);
        }
        data = mRightTester.RelayStatus;
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Right Power "+QString(checked?"On":"Off"));
    }
}

void MainWindow::on_outputtoShavercheckBox_clicked(bool checked)
{
    unsigned char data;

    if( checked ){ //can't connect shaver and Rload at sametime
        ui->outputtoResistorcheckBox_3->setChecked(false);
        ui->outputtoResistorcheckBox_3->clicked(false);
    }

    if( isTesterValiable(&mLeftTester) ){
        if( checked ){
            mLeftTester.RelayStatus |= 0x01;
        }else{
            mLeftTester.RelayStatus &= (~0x01);
        }
        data = mLeftTester.RelayStatus;
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Left Shaver "+QString(checked?"On":"Off"));
    }
    if( isTesterValiable(&mRightTester) ){
        if( checked ){
            mRightTester.RelayStatus |= 0x01;
        }else{
            mRightTester.RelayStatus &= (~0x01);
        }
        data = mRightTester.RelayStatus;
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Right Shaver "+QString(checked?"On":"Off"));
    }
}

void MainWindow::on_outputtoResistorcheckBox_3_clicked(bool checked)
{
    unsigned char data;

    if( checked ){  //can't connect shaver and Rload at sametime
        ui->outputtoShavercheckBox->setChecked(false);
        ui->outputtoShavercheckBox->clicked(false);
    }

    if( isTesterValiable(&mLeftTester) ){
        if( checked ){
            mLeftTester.RelayStatus |= 0x02;
        }else{
            mLeftTester.RelayStatus &= (~0x02);
        }
        data = mLeftTester.RelayStatus;
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Left RLoad "+QString(checked?"On":"Off"));
    }
    if( isTesterValiable(&mRightTester) ){
        if( checked ){
            mRightTester.RelayStatus |= 0x02;
        }else{
            mRightTester.RelayStatus &= (~0x02);
        }
        data = mRightTester.RelayStatus;
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Right Rload "+QString(checked?"On":"Off"));
    }

}

void MainWindow::on_outputtoVmetercheckBox_4_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        ui->measuretoVmetercheckBox_5->setChecked(false);
        ui->measuretoVmetercheckBox_5->clicked(false);
    }
    if( isTesterValiable(&mLeftTester) ){
        if( checked ){
            mLeftTester.RelayStatus |= 0x08;
        }else{
            mLeftTester.RelayStatus &= (~0x08);
        }
        data = mLeftTester.RelayStatus;
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Left Vout->Vmeter "+QString(checked?"On":"Off"));
    }
    if( isTesterValiable(&mRightTester) ){
        if( checked ){
            mRightTester.RelayStatus |= 0x08;
        }else{
            mRightTester.RelayStatus &= (~0x08);
        }
        data = mRightTester.RelayStatus;
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Right Vout->Vmeter "+QString(checked?"On":"Off"));
    }
}

void MainWindow::on_airpresscheckBox_clicked(bool checked)
{
    unsigned char data;

    if( isTesterValiable(&mLeftTester) ){
        if( checked ){
            mLeftTester.RelayStatus |= 0x20;
        }else{
            mLeftTester.RelayStatus &= (~0x20);
        }
        data = mLeftTester.RelayStatus;
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Left Air Cylinder "+QString(checked?"On":"Off"));
    }
    if( isTesterValiable(&mRightTester) ){
        if( checked ){
            mRightTester.RelayStatus |= 0x20;
        }else{
            mRightTester.RelayStatus &= (~0x20);
        }
        data = mRightTester.RelayStatus;
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Right Air Cylinder "+QString(checked?"On":"Off"));
    }
}

void MainWindow::on_measuretoVmetercheckBox_5_clicked(bool checked)
{
    unsigned char data;
    if( checked ){
        ui->outputtoVmetercheckBox_4->setChecked(false);
        ui->outputtoVmetercheckBox_4->clicked(false);
    }

    if( isTesterValiable(&mLeftTester) ){
        if( checked ){
            mLeftTester.RelayStatus |= 0x04;
        }else{
            mLeftTester.RelayStatus &= (~0x04);
        }
        data = mLeftTester.RelayStatus;
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Left Vpcb->Vmeter "+QString(checked?"On":"Off"));
    }

    if( isTesterValiable(&mRightTester) ){
        if( checked ){
            mRightTester.RelayStatus |= 0x04;
        }else{
            mRightTester.RelayStatus &= (~0x04);
        }
        data = mRightTester.RelayStatus;
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCH, &data, 1);
        ui->consoleTextBrowser->append("Right Vpcb->Vmeter "+QString(checked?"On":"Off"));
    }
}


void MainWindow::on_VmeterStartcheckBox_clicked(bool checked)
{
    unsigned char data;

    data = checked?0x01:0x00;

    if( isTesterValiable(&mLeftTester) ){
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_VMETER_READ, &data, 1);
        ui->consoleTextBrowser->append("Left Vmeter Read "+QString(checked?"On":"Off"));
    }

    if( isTesterValiable(&mRightTester) ){
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_VMETER_READ, &data, 1);
        ui->consoleTextBrowser->append("Right Vmeter Read "+QString(checked?"On":"Off"));
    }
}


void MainWindow::on_meausureVDDcheckBox_6_clicked(bool checked)
{
    unsigned char data;

    data = checked?0x0C:0x0F;

    if( checked ){
        ui->measureLEDcheckBox_7->clicked(false);
        ui->measureLEDcheckBox_7->setCheckState(Qt::CheckState::Unchecked);
    }

    if( isTesterValiable(&mLeftTester) ){
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCHES_CHANNEL, &data, 1);
        ui->consoleTextBrowser->append("Left Meausre VDD "+QString(checked?"On":"Off"));
    }

    if( isTesterValiable(&mRightTester) ){
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCHES_CHANNEL, &data, 1);
        ui->consoleTextBrowser->append("Right Meausre VDD "+QString(checked?"On":"Off"));
    }
}

void MainWindow::on_measureLEDcheckBox_7_clicked(bool checked)
{
    unsigned char data;

    data = checked?0x03:0x0F;

    if( checked ){
        ui->meausureVDDcheckBox_6->clicked(false);
        ui->meausureVDDcheckBox_6->setCheckState(Qt::CheckState::Unchecked);
    }

    if( isTesterValiable(&mLeftTester) ){
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_SWITCHES_CHANNEL, &data, 1);
        ui->consoleTextBrowser->append("Left Meausre VLED "+QString(checked?"On":"Off"));
    }

    if( isTesterValiable(&mRightTester) ){
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_SWITCHES_CHANNEL, &data, 1);
        ui->consoleTextBrowser->append("Right Meausre VLED "+QString(checked?"On":"Off"));
    }
}

void MainWindow::on_LedCapturecheckBox_3_clicked(bool checked)
{
    unsigned char data;

    data = checked?0x01:0x00;

    if( isTesterValiable(&mLeftTester) ){
        serial_send_PMSG(&mLeftTester,PC_TAG_CMD_CAPTURE_EN, &data, 1);
        ui->consoleTextBrowser->append("Left LED capture "+QString(checked?"On":"Off"));
    }

    if( isTesterValiable(&mRightTester) ){
        serial_send_PMSG(&mRightTester,PC_TAG_CMD_CAPTURE_EN, &data, 1);
        ui->consoleTextBrowser->append("Right LED capture "+QString(checked?"On":"Off"));
    }
}

void MainWindow::on_measuretoVmetercheckBox_5_clicked()
{
//no use
}

void MainWindow::on_scanerStartpushButton_clicked()
{
    unsigned char cmd[3] = {0x16,0x54,0x0D};

    if( isTesterValiable(&mLeftTester) ){
        scaner_send_cmd(&mLeftTester,cmd,sizeof(cmd));
    }

    if( isTesterValiable(&mRightTester) ){
        scaner_send_cmd(&mRightTester,cmd,sizeof(cmd));
    }
}



void MainWindow::on_DebugpushButton_clicked()
{
    if( isTesterValiable(&mLeftTester) ){
        emit leftDebug_step(true);
    }

    if( isTesterValiable(&mRightTester) ){
        emit rightDebug_step(true);
    }
}

void MainWindow::on_startTestpushButton_clicked()
{
    bool toStart = false;

    if( mLeftTester.TesterThread->mStartTest || mRightTester.TesterThread->mStartTest){
        toStart = false;
    }else{
        toStart = true;
    }


    if( isTesterValiable(&mLeftTester) ){
        if( toStart ){
            ui->airpresscheckBox->setChecked(true);
            ui->airpresscheckBox->clicked(true);
            ui->ErrorcodepushButton->setText(" ");
            ui->testResultpushButton_2->setText(tr("测试中..."));
            ui->testResultpushButton_2->setStyleSheet("color: black");
            ui->startTestpushButton->setText(tr("中止"));
        }else{
            ui->airpresscheckBox->setChecked(false);
            ui->airpresscheckBox->clicked(false);
            ui->ErrorcodepushButton->setText(" ");
            ui->testResultpushButton_2->setText(tr("中止测试"));
            ui->testResultpushButton_2->setStyleSheet("color: black");
            ui->startTestpushButton->setText(tr("开始"));
            ui->statuslabel_left->setText("");
        }
        emit leftTestThread_start(toStart);
    }

    if( isTesterValiable(&mRightTester) ){
        if( toStart ){
            ui->airpresscheckBox->setChecked(true);
            ui->airpresscheckBox->clicked(true);
            ui->ErrorcodepushButton_right->setText(" ");
            ui->testResultpushButton_right->setText(tr("测试中..."));
            ui->testResultpushButton_right->setStyleSheet("color: black");
            ui->startTestpushButton->setText(tr("中止"));
        }else{
            ui->airpresscheckBox->setChecked(false);
            ui->airpresscheckBox->clicked(false);
            ui->ErrorcodepushButton_right->setText(" ");
            ui->testResultpushButton_right->setText(tr("中止测试"));
            ui->testResultpushButton_right->setStyleSheet("color: black");
            ui->startTestpushButton->setText(tr("开始"));
            ui->statuslabel_right->setText("");
        }
        emit rightTestThread_start(toStart);
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
    if( isTesterValiable(&mLeftTester) ){
        mLeftTester.TesterThread->mDebugMode = checked;
    }

    if( isTesterValiable(&mRightTester) ){
        mRightTester.TesterThread->mDebugMode = checked;
    }
}

void MainWindow::on_serialrescanPushButton_right_clicked()
{
    update_serial_info();
}

void MainWindow::on_serialconnectPushButton_right_clicked()
{
    if( mRightTester.Serialport != NULL || mRightTester.Scanerserialport != NULL)
    {
        close_serial(&mRightTester);
        ui->serialconnectPushButton_right->setText(tr("连接"));
        ui->testResultpushButton_right->setText(tr("通讯未连接"));
    }else{
        if( open_serial(&mRightTester) ){
            ui->serialconnectPushButton_right->setText(tr("断开"));
            ui->testResultpushButton_right->setText(tr("待测"));
        }
    }
}
