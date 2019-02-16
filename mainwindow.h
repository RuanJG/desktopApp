#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QtSerialPort>
#include <QMutex>
#include <uartcoder.h>
#include <chunk.h>
#include "excelengine.h"
#include <iostream>
#include "iapmaster.h"
#include <QTimer>
#include "qcustomplot.h"
#include <QTcpSocket>
#include "serialcoder.h"
#include <QSettings>

// packget  head tag
#define USER_DATA_TAG	1
#define USER_LOG_TAG	2
#define USER_START_TAG  3
#define USER_CONFIG_TAG	4
#define USER_CMD_CHMOD_TAG  4
#define USER_CMD_CURRENT_MAXMIN_TAG 5
#define USER_CMD_VOICE_MAXMIN_TAG   6
#define USER_CMD_GET_MAXMIN_TAG 7
#define USER_ACK_TAG 8
#define USER_CMD_SET_SW_STATUS_TAG 9
#define USER_CMD_SET_MACHINE_NO 10

//packget body : result error value
#define USER_RES_CURRENT_FALSE_FLAG 1
#define USER_RES_VOICE_FALSE_FLAG 2
#define USER_RES_ERROR_FLAG 4

//work_mode value
#define USER_MODE_ONLINE 1
#define USER_MODE_OFFLINE 2
#define USER_MODE_SLAVER 3

//if use execl
#define USE_EXECL FALSE
#define USE_TXT (!USE_EXECL)

namespace Ui {
class MainWindow;
}



#include <QFont>

#define MAX_X_COUNT 50
class ResPlot
{
private:
    QCPBars *hightBar;
    QCPBars *middleBar;
    QCPBars *lowBar;
    QVector<double> hightBarData, middleBarData, lowBarData, mticks;
    QCustomPlot *customPlot;
    QCPRange mValidRange;

public:
    ResPlot(){
        customPlot = NULL;
    }

    ~ResPlot(){

    }

