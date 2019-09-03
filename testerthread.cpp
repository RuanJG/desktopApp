#include "testerthread.h"
#include <QDateTime>
#include <QDebug>

TesterThread::TesterThread(QObject *parent) : QThread(parent)
{
    mExitcmd = false;
    mStartTest = false;
    mDebugMode = false;
    mNextStep = false;
    testThread_reset();
}

void TesterThread::testThread_reset()
{
    LedBrightness_update = false;
    QRcode_update  = false;
    VMeter_update = false;
    mRelayStatus = 0;
}

void TesterThread::testThread_start(bool start)
{
    mStartTest = start;
}

void TesterThread::testThread_exit()
{
    mExitcmd = true;
}

void TesterThread::update_data(int tag, unsigned char *data)
{
    if( tag == PC_TAG_DATA_LED_BRIGHTNESS ){
        int *pdata = (int*)data;
        for( int i=0; i<12 ; i++) LedBrightness[i]=pdata[i];
        LedBrightness_update = true;
    }else if( tag == PC_TAG_DATA_QRCODE){
        QRcode = QString::fromLocal8Bit((char*)data);
        QRcode_update  = true;
    }else if( tag == PC_TAG_DATA_VMETER){
        VMeter_value = *((float*)data);
        VMeter_update = true;
    }
}

void TesterThread::debug_step(bool start)
{
    //mDebugMode = start;
    mNextStep = start;
}




void TesterThread::sendcmd(int tag, int data)
{
    unsigned char cmd[2];
    cmd[0]=tag;
    cmd[1]=data;
    emit sendSerialCmd( BORAD_ID , cmd, 2);
}




void TesterThread::checkDebug()
{
    mNextStep = false;
    while( mDebugMode && !mNextStep ){
        if( mExitcmd ) break;
        if( !mStartTest ) break;
        msleep(10);
    }
}

#define checkEven() { checkDebug();if( mExitcmd ) {setAlloff();return;} if(!mStartTest ){setAlloff();continue;}}










