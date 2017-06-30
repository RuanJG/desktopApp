#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QtSerialPort>
#include <QMutex>
#include <uartcoder.h>
#include <chunk.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);

private slots:
    void on_save_close_Button_clicked();

    void on_pushButton_3_clicked();

    void on_SerialButton_clicked();

    void on_Serial_ReadReady();

    void on_setcurrentButton_clicked();

    void on_setVolumeButton_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *mSerialport;
    QMutex mSerialMutex;
    UartCoder mCoder;

    void update_serial_info();
    void close_serial();
    bool open_serial();
    bool open_serial_dev(QString portname);
    void handle_Serial_Data(QByteArray &bytes);
};

#endif // MAINWINDOW_H
