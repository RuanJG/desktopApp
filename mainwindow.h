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


//LED to PC
#define PC_TAG_DATA_LED_BRIGHTNESS (PMSG_TAG_DATA|0x1)
//Control to PC
#define PC_TAG_DATA_VMETER (PMSG_TAG_DATA|0x2)
#define PC_TAG_DATA_AMETER (PMSG_TAG_DATA|0x3)
#define PC_TAG_DATA_BUTTON (PMSG_TAG_DATA|0x4) // startbutton data[0] = 1 pressed


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_serialconnectPushButton_clicked();

    void on_serialrescanPushButton_2_clicked();

    void on_NormalradioButton_toggled(bool checked);

    void on_TestMode2radioButton_2_toggled(bool checked);

    void solt_mSerial_ReadReady();
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
    void on_scanerStartpushButton_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *mSerialport;
    QSerialPort *mScanerserialport;
    QMutex mSerialMutex;
    UartCoder mDecoder;
    UartCoder mEncoder;
    QTimer  mTimer;
    int mRelayStatus;
    QByteArray QRcodeBytes;
    void update_serial_info();
    bool open_serial();
    void close_serial();
    void handle_Serial_Data(QByteArray &bytes);
    void serial_send_packget(const Chunk &chunk);
    void serial_send_PMSG(unsigned char tag, unsigned char *data, int data_len);
    void scaner_send_cmd(unsigned char *data, int len);
};

#endif // MAINWINDOW_H
