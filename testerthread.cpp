#include "testerthread.h"
#include <QDateTime>
#include <QDebug>





TesterThread::TesterThread(QObject *parent) : QThread(parent),mMutex()
{
    mExitcmd = false;
    mStartTest = false;
    mDebugMode = false;
    mNextStep = false;
    testThread_reset();
}

void TesterThread::testThread_reset()
{
    mRelayStatus = 0;
    testThread_clear_data(1,1);
}

void TesterThread::testThread_clear_data(int led_mean_cnt, int vmeter_mean_cnt)
{
    mMutex.lock();

    QRcode_update  = false;

    LedBrightness_update = false;
    VMeter_update = false;


    LedBrightness_update_mean_cnt = led_mean_cnt;
    LedBrightness_current_cnt = 0;
    for( int i=0; i< 12; i++)
        LedBrightness_current[i]=0;

    VMeter_update_mean_cnt = vmeter_mean_cnt;
    VMeter_current_cnt = 0;
    VMeter_value_current = 0;

    mMutex.unlock();
}

void TesterThread::testThread_start(bool start)
{
    mStartTest = start;
}

void TesterThread::testThread_exit()
{
    mExitcmd = true;
}

void TesterThread::update_data(int tag, QByteArray bytes)
{
    unsigned char *data = (unsigned char*) bytes.data();

    mMutex.lock();
    if( tag == PC_TAG_DATA_LED_BRIGHTNESS ){
        int *pdata = (int*)data;
#if 0
        for( int i=0; i<12 ; i++) LedBrightness[i]=pdata[i];
        LedBrightness_update = true;

#else
        LedBrightness_current_cnt++;
        for( int i=0; i<12 ; i++) {
            LedBrightness_current[i]+=pdata[i];
            if( LedBrightness_current_cnt >= LedBrightness_update_mean_cnt ){
                LedBrightness[i] = LedBrightness_current[i] / LedBrightness_current_cnt;
                LedBrightness_current[i] = 0;
            }
        }
        if( LedBrightness_current_cnt >= LedBrightness_update_mean_cnt ){
            LedBrightness_update = true;
            LedBrightness_current_cnt = 0;
        }
#endif
    }else if( tag == PC_TAG_DATA_QRCODE){
        QRcode = QString::fromLocal8Bit(bytes);
        QRcode_update  = true;
    }else if( tag == PC_TAG_DATA_VMETER){
        float Vol;
        memcpy( (char*)&Vol , data , 4);
#if 0
        VMeter_value = *((float*)data);
        VMeter_update = true;
#else
        VMeter_current_cnt++;
        VMeter_value_current += Vol;
        if( VMeter_current_cnt >= VMeter_update_mean_cnt){
            VMeter_value = VMeter_value_current/VMeter_current_cnt;
            VMeter_value_current = 0;
            VMeter_current_cnt = 0;
            VMeter_update = true;
        }
#endif
    }
    mMutex.unlock();
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

    QByteArray cmdBA;
    cmdBA.append((char*)cmd,2);
    //qDebug()<< "thread send cmd : 0x"+QByteArray((char*)cmd,2).toHex();
    //emit sendSerialCmd( BORAD_ID , cmd, 2);
    emit sendSerialCmd(BORAD_ID ,cmdBA);
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

#define checkEven() { checkDebug();if( mExitcmd ) {setAlloff();return;} if(!mStartTest ){setAlloff();emit error("Test abort");continue;}}










void TesterThread::run()
{
    unsigned char cmd[8];
    volatile int retry = 0;
    volatile int timeoutMs = 0;
    TesterRecord testRes;
    QString qrcodeExample = "CSWL-9711.1-B01-S01-19325-000008";
    bool keepTestEvenNG = false;
    int poweroffMs = 300;
    int switchmodeMs = 1000;



    while(true)
    {
        testRes = TesterRecord();
        testRes.date = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        testRes.errorCode = ERRORCODE::E0;
        testRes.errorCodeString = "";
        testRes.errorCodeParameter ="";
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
        emit log("AutoTest: 1 关闭所有开关");
        setLedCaptureStop();
        mRelayStatus=0;
        sendcmd(PC_TAG_CMD_SWITCH,mRelayStatus);
        checkEven();
    //start Vmeter
        emit log("AutoTest: 2  开启万用表");
        setVmeterReadStart();
    //press down the PCBA
        emit log("AutoTest: 3  等待压板");
        setAirCylinderOn();
        msleep(1000);
        checkEven();
    //scan the QRcode
        emit log("AutoTest: 4  扫描二维码");
        cmd[0]=0x16; cmd[1] =0x54; cmd[2]=0x0D;
        QByteArray scancmd = QByteArray((char*)cmd,3);
        QRcode_update = false;
        for( retry = 0, timeoutMs = 0;  retry < 3 ; retry++){
            emit sendSerialCmd(SCANER_ID ,scancmd);
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
            if( QRcode.length() != qrcodeExample.length() || QRcode.left(11) != "CSWL-9711.1" ){
                testRes.errorCode = ERRORCODE::E1;
                testRes.errorCodeString = QRcode+tr(":二维码不符合要求");
            }
            if( testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            testRes.errorCode = ERRORCODE::E1;
            testRes.errorCodeString = tr("扫描二维码超时");
            emit result(testRes);
            setAlloff();
            continue;
        }
        checkEven();
    //set TP300 to high
        emit log("AutoTest: 4 TP300 connect to TP107");
        setNormalMode();
        setTest2Mode();
        msleep(20);
        checkEven();
    //power on
        emit log("AutoTest: 5 Power On 15V");
        setPowerOn();
        msleep(switchmodeMs);
        checkEven();
    //dissconnect TP300
        emit log("AutoTest: 6 disconnect TP300 ");
        setNormalMode();
        //msleep(10);
        checkEven();
    //Verify that all LEDs are fully ON
        emit log("AutoTest: 7 Verify Led brighness");
        setMeasureLED();
        setConnectMeasureVmeter();
        setLedCaptureStart();
        msleep(1000);
        //LedBrightness_update_mean_cnt = 10;
        //LedBrightness_update = false;
        testThread_clear_data(10,5);
        timeoutMs = 0;
        while(!LedBrightness_update){
            msleep(10);
            timeoutMs+=10;
            if( mExitcmd || !mStartTest ) break;
            if( timeoutMs >= 1000 ) break;
        }
        if( LedBrightness_update ) {
            for( int i=0; i<12; i++ ){
                //TODO check the level in range;
                if( LedBrightness[i] < LED_FULL_LOW_LEVEL || LedBrightness[i] > LED_FULL_HIGH_LEVEL){
                    testRes.errorCode = ERRORCODE::E2;
                    if( testRes.errorCodeParameter.length() == 0 )
                        testRes.errorCodeParameter += "LED";
                    else
                        testRes.errorCodeParameter += "&";
                    testRes.errorCodeParameter += (QString::number(i+1));
                    testRes.errorCodeString += tr("LED%1 100%亮度(%2)不在范围(%3~%4);").arg(i+1).arg(LedBrightness[i]).arg(LED_FULL_LOW_LEVEL).arg(LED_FULL_HIGH_LEVEL);
                }
                testRes.LedFullLevel[i] = LedBrightness[i];
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error(tr("100%亮度时LED亮度数据读取超时"));
            setAlloff();
            continue;
        }
        emit log("AutoTest: 8 Verify Led Voltage");
        timeoutMs = 0;
        while(!VMeter_update){
            msleep(10);
            timeoutMs+=10;
            if( mExitcmd || !mStartTest ) break;
            if( timeoutMs >= 2000 ) break;
        }
        if( VMeter_update ){
            testRes.VLedFull = VMeter_value;
            if( VMeter_value > LED_FULL_MAX_V_LEVEL || VMeter_value < LED_FULL_MIN_V_LEVEL){
                testRes.errorCode = ERRORCODE::E3;
                testRes.errorCodeString = tr("100%亮度时LED电压值(%1)不在范围[%2~%3]").arg(QString::number(VMeter_value,'f',4)).arg(QString::number(LED_FULL_MIN_V_LEVEL)).arg(QString::number(LED_FULL_MAX_V_LEVEL));
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error(tr("100%亮度时LED电压数据读取超时"));
            setAlloff();
            continue;
        }
        setMeasureNone();
        setLedCaptureStop();
        checkEven();

    //Verify VDD
        emit log("AutoTest: 9 Verify VDD");
        setMeasureVDD();
        setConnectMeasureVmeter();
        msleep(100);
        //VMeter_update_mean_cnt = 5;
        //VMeter_update = false;
        testThread_clear_data(1,5);
        timeoutMs = 0;
        while(!VMeter_update){
            msleep(10);
            timeoutMs+=10;
            if( mExitcmd || !mStartTest ) break;
            if( timeoutMs >= 2000 ) break;
        }
        if( VMeter_update ){
            testRes.VDD = VMeter_value;
            if( VMeter_value > VDD_MAX_V_LEVEL || VMeter_value < VDD_MIN_V_LEVEL){
                testRes.errorCode = ERRORCODE::E4;
                testRes.errorCodeString = tr("VDD=（%1）不在范围[%2~%3]").arg(QString::number(VMeter_value,'f',4)).arg(QString::number(VDD_MIN_V_LEVEL)).arg(QString::number(VDD_MAX_V_LEVEL));
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error(tr("VDD电压数据读取超时"));
            setAlloff();
            continue;
        }
        setMeasureNone();
        setDisconnectMeasureVmeter();
        checkEven();
    //Power off
        emit log("AutoTest: 10 Power off");
        setPowerOff();
        msleep(poweroffMs);
        checkEven();
    //TP303 High->Power on->TP303 LOW
        emit log("AutoTest: 11 Into TP303 mode");
        setNormalMode();
        setTest3Mode();
        msleep(100);
        setPowerOn();
        msleep(switchmodeMs);
        setNormalMode();
        msleep(20);
        checkEven();
    //Verify Led mid brighness
        emit log("AutoTest: 12 Verify Led Mid brighness");
        setLedCaptureStart();
        setMeasureLED();
        setConnectMeasureVmeter();
        msleep(500);
        //LedBrightness_update_mean_cnt = 10;
        //LedBrightness_update = false;
        testThread_clear_data(10,5);
        timeoutMs = 0;
        while(!LedBrightness_update){
            msleep(10);
            timeoutMs+=10;
            if( mExitcmd || !mStartTest ) break;
            if( timeoutMs >= 1000 ) break;
        }
        if( LedBrightness_update ) {
            for( int i=0; i<12; i++ ){
                //TODO check the level in range;
                testRes.LedMidLevel[i] = LedBrightness[i];
                if( LedBrightness[i] < LED_MID_LOW_LEVEL || LedBrightness[i] > LED_MID_HIGH_LEVEL ){
                    testRes.errorCode = ERRORCODE::E5;
                    testRes.errorCodeParameter += (QString::number(i+1)+",");
                    testRes.errorCodeString += tr("LED%1 30%亮度(%2)不在范围[%3~%4];").arg(i+1).arg(LedBrightness[i]).arg(LED_MID_LOW_LEVEL).arg(LED_MID_HIGH_LEVEL);
                }
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error(tr("30%亮度时，LED亮度数据读取超时"));
            setAlloff();
            continue;
        }
        emit log("AutoTest: 13 Verify Led mid current");
        timeoutMs = 0;
        while(!VMeter_update){
            msleep(10);
            timeoutMs+=10;
            if( mExitcmd || !mStartTest ) break;
            if( timeoutMs >= 2000 ) break;
        }
        if( VMeter_update ){
            testRes.VLedMid = VMeter_value;
            if( VMeter_value > LED_MID_MAX_V_LEVEL || VMeter_value < LED_MID_MIN_V_LEVEL){
                testRes.errorCode = ERRORCODE::E6;
                testRes.errorCodeString = tr("30%亮度时LED电压值(%1)不在范围[%2~%3]").arg(QString::number(VMeter_value,'f',4)).arg(LED_MID_MIN_V_LEVEL).arg(LED_MID_MAX_V_LEVEL);
            }
            if( !keepTestEvenNG && testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error(tr("30%亮度时，LED电压数据读取超时"));
            setAlloff();
            continue;
        }
        setMeasureNone();
        setDisconnectMeasureVmeter();
        setLedCaptureStop();
        checkEven();

    //Power off
        emit log("AutoTest: 14 Power off");
        setPowerOff();
        msleep(poweroffMs);
        checkEven();

    //TP306 High->Power on->TP306 LOW
        emit log("AutoTest: 15 Into TP306 mode");
        setNormalMode();
        setTest4Mode();
        msleep(100);
        setPowerOn();
        msleep(switchmodeMs);
        setNormalMode();
        msleep(20);
        checkEven();

        setConnectOutVmeter();
        setConnectResistor();

        //Verify Led half on , half off
        setLedCaptureStart();
        msleep(100);
        ledhalfonTimeoutMs = 0;
        bool ledhalfon = false;

        //LedBrightness_update_mean_cnt = 10;
        //LedBrightness_update = false;
        testThread_clear_data(1,5);
        while( ledhalfonTimeoutMs < 3000)
        {
            timeoutMs = 0;
            while(!LedBrightness_update){
                msleep(10);
                timeoutMs+=10;
                ledhalfonTimeoutMs += 10;
                if( ledhalfonTimeoutMs > 3000 ) break;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 2000 ) break;
            }
            if( LedBrightness_update ) {
                int LenonCnt = 0;
                for( int i=0; i<12; i++ ){
                    if( LedBrightness[i] >= LED_MID_LOW_LEVEL ){
                        LenonCnt++;
                    }
                }
                if( LenonCnt == 6 ) { ledhalfon = true; break;}
                if( LenonCnt > 6 ) { ledhalfon = false; emit error(tr("进入测试模式4，LED亮度异常"));break;}
                LedBrightness_update = false; //!!!!!
            }else{
                emit error(tr("进入测试模式4，LED亮度数据读取超时"));
                ledhalfon = false;
                break;
            }
            if( mExitcmd || !mStartTest ) break;
            msleep(1);
            ledhalfonTimeoutMs++;
        }
        if( !ledhalfon ){
            //testRes.errorCode = TesterThread::ERRORCODE::E9;
            //testRes.errorCodeString =  tr("进行测试模式4后LED亮度异常");
            if( !keepTestEvenNG ){
                //emit result(testRes);
                setAlloff();
                //if( ledhalfonTimeoutMs >= 3000 ) emit error(tr("测试模式4，LED检测超时"));
                continue;
            }else{
                if( ledhalfonTimeoutMs >= 3000 ) emit log("Verify Led harf on timeout");
            }
        }else{
            emit log("Verify Led harf on OK");
        }
        checkEven();

        emit log("AutoTest: 16 Verify RLoad current");
        timeoutMs = 0;
        while(!VMeter_update){
            msleep(10);
            timeoutMs+=10;
            if( mExitcmd || !mStartTest ) break;
            if( timeoutMs >= 2000 ) break;
        }
        if( VMeter_update ){
            testRes.RloadCurrent = VMeter_value/100; // A = V/100R
            emit log("VRload="+QString::number(VMeter_value,'f',6)+"; A="+QString::number(testRes.RloadCurrent,'f',6));
            if( testRes.RloadCurrent > RLOAD_MAX_A_LEVEL || testRes.RloadCurrent < RLOAD_MIN_A_LEVEL){
                testRes.errorCode = ERRORCODE::E7;
                testRes.errorCodeString = tr("负载电阻充电电流(%1)不在范围[%2~%3]").arg(QString::number(VMeter_value,'f',4)).arg(QString::number(RLOAD_MIN_A_LEVEL)).arg(QString::number(RLOAD_MAX_A_LEVEL));//+tr(", 不符合要求");
            }
            if(!keepTestEvenNG &&  testRes.errorCode != ERRORCODE::E0){
                emit result(testRes);
                setAlloff();
                continue;
            }
        }else{
            emit error(tr("负载电阻的电压数据读取超时"));
            setAlloff();
            continue;
        }
        setDisconnectOutVmeter();
        setDisconnectResistor();


        setLedCaptureStop();
        checkEven();
 #if 0
    //Verify RLoad voltage
        emit log("AutoTest: 16 Verify RLoad current");
        setConnectOutVmeter();
        msleep(100);
        setConnectResistor();
        msleep(2000);
        checkEven();
        //VMeter_update_mean_cnt = 5;
        //VMeter_update = false;
        testThread_clear_data(1,5);
        timeoutMs = 0;
        while(!VMeter_update){
            msleep(10);
            timeoutMs+=10;
            if( mExitcmd || !mStartTest ) break;
            if( timeoutMs >= 2000 ) break;
        }
        if( VMeter_update ){
            testRes.RloadCurrent = VMeter_value/100; // A = V/100R
            emit log("VRload="+QString::number(VMeter_value,'f',6)+"; A="+QString::number(testRes.RloadCurrent,'f',6));
            if( testRes.RloadCurrent > RLOAD_MAX_A_LEVEL || testRes.RloadCurrent < RLOAD_MIN_A_LEVEL){
                testRes.errorCode = ERRORCODE::E7;
                testRes.errorCodeString = "Rload current="+QString::number(VMeter_value,'f',4)+",is out of range ";
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
 #endif
    //Power off
        emit log("AutoTest: 17 Power off");
        setPowerOff();
        msleep(poweroffMs);
        checkEven();
    //Normal mode
        emit log("AutoTest: 18 Into normal mode");
        setNormalMode();
        msleep(100);
        setPowerOn();
        //msleep(500);
        checkEven();

    //Verify LED animation
        emit log("AutoTest: 19 Verify LED animation");
        setConnectShaver();
        setLedCaptureStart();
        setVmeterReadStop();
        msleep(100);
        ledAnimationTimeMs = 0;
        mLedAnimationStep = 0;
        mLedAnimationIndex = 0;
        //LedBrightness_update_mean_cnt = 4;
        //LedBrightness_update = false;
        testThread_clear_data(1,1);
        int res;
        int animationTimeoutMs = 4000;
        bool checkRes;

        checkRes = false;
        testRes.LedAnimation = false;
        while(ledAnimationTimeMs < animationTimeoutMs )
        {
            timeoutMs = 0;
            while(!LedBrightness_update){
                msleep(1);
                timeoutMs+=1;
                ledAnimationTimeMs+=1;
                if( mExitcmd || !mStartTest ) break;
                if( timeoutMs >= 1000 ) break;
            }
            if( LedBrightness_update ) {
                res = testLedAnimationLoop();
                LedBrightness_update = false; //!!!!!
                if( -1 ==  res){
                    testRes.errorCode = ERRORCODE::E8;
                    testRes.errorCodeString = tr("连接剃须刀后-LED动态效果测试失败");
                    checkRes = false;
                    break;
                }
                if( 1 == res ){
                    testRes.LedAnimation = true;
                    checkRes = true;
                    break;
                }
            }else{
                checkRes = false;
                break;
            }
            if( mExitcmd || !mStartTest ) break;
            msleep(1);
            ledAnimationTimeMs++;
            //emit log("animation timeout "+QString::number(ledAnimationTimeMs));
        }
        if( !checkRes ){
            setDisconnectShaver();
            setLedCaptureStop();
            if( ledAnimationTimeMs >= animationTimeoutMs || timeoutMs >= 1000 ){
                setAlloff();
                testRes.errorCode = ERRORCODE::E8;
                testRes.errorCodeString = tr("LED动态效果检测超时");
                emit result(testRes);
                continue;
            }else{
                setAlloff();
                emit result(testRes);
                continue;
            }
        }
        setDisconnectShaver();
        setLedCaptureStop();
        msleep(100);
        checkEven();

    //send the result and reset
        emit log("AutoTest: test Finish");
        setPowerOff();
        msleep(poweroffMs);
        setAirCylinderOff();
        setAlloff();
        testThread_reset();
        emit result(testRes);
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
#if 0 //check all mid light
        checkOK = true;
        for( int i=0; i<12; i++ ){
            if( LedBrightness[i] < LED_ANIMATION_MID_MIN_LEVEL || LedBrightness[i] > LED_ANIMATION_MID_MAX_LEVEL) {
                checkOK = false;
            }
        }
        if( checkOK ) {
            emit log("Step1 OK");
            mLedAnimationStep++;
        }
#else
        mLedAnimationStep++;
#endif
        break;
    }
    case 2:{
        //check mid -> full

        if( mLedAnimationIndex == 0xfff) return 1;

        for( int i=0; i< 12; i++){
            if( LedBrightness[i] < LED_ANIMATION_MID_MIN_LEVEL ){
                emit log("Step2 Failed, LED"+QString::number(i+1)+"is too dim:"+QString::number(LedBrightness[i]));
                return -1;
            }
            if( LedBrightness[i] >= LED_ANIMATION_HIGH_LOW_LEVEL ){
                emit log("LED"+QString::number(i+1)+" checked.");
                mLedAnimationIndex |= ( 1 << i);
            }
        }

        if( mLedAnimationIndex == 0xfff ) {
            emit log("Step2 OK");
            return 1;
        }
        break;

    }
    }


    return 0;
}