    void setup(QCustomPlot *cplot, QCPRange maxminRange, QCPRange yrange, QString yLable){
        customPlot = cplot;
        mValidRange = maxminRange;

        for ( int i=1; i<= MAX_X_COUNT ; i++ ){
            mticks << i;
        }

        hightBar = new QCPBars(customPlot->xAxis, customPlot->yAxis);
        middleBar = new QCPBars(customPlot->xAxis, customPlot->yAxis);
        lowBar = new QCPBars(customPlot->xAxis, customPlot->yAxis);
        // gives more crisp, pixel aligned bar borders
        hightBar->setAntialiased(false);
        middleBar->setAntialiased(false);
        lowBar->setAntialiased(false);
        lowBar->setStackingGap(1);
        middleBar->setStackingGap(1);
        hightBar->setStackingGap(1);
        // set names and colors:
        middleBar->setName("normal");
        middleBar->setPen(QPen(QColor(9, 111, 176).lighter(170)));
        middleBar->setBrush(QColor(9, 111, 176));
        hightBar->setName("hightPart");
        hightBar->setPen(QPen(QColor("RED").lighter(150)));
        hightBar->setBrush(QColor("RED"));
        lowBar->setName("lowPart");
        lowBar->setPen(QPen(QColor(250, 170, 20).lighter(130)));
        lowBar->setBrush(QColor(250, 170, 20));
        // stack bars on top of each other:
        hightBar->moveAbove(middleBar);
        middleBar->moveAbove(lowBar);



        // add init
        /*
        for ( int i=1; i<= MAX_X_COUNT ; i++ ){
            middleBarData << 0;
            hightBarData << 0;
            lowBarData << 0;
        }

        QVector<double> ticks;
        for( int i=0; i< hightBarData.size(); i++){
            ticks<< i+1;
        }

        hightBar->setData(ticks,hightBarData);
        middleBar->setData(ticks, middleBarData);
        lowBar->setData(ticks,lowBarData);


        hightBar->moveAbove(middleBar);
        middleBar->moveAbove(lowBar);
*/






        // set dark background gradient:
        QLinearGradient gradient(0, 0, 0, 400);
        gradient.setColorAt(0, QColor(90, 90, 90));
        gradient.setColorAt(0.38, QColor(105, 105, 105));
        gradient.setColorAt(1, QColor(70, 70, 70));
        customPlot->setBackground(QBrush(gradient));


        // prepare x axis with country labels:
        QVector<QString> labels;
        //mticks << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 <<10 << 11;
        //labels << "1" << "2" << "3" << "4" << "5" << "6" << "7" <<"8" << "9" << "10" <<"11";
        for ( int i=1; i<= MAX_X_COUNT ; i++ ){
            labels << QString::number(i);
        }
        QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
        textTicker->addTicks(mticks, labels);
        customPlot->xAxis->setTicker(textTicker);
        customPlot->xAxis->setTickLabelRotation(60);
        customPlot->xAxis->setSubTicks(false);
        customPlot->xAxis->setTickLength(0, 3);
        customPlot->xAxis->setRange(0, MAX_X_COUNT);
        customPlot->xAxis->setBasePen(QPen(Qt::white));
        customPlot->xAxis->setTickPen(QPen(Qt::white));
        customPlot->xAxis->grid()->setVisible(true);
        customPlot->xAxis->grid()->setPen(QPen(QColor(130, 130, 130), 0, Qt::DotLine));
        customPlot->xAxis->setTickLabelColor(Qt::white);
        customPlot->xAxis->setLabelColor(Qt::white);

        // prepare y axis:
        customPlot->yAxis->setRange(yrange);
        customPlot->yAxis->setPadding(5); // a bit more space to the left border
        customPlot->yAxis->setLabel(yLable);
        customPlot->yAxis->setBasePen(QPen(Qt::white));
        customPlot->yAxis->setTickPen(QPen(Qt::white));
        customPlot->yAxis->setSubTickPen(QPen(Qt::white));
        customPlot->yAxis->grid()->setSubGridVisible(true);
        customPlot->yAxis->setTickLabelColor(Qt::white);
        customPlot->yAxis->setLabelColor(Qt::white);
        customPlot->yAxis->grid()->setPen(QPen(QColor(130, 130, 130), 0, Qt::SolidLine));
        customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(130, 130, 130), 0, Qt::DotLine));

        // setup legend:
        customPlot->legend->setVisible(true);
        customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignRight);
        customPlot->legend->setBrush(QColor(255, 255, 255, 100));
        customPlot->legend->setBorderPen(Qt::NoPen);
        QFont legendFont = customPlot->font();
        legendFont.setPointSize(10);
        customPlot->legend->setFont(legendFont);
        customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

        //draw
        customPlot->replot();

    }




    void append( float data){
        //return;
        //qDebug() << mValidRange.lower << "," << mValidRange.upper << ":" << data << endl;
        if( data > mValidRange.upper ){
            middleBarData << mValidRange.upper;
            hightBarData << data-mValidRange.upper;
            lowBarData << 0;
        }else if( data < mValidRange.lower ){
            middleBarData << 0;
            hightBarData << 0;
            lowBarData << data;
        }else{
            middleBarData << data;
            hightBarData << 0;
            lowBarData << 0;
        }

        QVector<double> ticks;
        for( int i=0; i< hightBarData.size(); i++){
            ticks<< i+1;
        }

        hightBar->setData(ticks,hightBarData,true);
        middleBar->setData(ticks, middleBarData,true);
        lowBar->setData(ticks,lowBarData,true);


        hightBar->moveAbove(middleBar);
        middleBar->moveAbove(lowBar);

        customPlot->replot();
    }

    void clear( ){
        //return;
        QVector<double> ticks;
        middleBarData.clear();
        hightBarData.clear();
        lowBarData.clear();
        hightBar->setData(ticks,hightBarData);
        middleBar->setData(ticks, middleBarData);
        lowBar->setData(ticks,lowBarData);
        customPlot->replot();
    }

    void changeRange( float min, float max){
        mValidRange = QCPRange( min,max);
    }


};


class TestData{

    typedef struct _testData_t {
        int ms;
        int db;
        float currentmA;
    }testData_t;

    testData_t mTestData[100];
    int mIndex ;

public:
    TestData(){
        clear();
    }

    bool add( int ms, int db, float currentmA){
        if( mIndex >= 100){
            return false;
        }
        mTestData[mIndex].ms = ms;
        mTestData[mIndex].db = db;
        mTestData[mIndex].currentmA = currentmA;
        mIndex++;
        return true;
    }
    bool set( int index, int ms, int db, float currentmA){
        if( index >= 100){
            return false;
        }
        mTestData[index].ms = ms;
        mTestData[index].db = db;
        mTestData[index].currentmA = currentmA;
        return true;
    }
    void clear(){
        mIndex = 0;
    }
    int count()
    {
        return mIndex;
    }

