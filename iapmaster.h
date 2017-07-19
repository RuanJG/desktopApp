#ifndef IAPMASTER_H
#define IAPMASTER_H

#include <iostream>
#include "uartcoder.h"
#include "chunk.h"
#include <fstream>
#include <cstdlib>
#include <mutex>






class IapMaster
{
public:
    typedef struct _ackPackget {
        unsigned char type;
        unsigned char value;
        //tag for check packget is update
        bool valid;
    }IapAckPackget;


    enum STEP_T{
        STEP_IDEL,
        STEP_START,
        STEP_DATA,
        STEP_JUMP,
    };

    enum EVENT_TYPE{
        EVENT_TYPE_STARTED = 1,
        EVENT_TYPE_STATUS,
        EVENT_TYPE_FALSE,
        EVENT_TYPE_FINISH,
    };


public:
    IapMaster();
    ~IapMaster();

    bool    iapStart( std::string  filepath);
    void    iapStop();
    bool    isIapStarted();
    float   getIapStatus();
    void    iapParse (unsigned char *data, size_t len);
    void    iapTick(int intervalMs);

protected:
    virtual void iapSendBytes (unsigned char *data, size_t len)=0;
    virtual void iapEvent (int EventType, std::string value)=0;



private:

    std::string     mFirmwarePath;
    size_t  mFirmwareSize;
    size_t  mCurrentSize;
    unsigned char *mFirmwareBuffer;
    volatile int mStep;
    UartCoder mDecoder;
    UartCoder mEncoder;
    unsigned char mCurrentDataSeq;
    unsigned int mTimeOutMs;
    IapAckPackget mAckPkg;
    std::mutex   mMutex;




    void handle_device_message(const unsigned char *data, int len);
    void sendPackget(unsigned char *data, int len);
    void sendStartPackget();
    void iapReset();
    void sendDataPackget(unsigned char *data, int len);
    void sendJumpPackget();
    void setStep(int step);
};




// example

//class MainWindow : public QMainWindow,IapMaster
//{
//}

//void MainWindow::iapSendBytes(unsigned char *data, size_t len)
//{
//    mSerialMutex.lock();

//    if( mSerialport != NULL && mSerialport->isOpen()){
//        size_t count = mSerialport->write( (const char*) data, len );
//        if( count != len ){
//            qDebug() << "iapSendHandler: send data to serial false \n" << endl;
//        }
//    }
//    mSerialMutex.unlock();
//}


//void MainWindow::iapEvent(int EventType, std::string value)
//{

//    if( EventType == IapMaster::EVENT_TYPE_STARTED){
//        ui->textBrowser->append("iap started:");
//    }else if ( EventType == IapMaster::EVENT_TYPE_FINISH){
//        ui->textBrowser->append("iap finished");
//    }else if( EventType == IapMaster::EVENT_TYPE_FALSE){
//        ui->textBrowser->append("iap false: "+ value.empty() ? "...":QString::fromStdString(value) );
//    }
//}


//void MainWindow::iapTickHandler(){

//    this->iapTick(10);
//}


//void MainWindow::initIap()
//{
//    mTimer.stop();
//    this->iapStop();
//    connect(&mTimer,SIGNAL(timeout()),this,SLOT(iapTickHandler()));
//}

//void MainWindow::startIap()
//{
//    this->iapStart("C:\\iap.bin");
//    mTimer.start(10);
//}

//void MainWindow::stopIap()
//{
//    mTimer.stop();
//    this->iapStop();
//}



#endif // IAPMASTER_H
