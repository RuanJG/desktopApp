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
    mTimer(),
    currentPlot(),
    noisePlot(),
    mExcelTestIndex(0),
    mExcelTestCount(0),
    mExcelTitleRow(0),
    mtestTimer(),
    mNoiseFalseCount(0),
    mCurrentFalseCount(0),
    mFalseCount(0)
{
    ui->setupUi(this);
    ui->currentMaxLineEdit->setText(tr("500"));
    ui->currentMinLineEdit->setText(tr("400"));
    ui->VolumeMaxlineEdit->setText(tr("65"));
    ui->VolumeMinlineEdit->setText(tr("20"));

    update_serial_info();

    on_checkBox_clicked();

    saveRecordToExcel(0,0,0,USER_RES_ERROR_FLAG);
    ui->excelFilePathlineEdit->setText( mExcel.getFilePath() );

    initIap();


    currentPlot.setup(ui->currentPlotwidget, \
                      QCPRange(ui->currentMinLineEdit->text().toFloat(), ui->currentMaxLineEdit->text().toFloat()),\
                      QCPRange(0,800),"Current(mA)");

    noisePlot.setup(ui->noisePlotwidget, \
                      QCPRange(ui->VolumeMinlineEdit->text().toFloat(), ui->VolumeMaxlineEdit->text().toFloat()),\
                      QCPRange(0,100),"Noise(DB)");


    for( int i=1; i< 10; i++ ){
        ui->testCountcomboBox->addItem( QString::number(i) );
    }
    ui->testCountcomboBox->setCurrentIndex(0);

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
        ui->SerialButton->setText(tr("Connect 连接"));
    }else{
        if( open_serial() ){
            ui->SerialButton->setText(tr("Disconnect 断开"));

            //to get the config
            Chunk chunk;
            chunk.append( USER_CMD_GET_MAXMIN_TAG );
            serial_send_packget( chunk );
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

    //qDebug() << "receive: " << bytes.count() << endl;

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

        int db,count,error;
        float current;
        memcpy((char*)&db, data+1, 4);
        memcpy( (char*)&current , data+5 , 4);
        count = data[9];
        error = data[10];

        current *= 1000; // A->mA

        if( error ==0 && count == 0 ){
            // this packget is not a result , is the test data in each record
            currentPlot.append(current);
            noisePlot.append((float)db);
            return;
        }

        saveRecordToExcel(db, current, count, error );
        displayResult(db, current , error);


        ui->textBrowser->append("db="+QString::number(db) +"db, current="+QString::number(current)+"mA, count=" + QString::number(count) +
                                ", error="+ QString::number(error) );

        break;
    }


    case USER_CONFIG_TAG:{
        //tag[0]+config_valide[1]+currentmax(A) + currentmin(A) + dbmax + dbmin
        if( len != 18 )
            return;

        int dbmin,dbmax;
        float cmax,cmin;

        if( data[1] != 1){
            ui->textBrowser->append("get an error config");
            break;
        }
        memcpy((char*)&cmax, data+2, 4);
        memcpy((char*)&cmin , data+6 , 4);
        memcpy((char*)&dbmax, data+10, 4);
        memcpy((char*)&dbmin , data+14 , 4);

        ui->currentMaxLineEdit->setText( QString::number(cmax*1000));
        ui->currentMinLineEdit->setText( QString::number(cmin*1000));
        ui->VolumeMaxlineEdit->setText( QString::number(dbmax) );
        ui->VolumeMinlineEdit->setText( QString::number(dbmin) );

        currentPlot.changeRange(cmin*1000,cmax*1000);
        noisePlot.changeRange( dbmin, dbmax);

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

        currentPlot.clear();
        noisePlot.clear();

        break;
    }

    default:{
        ui->textBrowser->append("unknow message!!!");
        break;
    }

    }
}


void MainWindow::displayResult( int db, float current, int error )
{

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
}





