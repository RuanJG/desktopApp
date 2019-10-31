#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>
#include <QStringList>
#include <QList>
#include <QFile>
#include <QMessageBox>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include "databasehelper.h"



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


    bool eventFilter(QObject *watched, QEvent *event);

public slots:
    void slot_Start_Packing();
    void slot_Start_connectDB();

private slots:
    void on_reducepushButton_clicked();

    void on_AddpushButton_2_clicked();

    void on_exportpushButton_clicked();

    void on_dbFileChoisepushButton_2_clicked();

    void on_dbImportpushButton_clicked();

    void on_boxlistfilechoisepushButton_3_clicked();

    void on_boxlisttxtfileimportpushButton_4_clicked();

    void on_boxcheckpushButton_clicked();

    void on_testDBpushButton_clicked();

    void on_connectRemoteDBpushButton_2_clicked();

    void on_connectLocalDBpushButton_clicked();

    void on_mergerLocaldbpushButton_clicked();

    void on_localModecheckBox_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    volatile int mStepIndex;
    DataBaseHelper mDataBaseHelper;
    UnitsBox mCurrentBox;
    QByteArray mStringData;
    QMessageBox mMsgbox;
    volatile bool mUsingLocalDB;
    void updateTable();
    void updateStep();
    void testDatabase(DataBaseHelper *db);
    bool openRemoteDataBase(DataBaseHelper *mdber);
    bool openLocalDataBase(DataBaseHelper *mdber);
    void updateDBConnectStatus();
    bool isUsingLocalDB(DataBaseHelper *mdber);
    bool mergeLocalDBToRemote(QString localDBFileNmae);
    QString mLocalDataBaseFileName;
};










#endif // MAINWINDOW_H