void TesterThread::run()
{
    unsigned char cmd[8];
    volatile int retry = 0;
    volatile int timeoutMs = 0;
    TesterRecord testRes;
    QString qrcodeExample = "CSWL-B01-S01-19325-000008";
    bool keepTestEvenNG = true;



    while(true)
    {
        testRes = TesterRecord();
        testRes.date = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        testRes.errorCode = ERRORCODE::E0;
        mStartTest = false;
        emit log("AutoTest: Standby");
        while( !mStartTest )
        {
            if( mExitcmd ){
                return;
            }
            msleep(10);
        }
        testThread_reset();

    //first open all relay and switches
        emit log("AutoTest: 1 close all relay and switches");
        setLedCaptureStop();
        mRelayStatus=0;
        sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);
        checkEven();
    //start Vmeter
        emit log("AutoTest: 2 start VMeter");
        setVmeterReadStart();
    //press down the PCBA
        emit log("AutoTest: 3 Load PCBA");
        setAirCylinderOn();
        msleep(3000);
        checkEven();
    //scan the QRcode
        emit log("AutoTest: 4 Scan QRcode");
        cmd[0]=0x16; cmd[1] =0x54; cmd[2]=0x0D;
        QRcode_update = false;
        for( retry = 0, timeoutMs = 0;  retry < 3 ; retry++){
            emit sendSerialCmd(SCANER_ID , cmd, 3);
            while(!QRcode_update){
                msleep(10);
                timeoutMs+=10;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 3000 ) break;
            }
            if( QRcode_update )break;
        }
        if( QRcode_update ) {
            testRes.QRcode = QRcode;
            emit log("get QRcode: "+ QRcode);
            if( QRcode.length() != qrcodeExample.length() ){
                testRes.errorCode = ERRORCODE::E1;
            }
            if( testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error("Scan QRcode timeout");
            setAlloff();
            continue;
        }
        checkEven();
    //set TP300 to high
        emit log("AutoTest: 4 TP300 connect to TP107");
        setNormalMode();
        setTest2Mode();
        msleep(100);
        checkEven();
    //power on
        emit log("AutoTest: 5 Power On 15V");
        setPowerOn();
        msleep(1000);
        checkEven();
    //dissconnect TP300
        emit log("AutoTest: 6 disconnect TP300 ");
        setNormalMode();
        msleep(100);
        checkEven();
    //Verify that all LEDs are fully ON
        emit log("AutoTest: 7 Verify Led brighness");
        setLedCaptureStart();
        msleep(100);
        LedBrightness_update = false;
        for( retry = 0, timeoutMs = 0;  retry < 3 ; retry++){
            while(!LedBrightness_update){
                msleep(10);
                timeoutMs+=10;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 1000 ) break;
            }
            if( LedBrightness_update )break;
        }
        if( LedBrightness_update ) {
            for( int i=0; i<12; i++ ){
                //TODO check the level in range;
                if( LedBrightness[i] < LED_FULL_LOW_LEVEL ){
                    testRes.errorCode = ERRORCODE::E2;
                }
                testRes.LedFullLevel[i] = LedBrightness[i];
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error("Verify Led brighness timeout");
            setAlloff();
            continue;
        }
        setLedCaptureStop();
        checkEven();
    //Verify LED Voltage
        emit log("AutoTest: 8 Verify Led Voltage");
        setMeasureLED();
        setConnectMeasureVmeter();
        msleep(500);
        VMeter_update = false;
        for( retry = 0, timeoutMs = 0;  retry < 3 ; retry++){
            while(!VMeter_update){
                msleep(10);
                timeoutMs+=10;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 1000 ) break;
            }
            if( VMeter_update )break;
        }
        if( VMeter_update ){
            testRes.VLedFull = VMeter_value;
            if( VMeter_value > LED_FULL_MAX_V_LEVEL || VMeter_value < LED_FULL_MIN_V_LEVEL){
                testRes.errorCode = ERRORCODE::E3;
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error("Verify Led current timeout");
            setAlloff();
            continue;
        }
        setMeasureNone();
        //setDisconnectMeasureVmeter();
        msleep(100);
        checkEven();
    //Verify VDD
        emit log("AutoTest: 9 Verify VDD");
        setMeasureVDD();
        setConnectMeasureVmeter();
        msleep(500);
        VMeter_update = false;
        for( retry = 0, timeoutMs = 0;  retry < 3 ; retry++){
            while(!VMeter_update){
                msleep(10);
                timeoutMs+=10;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 1000 ) break;
            }
            if( VMeter_update )break;
        }
        if( VMeter_update ){
            testRes.VDD = VMeter_value;
            if( VMeter_value > VDD_MAX_V_LEVEL || VMeter_value < VDD_MIN_V_LEVEL){
                testRes.errorCode = ERRORCODE::E4;
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error("Verify VDD timeout");
            setAlloff();
            continue;
        }
        setMeasureNone();
        setDisconnectMeasureVmeter();
        checkEven();
    //Power off
        emit log("AutoTest: 10 Power off");
        setPowerOff();
        msleep(1000);
        checkEven();
    //TP303 High->Power on->TP303 LOW
        emit log("AutoTest: 11 Into TP303 mode");
        setNormalMode();
        setTest3Mode();
        msleep(100);
        setPowerOn();
        msleep(1000);
        setNormalMode();
        msleep(100);
        checkEven();
    //Verify Led mid brighness
        emit log("AutoTest: 12 Verify Led Mid brighness");
        setLedCaptureStart();
        msleep(100);
        LedBrightness_update = false;
        for( retry = 0, timeoutMs = 0;  retry < 3 ; retry++){
            while(!LedBrightness_update){
                msleep(10);
                timeoutMs+=10;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 1000 ) break;
            }
            if( LedBrightness_update )break;
        }
        if( LedBrightness_update ) {
            for( int i=0; i<12; i++ ){
                //TODO check the level in range;
                testRes.LedMidLevel[i] = LedBrightness[i];
                if( LedBrightness[i] < LED_MID_LOW_LEVEL || LedBrightness[i] > LED_MID_HIGH_LEVEL ){
                    testRes.errorCode = ERRORCODE::E5;
                }
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error("Verify Led Mid brighness timeout");
            setAlloff();
            continue;
        }
        setLedCaptureStop();
        checkEven();
    //Verify Led mid voltage
        emit log("AutoTest: 13 Verify Led mid current");
        setMeasureLED();
        setConnectMeasureVmeter();
        msleep(300);
        VMeter_update = false;
        for( retry = 0, timeoutMs = 0;  retry < 3 ; retry++){
            while(!VMeter_update){
                msleep(10);
                timeoutMs+=10;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 1000 ) break;
            }
            if( VMeter_update )break;
        }
        if( VMeter_update ){
            testRes.VLedMid = VMeter_value;
            if( VMeter_value > LED_MID_MAX_V_LEVEL || VMeter_value < LED_MID_MIN_V_LEVEL){
                testRes.errorCode = ERRORCODE::E6;
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error("Verify Led mid current timeout");
            setAlloff();
            continue;
        }
        setMeasureNone();
        setDisconnectMeasureVmeter();
        checkEven();
    //Power off
        emit log("AutoTest: 14 Power off");
        setPowerOff();
        msleep(1000);
        checkEven();
    //TP306 High->Power on->TP306 LOW
        emit log("AutoTest: 15 Into TP306 mode");
        setNormalMode();
        setTest4Mode();
        msleep(100);
        setPowerOn();
        msleep(1000);
        setNormalMode();
        msleep(100);
        checkEven();
    //Verify RLoad voltage
        emit log("AutoTest: 16 Verify RLoad current");
        setConnectOutVmeter();
        msleep(100);
        setConnectResistor();
        msleep(2000);
        checkEven();
        VMeter_update = false;
        for( retry = 0, timeoutMs = 0;  retry < 3 ; retry++){
            while(!VMeter_update){
                msleep(10);
                timeoutMs+=10;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 1000 ) break;
            }
            if( VMeter_update )break;
        }
        if( VMeter_update ){
            testRes.RloadCurrent = VMeter_value/100; // A = V/100R
            emit log("VRload="+QString::number(VMeter_value,'f',6)+"; A="+QString::number(testRes.RloadCurrent,'f',6));
            if( testRes.RloadCurrent > RLOAD_MAX_A_LEVEL || testRes.RloadCurrent < RLOAD_MIN_A_LEVEL){
                testRes.errorCode = ERRORCODE::E7;
            }
            if(!keepTestEvenNG &&  testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error("Verify RLoad current timeout");
            setAlloff();
            continue;
        }
        setDisconnectOutVmeter();
        setDisconnectResistor();
        msleep(100);
        checkEven();
    //Power off
        emit log("AutoTest: 17 Power off");
        setPowerOff();
        msleep(1000);
        checkEven();
    //Normal mode
        emit log("AutoTest: 18 Into normal mode");
        setNormalMode();
        msleep(100);
        setPowerOn();
        msleep(500);
        checkEven();
    //Verify LED animation
        emit log("AutoTest: 19 Verify LED animation");
        setConnectShaver();
        setLedCaptureStart();
        setVmeterReadStop();
        msleep(100);
        volatile int ledAnimationTimeMs = 0;
        mLedAnimationStep = 0;
        mLedAnimationIndex = 0;
        LedBrightness_update = false;
        int res;
        bool checkRes;

        checkRes = false;
        testRes.LedAnimation = false;
        while(ledAnimationTimeMs < 6000 )
        {
            timeoutMs = 0;
            while(!LedBrightness_update){
                msleep(10);
                timeoutMs+=10;
                ledAnimationTimeMs+=10;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 1000 ) break;
            }
            if( LedBrightness_update ) {
                res = testLedAnimationLoop();
                if( -1 ==  res){
                    testRes.errorCode = ERRORCODE::E8;
                    checkRes = false;
                    break;
                }
                if( 1 == res ){
                    testRes.LedAnimation = true;
                    checkRes = true;
                    break;
                }
            }else{
                emit error("Verify Led animation data timeout");
                checkRes = false;
                break;
            }
        }
        if( !checkRes ){
            if( ledAnimationTimeMs >= 6000 )
                emit error("Verify Led animation timeout");
            emit result(testRes);
            setAlloff();
            continue;
        }
        setDisconnectShaver();
        setLedCaptureStop();
        msleep(100);
        checkEven();

    //send the result and reset
        emit log("AutoTest: test Finish");
        emit result(testRes);
        setPowerOff();
        msleep(100);
        setAirCylinderOff();
        setAlloff();
        testThread_reset();
    }

}


