#ifndef TESTERTHREAD_H
#define TESTERTHREAD_H

#include <QObject>
#include <QThread>
#include "libs/chunk.h"
#include "QMetaType"
#include <QMutex>

#define PMSG_TAG_IAP		0x00
#define PMSG_TAG_ACK		0x10
#define PMSG_TAG_LOG		0x20
#define PMSG_TAG_DATA		0x30
#define PMSG_TAG_CMD		0x40

#define PC_TAG_LOGE  (PMSG_TAG_LOG|0x1)
#define PC_TAG_LOGI  (PMSG_TAG_LOG|0x2)

//PC to LED
#define PC_TAG_CMD_CAPTURE_EN  (PMSG_TAG_CMD | 0x01 ) // data[0] = 1 auto start, data[0] = 0 stop   ; 4101  4100
#define PC_TAG_CMD_LED_SELECT  (PMSG_TAG_CMD | 0x02 )  // data[0] = led id [1~12], data[1] = status  ; 420138  42010B
#define PC_TAG_CMD_CAPTURE_INTERVAL  (PMSG_TAG_CMD | 0x03 )  //data[0] = ms_L, data[1] = ms_H    ;  430A00 (10ms)
#define PC_TAG_CMD_TEST (PMSG_TAG_CMD | 0x04 )
#define PC_TAG_CMD_SWITCHES_TESTMODE (PMSG_TAG_CMD | 0x05 ) // data[0] = 2,3,4,5 , FF mean off all ; 4502  4503  4504  45FF
#define PC_TAG_CMD_SWITCHES_CHANNEL (PMSG_TAG_CMD | 0x06 ) // data[0] = 0~15 , FF mean off all    ; 4603 (Vled) 460C (VDD)
// PC->Control
#define PC_TAG_CMD_SWITCH  (PMSG_TAG_CMD | 0x07 ) // data[0] = SWITCH status   4701 (shave) 4702 (RESISTOR) 4704 (OUTPUT_METER)  4708 (VDD_METER) 4710 (POWER) 4720 (SIGNAL air )
#define PC_TAG_CMD_UART_TEST  (PMSG_TAG_CMD | 0x08 ) // data[0] = Uart ID , 0 = stop
#define PC_TAG_CMD_AMETER_READ  (PMSG_TAG_CMD | 0x09 ) //data[0] = read n times and send the mean data , 0 stop
#define PC_TAG_CMD_VMETER_READ  (PMSG_TAG_CMD | 0x0A ) //data[0] = read n times and send the mean data , 0 stop

//LED to PC
#define PC_TAG_DATA_LED_BRIGHTNESS (PMSG_TAG_DATA|0x1)
//Control to PC
#define PC_TAG_DATA_VMETER (PMSG_TAG_DATA|0x2)
#define PC_TAG_DATA_AMETER (PMSG_TAG_DATA|0x3)
#define PC_TAG_DATA_BUTTON (PMSG_TAG_DATA|0x4) // startbutton data[0] = 1 pressed
#define PC_TAG_DATA_QRCODE (PMSG_TAG_DATA|0x5)


//LED brightness range
#define LED_FULL_LOW_LEVEL 700000
#define LED_MID_HIGH_LEVEL 350000
#define LED_MID_LOW_LEVEL  150000
#define LED_ANIMATION_LOW_LEVEL  20000

//Measure value range
#define VDD_MAX_V_LEVEL 3.55
#define VDD_MIN_V_LEVEL 3.45
#define LED_FULL_MAX_V_LEVEL 0.9
#define LED_FULL_MIN_V_LEVEL 0.7
#define LED_MID_MAX_V_LEVEL 0.3
#define LED_MID_MIN_V_LEVEL 0.2
#define RLOAD_MAX_A_LEVEL 0.2
#define RLOAD_MIN_A_LEVEL 0.1



class TesterRecord{
public:
    int errorCode;
    QString errorCodeString;
    QString date;
    QString QRcode;
    float VDD;
    int LedFullLevel[12];
    float VLedFull;
    int LedMidLevel[12];
    float VLedMid;
    float RloadCurrent;
    bool LedAnimation;
};
Q_DECLARE_METATYPE(TesterRecord)



class TesterThread : public QThread
{
    Q_OBJECT
public:
    explicit TesterThread(QObject *parent = 0);

