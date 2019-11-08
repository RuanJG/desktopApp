#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QtSerialPort>
#include <QMutex>
#include "libs/uartcoder.h"
#include <libs/chunk.h>
#include <iostream>
#include "libs/iapmaster.h"
#include <QTimer>
#include <QThread>
#include "testerthread.h"
#include <QSettings>
#include <QFile>
#include <QMap>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


signals:
    void leftDebug_step(bool start);
    void leftUpdate_tester_data(int tag, unsigned char *data);
    void leftTestThread_start(bool start);
    void leftTestThread_exit();
    void rightDebug_step(bool start);
    void rightTestThread_start(bool start);
    void rightTestThread_exit();
    void rightUpdate_tester_data(int tag, unsigned char *data);

private slots:
    void leftTesterThread_sendSerialCmd(int id, unsigned char *data, int len);
    void leftTesterThread_log(QString str);
    void leftTesterThread_result(TesterRecord res);
    void leftTesterThread_error(QString errorStr);

    void rightTesterThread_sendSerialCmd(int id, unsigned char *data, int len);
    void rightTesterThread_log(QString str);
    void rightTesterThread_result(TesterRecord res);
    void rightTesterThread_error(QString errorStr);

    void on_serialconnectPushButton_clicked();

    void on_serialrescanPushButton_2_clicked();

    void on_NormalradioButton_toggled(bool checked);

    void on_TestMode2radioButton_2_toggled(bool checked);

    void solt_mSerial_ReadReady();

    void solt_mSerial_Right_ReadReady();

    void on_TestMode3radioButton_3_toggled(bool checked);

    void on_Testmode4radioButton_4_toggled(bool checked);

    void on_V15PoweroncheckBox_2_clicked(bool checked);

    void on_outputtoShavercheckBox_clicked(bool checked);

    void on_outputtoResistorcheckBox_3_clicked(bool checked);

    void on_outputtoVmetercheckBox_4_clicked(bool checked);

    void on_measuretoVmetercheckBox_5_clicked(bool checked);

    void on_airpresscheckBox_clicked(bool checked);

    void on_meausureVDDcheckBox_6_clicked(bool checked);

    void on_measureLEDcheckBox_7_clicked(bool checked);

    void on_LedCapturecheckBox_3_clicked(bool checked);

    void on_measuretoVmetercheckBox_5_clicked();

    void solt_mScanerSerial_ReadReady();

    void solt_mScanerSerial_Right_ReadReady();

    void on_scanerStartpushButton_clicked();

    void on_VmeterStartcheckBox_clicked(bool checked);

    void on_DebugpushButton_clicked();

    void on_startTestpushButton_clicked();

    void on_AutoTestClearLogpushButton_clicked();

    void on_ClearLogpushButton_2_clicked();

    void on_debugModeCheckBox_clicked(bool checked);

    void on_serialrescanPushButton_right_clicked();

    void on_serialconnectPushButton_right_clicked();

private:
    Ui::MainWindow *ui;

    typedef struct TestTargetControler{
        QSerialPort *Scanerserialport;
        QByteArray QRcodeBytes;
        QSerialPort *Serialport;
        QMutex SerialportMutex;
        UartCoder Decoder;
        UartCoder Encoder;
        volatile int RelayStatus;
        TesterThread *TesterThread;
        QString QRcode;
        QString id;
    }TestTargetControler_t;

    TestTargetControler_t mLeftTester;
    TestTargetControler_t mRightTester;

    volatile int mLedBrightness_update;

    QFile mTxtfile;
    QMutex mDataMutex;
    QSettings mSetting;
    QMap<QString,quint32> mSerialMap;
    int mRelayStatus;

    void update_serial_info();
    bool saveDataToFile(TesterRecord res);
    bool isLeftTester(TestTargetControler_t *tester);
    bool open_serial(TestTargetControler_t *tester);
    void close_serial(TestTargetControler_t *tester);
    void handle_Serial_Data(TestTargetControler_t *tester, QByteArray &bytes);
    void serial_send_packget(TestTargetControler_t *tester, const Chunk &chunk);
    void scaner_send_cmd(TestTargetControler_t *tester, unsigned char *data, int len);
    void sendcmd(int tag, int data);
    bool isTesterValiable(TestTargetControler_t *tester);
    void serial_send_PMSG(TestTargetControler_t *tester, unsigned char tag, unsigned char *data, int data_len);
};

#endif // MAINWINDOW_H