    int findItemAfterMs( int ms )
    {
        for( int i=0; i< mIndex; i++){
            if( mTestData[i].ms >= ms )
                return i;
        }
        return -1;
    }
    int findItemBeforeMs( int ms )
    {
        for( int i= (mIndex-1); i >= 0; i--){
            if( mTestData[i].ms <= ms )
                return i;
        }
        return -1;
    }
    int getAverageDB( int minId, int maxId )
    {
        int db =0,min,max;
        min = mTestData[minId].db;
        max = mTestData[minId].db;
        for( int i= minId; i <= maxId; i++){
            db += mTestData[i].db;
            if( min > mTestData[i].db) min = mTestData[i].db;
            if( max < mTestData[i].db) max = mTestData[i].db;
        }
        int count = maxId - minId +1;
        if( count > 2){
            return (db-min-max)/(count-2);
        }else{
            return db/count;
        }
    }
    float getAverageCurrent( int minId, int maxId )
    {
        float current=0,min,max;
        min = mTestData[minId].currentmA;
        max = mTestData[minId].currentmA;
        for( int i= minId; i <= maxId; i++){
            current += mTestData[i].currentmA;
            if( min > mTestData[i].currentmA) min = mTestData[i].currentmA;
            if( max < mTestData[i].currentmA) max = mTestData[i].currentmA;
        }
        int count = maxId - minId +1;
        if( count > 2){
            return (current-min-max)/(count-2);
        }else{
            return current/count;
        }
    }
};


class MainWindow : public QMainWindow,IapMaster
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);

    void testSendStartPackget();
    void testSendDataPackget(int db, float current, int count, int error);
    //void saveRecordToTXT(int db, float current, int count, int error);
    QString ByteArrayToHexString(QByteArray data);
    void startSlaveMode(int mode);
    void setUserMode(int mode);
    void setSWMode(int mode);
    void saveRecordToTXT(int db, float current, int count, int error, int db3, float current3, int count3, int error3);
private slots:
    void on_save_close_Button_clicked();

    void on_pushButton_3_clicked();

    void on_SerialButton_clicked();

    void solt_mSerial_ReadReady();

    void on_setcurrentButton_clicked();

    void on_setVolumeButton_clicked();

    void on_ClearTextBrowButton_clicked();

    void on_restartButton_clicked();

    void iapTickHandler();
    void on_iapButton_clicked();

    void on_fileChooseButton_clicked();

    void on_checkBox_clicked();

    void on_testCountcomboBox_currentIndexChanged(const QString &arg1);

    void victorEvent();

    void on_excelFileSelectpushButton_clicked();

    void dataTestEvent();

    void on_setMachineNoButton_clicked();

    void on_TcpConnectpushButton_clicked();

    void tcpReceivedData();
    tcpSocketState(QAbstractSocket::SocketState socketState);
    void on_checkBox_2_stateChanged(int arg1);

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_2_clicked();

    void on_lineEdit_textChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
    QSerialPort *mSerialport;
    QMutex mSerialMutex;
    UartCoder mDecoder;
    UartCoder mEncoder;
    ExcelEngine mExcel; //创建excl对象
    unsigned int mExcelCurrentRow;
    QTimer  mTimer;
    ResPlot currentPlot;
    ResPlot noisePlot;
    int mExcelTestIndex;
    int mExcelTestCount;
    int mExcelTitleRow;

    QTimer  mtestTimer;
    unsigned int mNoiseFalseCount;
    unsigned int mCurrentFalseCount;
    unsigned int mFalseCount;

    int dataTestCounter;
    QFile mTxtfile;
    QString mLogstr;
    int mSwMode;
    int mUsermode;
    QString mMachineName;
    QTcpSocket *mSocket;
    volatile bool isTcpConnected;
    SerialCoder mSerialCoder;
    bool useNewCoder;
    QSettings mSetting;
    TestData mTestData;



    void update_serial_info();
    void close_serial();
    bool open_serial();
    bool open_serial_dev(QString portname);
    void handle_Serial_Data(QByteArray &bytes);
    void handle_device_message(const unsigned char *data, int len);
    void serial_send_packget(const Chunk &chunk);
    void saveRecordToExcel(int db, float current, int count, int error);
    void iapSendBytes(unsigned char *data, size_t len);
    void iapEvent(int EventType, std::string value);
    void iapResetDevice();
    void stopIap();
    bool startIap();
    void initIap();
    void setupPlotWidget(QCustomPlot *customPlot);
    void setupBarChartDemo(QCustomPlot *customPlot);
    void displayResult(int db, float current, int error);
    bool sendVictorCmd(char *cmd, int timeoutMs = 0, char *res = __null, int len = 0);

    void startTestVictor();
    void disconnectTCP();
    void connectTCP();

};














#endif // MAINWINDOW_H