void MainWindow::saveRecordToExcel(int db, float current, int count, int error)
{
    bool newsheet = false;
    int noiseFalse=0;
    int currentFalse = 0;


    if( ! mExcel.IsValid() || ! mExcel.IsOpen() ){

        QString filename = "T-REX-Report_"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")+".xlsx";
        QString filedirPath = QDir::currentPath()+"/reports";
        QDir mdir;
        mdir.mkpath(filedirPath);
        filename =QDir::toNativeSeparators( filedirPath+"/"+filename );

        //use the select file or auto make file
        if( ui->excelFilePathlineEdit->text().length()> 0 ){
            filename = ui->excelFilePathlineEdit->text();
            if(!mExcel.Open(filename,1,true)){
                QMessageBox::warning(this,"Warning",tr("打开Excel文档失败"));
                return;
            }
            //qDebug() <<"Sheet count=" + QString::number(mExcel.getSheetCount()) << endl;
            //qDebug() <<"row count=" + QString::number(mExcel.GetUsedRagneRowCount()) << endl;
            //qDebug() <<"col count=" + QString::number(mExcel.GetUsedRagneColumnCount()) << endl;
            //qDebug() <<"start row count=" + QString::number(mExcel.getUsedRagneStartRow()) << endl;
            //qDebug() <<"start col count=" + QString::number(mExcel.getUsedRagneStartColumn()) << endl;
            int usedRow = mExcel.GetUsedRagneRowCount();
            int usedCol = mExcel.GetUsedRagneColumnCount();
            if( usedCol == 11 && usedRow > 1 ){
                newsheet = false;
                mExcelTitleRow = 1;
                mExcelCurrentRow = usedRow+1;
            }else{
                newsheet = true;
                mExcelTitleRow = usedRow+2;
                mExcelCurrentRow = mExcelTitleRow+1;
            }
            mExcelTestIndex = newsheet? 1:1+mExcel.GetCellData(usedRow,1).toInt() ;
            mExcelTestCount = 0;
            mNoiseFalseCount =newsheet? 0: mExcel.GetCellData(mExcelTitleRow+1,10).toInt();
            mCurrentFalseCount =newsheet? 0: mExcel.GetCellData(mExcelTitleRow+1,11).toInt();;
            mFalseCount =newsheet? 0: mExcel.GetCellData(mExcelTitleRow+1,9).toInt(); ;

        }else{
            if(!mdir.mkpath(filedirPath) || !mExcel.Open(filename,1,true) ){
                QMessageBox::warning(this,"Warning",tr("创建Excel文档失败"));
                return;
            }
            newsheet = true;
            mExcelCurrentRow = 2;
            mExcelTitleRow = 1;
            mExcelTestIndex = 1;
            mExcelTestCount = 0;
            mNoiseFalseCount =0;
            mCurrentFalseCount =0;
            mFalseCount =0 ;
        }
    }

    if( mExcelCurrentRow >= mExcel.getMaxRowCount() ){
        //不做续页处理
        QMessageBox::warning(this,"Warning",tr("Excel文档己经填满，请保存数据后重新开始程序"));
        return;
        //续页处理
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
        //不做续页处理
        //mExcelCurrentRow = 1 ; //mExcel.getStartRow();

        int col = 1;
        if( mExcel.SetCellData(mExcelTitleRow, col++, "Index") \
                && mExcel.SetCellData(mExcelTitleRow, col++, "Shell") \
                && mExcel.SetCellData(mExcelTitleRow, col++, "Color") \
                && mExcel.SetCellData(mExcelTitleRow, col++, "Noise(DB)") \
                && mExcel.SetCellData(mExcelTitleRow, col++, "Current(mA)") \
                //&& mExcel.SetCellData(mExcelCurrentRow, col++, "测量次数")
                && mExcel.SetCellData(mExcelTitleRow, col++, "Result"))
        {
            //set 统计结果
            col++;
            mExcel.SetCellData(mExcelTitleRow, col++, "Total Count");
            mExcel.SetCellData(mExcelTitleRow, col++, "False Count");
            mExcel.SetCellData(mExcelTitleRow, col++, "Noise False Count");
            mExcel.SetCellData(mExcelTitleRow, col++, "Current False Count");
        }else{
            QMessageBox::warning(this,"Warning",tr("Excel文档写入标题失败"));
            return;
        }
    }


    //不统计通信失败的测试，如果要加入，删除以下判断
    if( error & USER_RES_ERROR_FLAG )
        return;

    //插入到excel表
    QString res = "unknow";
    if( error == 0 ){
        res = tr("Pass");
    }else if( error & USER_RES_ERROR_FLAG ){
        res = tr("Error: 测量无效,通信失败");
    }else{
        res = tr("False: ");
        if( (error & USER_RES_CURRENT_FALSE_FLAG) != 0 ){
            float min = ui->currentMinLineEdit->text().toFloat();
            float max = ui->currentMaxLineEdit->text().toFloat();
            if( current >= max ){
                res += tr("Current Hight,");
            }
            if( current <= min ){
                res += tr("Current Low,");
            }
            currentFalse = 1;
        }
        if( (error & USER_RES_VOICE_FALSE_FLAG) != 0 ){
            res += tr("Noise Hight");
            noiseFalse = 1;
        }

    }

    int col = 1;
    if( mExcelTestCount == 0 ){
        mExcel.SetCellData(mExcelCurrentRow, col++, mExcelTestIndex );
        mExcel.SetCellData(mExcelCurrentRow, col++, ui->withSheelCheckBox->isChecked()? "Yes":"No" );
    }else{
        col++;
        mExcel.SetCellData(mExcelCurrentRow, col++, ui->withSheelCheckBox->isChecked()? "No":"Yes" );
    }

    if( ui->checkBox->isChecked()){
        mExcel.SetCellData(mExcelCurrentRow, col++, ui->colorBlackradioButton->isChecked()?"Black":"White" );
    }else{
        mExcel.SetCellData(mExcelCurrentRow, col++, "-" );
    }


    if(  mExcel.SetCellData(mExcelCurrentRow, col++, db) \
            && mExcel.SetCellData(mExcelCurrentRow, col++, QString::number(current) ) \
            //&& mExcel.SetCellData(mExcelCurrentRow, col++, count)
            && mExcel.SetCellData(mExcelCurrentRow, col, res))
    {
        if( error == 0){
            mExcel.setCellBackgroundColor(mExcelCurrentRow,col,QColor(0,255,0));
        }else{
            mExcel.setCellBackgroundColor(mExcelCurrentRow,col,QColor(255,0,0));
        }

        col++;
        if( mExcelTestCount >= (ui->testCountcomboBox->currentIndex()) ){
            if( mExcelTestCount >= 1 )
                mExcel.mergeUnit(mExcelCurrentRow-mExcelTestCount-1,QChar('A'), mExcelCurrentRow-1,QChar('A'));
            mExcelTestCount = 0;

            mNoiseFalseCount += noiseFalse;
            mCurrentFalseCount += currentFalse;
            mFalseCount += (noiseFalse+currentFalse > 0) ? 1:0 ;
            mExcel.SetCellData(mExcelTitleRow+1, col+1, QString::number(mExcelTestIndex) );
            mExcel.SetCellData(mExcelTitleRow+1, col+2, QString::number(mFalseCount) );
            mExcel.SetCellData(mExcelTitleRow+1, col+3, QString::number(mNoiseFalseCount) );
            mExcel.SetCellData(mExcelTitleRow+1, col+4, QString::number(mCurrentFalseCount) );

            if( mExcelTestIndex > 3 ){
                mExcel.SetCellData(mExcelTitleRow+2, col+2, QString::number(mFalseCount*100/mExcelTestIndex)+"%" );
                mExcel.SetCellData(mExcelTitleRow+2, col+3, QString::number(mNoiseFalseCount*100/mExcelTestIndex)+"%"  );
                mExcel.SetCellData(mExcelTitleRow+2, col+4, QString::number(mCurrentFalseCount*100/mExcelTestIndex)+"%"  );
            }
            mExcelTestIndex++;

        }else{
            mExcelTestCount++;
        }


        mExcelCurrentRow ++;
        mExcel.Save();

    }else{
        QMessageBox::warning(this,"Warning",tr("保存数据失败"));
    }


}