    typedef enum SerialID{
        BORAD_ID,
        SCANER_ID
    }SerialID_t;

    typedef enum ERRORCODE{
        E0=0,
        E1,//QR code scan error
        E2, //LED full brighness error
        E3, //VLED error
        E4, //VDD error
        E5, //LED mid brighness error
        E6, //LED mid voltage error
        E7, //Load resistor current error
        E8, //verify LED animation error after shaver connect
        E9, //ento test3mode error
    }ERRORCODE_t;


    volatile bool mDebugMode;
    volatile bool mNextStep;
    volatile bool mStartTest;
    volatile bool mExitcmd;




    void checkDebug();

signals:
    void sendSerialCmd(int id, unsigned char* data, int len);
    void log(QString str);
    void result(TesterRecord res);
    void error(QString errorStr);

public slots:
    void update_data(int tag, unsigned char *data);
    void debug_step(bool start);
    void testThread_start(bool start);
    void testThread_exit();

protected:
    void run() override;

private:

    volatile bool QRcode_update;
    QString QRcode;
    volatile int mRelayStatus;
    volatile int mLedAnimationStep;
    volatile int mLedAnimationIndex;


    volatile int ledhalfonTimeoutMs;
    volatile int ledAnimationTimeMs;

    volatile int LedBrightness_update_mean_cnt;
    volatile bool LedBrightness_update;
    int LedBrightness[12];
    volatile int LedBrightness_current_cnt;
    int LedBrightness_current[12];

    volatile int VMeter_update_mean_cnt;
    volatile bool VMeter_update;
    float VMeter_value;
    volatile int VMeter_current_cnt;
    float VMeter_value_current;

    QMutex mMutex;
    void sendcmd(int tag, int data);
    void testThread_reset();
    int testLedAnimationLoop();
    void testThread_clear_data(int led_mean_cnt, int vmeter_mean_cnt);

};


#define setNormalMode() sendcmd(PC_TAG_CMD_SWITCHES_TESTMODE,0xff)
#define setTest2Mode() sendcmd(PC_TAG_CMD_SWITCHES_TESTMODE,2)
#define setTest3Mode() sendcmd(PC_TAG_CMD_SWITCHES_TESTMODE,3)
#define setTest4Mode() sendcmd(PC_TAG_CMD_SWITCHES_TESTMODE,4)
#define setPowerOn() {mRelayStatus = (mRelayStatus | 0x10); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setPowerOff() {mRelayStatus = (mRelayStatus & (~0x10)); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setConnectShaver() {mRelayStatus = (mRelayStatus | 0x1); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setDisconnectShaver() {mRelayStatus = (mRelayStatus & (~0x1)); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setConnectResistor() {mRelayStatus = (mRelayStatus | 0x2); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setDisconnectResistor() {mRelayStatus = (mRelayStatus & (~0x2)); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setConnectOutVmeter() {mRelayStatus = ((mRelayStatus|0x8)&(~0x4)); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setDisconnectOutVmeter() {mRelayStatus = (mRelayStatus & (~0x8)); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setAirCylinderOn() {mRelayStatus = (mRelayStatus | 0x20); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setAirCylinderOff() {mRelayStatus = (mRelayStatus & (~0x20)); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setConnectMeasureVmeter() {mRelayStatus = ((mRelayStatus|0x4)&(~0x8)); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setDisconnectMeasureVmeter() {mRelayStatus = (mRelayStatus & (~0x4)); sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);}
#define setMeasureVDD() sendcmd(PC_TAG_CMD_SWITCHES_CHANNEL,0x0c)
#define setMeasureLED() sendcmd(PC_TAG_CMD_SWITCHES_CHANNEL,0x03)
#define setMeasureNone() sendcmd(PC_TAG_CMD_SWITCHES_CHANNEL,0x0f)
#define setLedCaptureStart() sendcmd(PC_TAG_CMD_CAPTURE_EN,0x1)
#define setLedCaptureStop() sendcmd(PC_TAG_CMD_CAPTURE_EN,0x0)
#define setVmeterReadStart() sendcmd(PC_TAG_CMD_VMETER_READ,0x1)
#define setVmeterReadStop() sendcmd(PC_TAG_CMD_VMETER_READ,0x0)
#define setAlloff() { setVmeterReadStop();setLedCaptureStop();setNormalMode(); mRelayStatus=0;sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus); setMeasureNone();}

#endif // TESTERTHREAD_H