//return 1 successfully, 0 testing, -1 failed
int TesterThread::testLedAnimationLoop()
{
    bool checkOK;

    switch(mLedAnimationStep)
    {

    case 0:{
        //check all full light
        checkOK = true;
        for( int i=0; i<12; i++ ){
            if( LedBrightness[i] < LED_FULL_LOW_LEVEL) {
                checkOK = false;
            }
        }
        if( checkOK ) {
            emit log("Step0 OK");
            mLedAnimationStep++;
        }
        break;
    }
    case 1:{
        //check all mid light
        checkOK = true;
        for( int i=0; i<12; i++ ){
            if( LedBrightness[i] < LED_MID_LOW_LEVEL || LedBrightness[i] > LED_MID_HIGH_LEVEL) {
                checkOK = false;
            }
        }
        if( checkOK ) {
            emit log("Step1 OK");
            mLedAnimationStep++;
        }
        break;
    }
    case 2:{
        //check mid -> full

        if( mLedAnimationIndex >= 12 ) return 1;
#if 1
        if( LedBrightness[mLedAnimationIndex] < LED_FULL_LOW_LEVEL){
            break;
        }
        unsigned char ledpos[12] = {0};
        ledpos[mLedAnimationIndex] = 1;
        for( int j=1; j<=4; j++ ){ //the last 5 or < 5 led is full
            if( (mLedAnimationIndex - j) >= 0){
                ledpos[mLedAnimationIndex-j] = 1;
            }
        }
        for( int i=0; i< 12; i++){
            if( LedBrightness[i] < LED_ANIMATION_LOW_LEVEL ){
                emit log("Step2 Failed, LED"+QString::number(i+1)+"is too dim:"+QString::number(LedBrightness[i]));
                return -1;
            }
            if( ledpos[i] == 1 && LedBrightness[i] <LED_FULL_LOW_LEVEL ){
                emit log("Step2 Failed, LED"+QString::number(i+1)+"should be high");
                return -1;
            }
            /*
            if( ledpos[i] == 0 && LedBrightness[i] > LED_FULL_LOW_LEVEL){
                emit log("Step2 Failed, LED"+QString::number(i+1)+"should be mid");
                return -1;
            }
            */
        }
        mLedAnimationIndex++;
#endif

#if 0
        if( LedBrightness[mLedAnimationIndex] > LED_FULL_LOW_LEVEL){
            //emit log("To LED"+QString::number(mLedAnimationIndex+1));
            for( int j=1; j<=5; j++ ){ //the last 5 or < 5 led is full
                if( (mLedAnimationIndex - j) >= 0){
                    if( LedBrightness[mLedAnimationIndex-j] < LED_FULL_LOW_LEVEL){
                        emit log("Step2 Failed, LED"+QString::number(mLedAnimationIndex-j+1)+" is not in right animation");
                        return -1;
                    }else{
                        //emit log(","+QString::number(mLedAnimationIndex-j+1));
                    }
                }else{
                    break;
                }
            }
            mLedAnimationIndex++;
        }
#endif

        if( mLedAnimationIndex >= 12 ) {
            emit log("Step2 OK");
            return 1;
        }
        break;
    }

    }


    return 0;
}