void MainWindow::on_setcurrentButton_clicked()
{
    float max = ui->currentMaxLineEdit->text().toFloat();
    float min = ui->currentMinLineEdit->text().toFloat();

    // mA -> A
    max /= 1000;
    min /= 1000;
    if( max > min){

        //tag+max+min
        //send to device
//        float Amax,Amin;
//        Amax = (float)max/1000;
//        Amin = (float)min/1000;
        Chunk chunk;
        chunk.append( USER_CMD_CURRENT_MAXMIN_TAG );
        chunk.append( (const unsigned char *)&max, 4);
        chunk.append( (const unsigned char *)&min, 4);
        serial_send_packget( chunk );
        currentPlot.changeRange(min,max);
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
        noisePlot.changeRange(min,max);
    }
}

void MainWindow::on_ClearTextBrowButton_clicked()
{
    ui->textBrowser->clear();

//    saveRecordToExcel(70,0.41,9,2);
//    displayResult(60,0.55,0);
//    currentPlot.append(0.3+(float)(rand()%3)/10 );
//    noisePlot.append( 55.0+ (float)(rand()%30));

//    startTestVictor();
}

void MainWindow::on_restartButton_clicked()
{
    if( mExcel.IsValid())
        mExcel.Close();

    ui->excelFilePathlineEdit->setText("");
    saveRecordToExcel(0,0,0,USER_RES_ERROR_FLAG);
    ui->excelFilePathlineEdit->setText( mExcel.getFilePath() );

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








//################################# color setting


void MainWindow::on_checkBox_clicked()
{
    if( ui->checkBox->isChecked() ){
        ui->colorWhiteradioButton->setEnabled(true);
        ui->colorBlackradioButton->setEnabled(true);
    }else{
        ui->colorBlackradioButton->setEnabled(false);
        ui->colorWhiteradioButton->setEnabled(false);
    }
}

void MainWindow::on_testCountcomboBox_currentIndexChanged(const QString &arg1)
{

}





bool MainWindow::sendVictorCmd( char* cmd, int timeoutMs, char * res , int len)
{
    int index = 0,count;


    mSerialport->write(cmd, strlen(cmd));

    if( timeoutMs == 0 || res == NULL || len == 0)
        return true;

    memset(res,0,len);

    while( mSerialport->waitForReadyRead( timeoutMs) ){
        count = mSerialport->read(res+index,len-index);
        if( count <= 0 ){
            return false;
        }
        index += count;
        if( res[index-1] == '\n' || index >= len){
            return true;
        }
    }

    return false;
}

void MainWindow::victorEvent()
{
    char data[32];

    if( sendVictorCmd("FUNC1?\n",300,data,32) ){
        if( 0 != strcmp(data,"ADC\n") ){
            ui->textBrowser->append("func init!");
            sendVictorCmd("ADC\n",300,data,32);

            sendVictorCmd("RATE M\n",300,data,32);

            sendVictorCmd("RANGE 5\n",300,data,32);
            return;
        }
    }else{
        ui->textBrowser->append("func timeout!");
    }

    if( sendVictorCmd("MEAS1?\n",300,data,32) ){
        ;//ui->textBrowser->append(QString::fromLocal8Bit(data));
    }else{
        ui->textBrowser->append("timeout!");
    }

}

void MainWindow::startTestVictor()
{
    char data[32];

    disconnect( mSerialport,SIGNAL(readyRead()),this, SLOT(solt_mSerial_ReadReady()) );

    sendVictorCmd("ADC\n",300,data,32);

    sendVictorCmd("RATE M\n",300,data,32);

    sendVictorCmd("RANGE 5\n",300,data,32);



    connect( &mtestTimer, SIGNAL(timeout()), SLOT(victorEvent()) );


    mtestTimer.start(10);
    //connect( mSerialport,SIGNAL(readyRead()),this, SLOT(solt_mSerial_ReadReady()) );
}













void MainWindow::on_excelFileSelectpushButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Excel File"), "", tr("Excel Files (*.xlsx)"));

    if( fileName.length() > 0 ){

        if( mExcel.IsValid())
            mExcel.Close();

        ui->excelFilePathlineEdit->setText(fileName);
        saveRecordToExcel(0,0,0,USER_RES_ERROR_FLAG);
    }


}















