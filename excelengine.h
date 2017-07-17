#ifndef EXCELENGINE_H
#define EXCELENGINE_H

#include <QObject>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QAxBase>
#include <QAxObject>
#include <QAxWidget>

#include <QTableWidget>
#include <QTableView>
#include <QTableWidgetItem>
#include <QDebug>


// the execl app type
#define TYPE_EXECL_APP_MS   "Excel.Application"
#define TYPE_EXECL_APP_WPS  "ET.Application"

/**
  *@brief 这是一个便于Qt读写excel封装的类，同时，便于把excel中的数据
  *显示到界面上，或者把界面上的数据写入excel中，同GUI进行交互，关系如下：
  *Qt tableWidget <--> ExcelEngine <--> xls file.
  *
  *@note ExcelEngine类只负责读/写数据，不负责解析，做中间层
  *@author yaoboyuan 254200341@qq.com
  *@date 2012-4-12
  */
class ExcelEngine : protected QObject
{
public:
    ExcelEngine();
    ~ExcelEngine();

private:
    bool Open(unsigned int nSheet, bool visible, QString type);
public:
    bool Open(QString xlsFile, unsigned int nSheet = 1, bool visible = false, QString type=TYPE_EXECL_APP_MS);
    void Save();                //保存xls报表
    void Close();               //关闭xls报表

    bool SaveDataFrTable(QTableWidget *tableWidget); //保存数据到xls
    bool ReadDataToTable(QTableWidget *tableWidget); //从xls读取数据到ui

    QVariant GetCellData(unsigned int row, unsigned int column);                //获取指定单元数据
    bool     SetCellData(unsigned int row, unsigned int column, QVariant data); //修改指定单元数据

    unsigned int GetRowCount()const;
    unsigned int GetColumnCount()const;

    bool IsOpen();
    bool IsValid();

    unsigned int getStartRow();
    unsigned int getStartColumn();
    unsigned int getCurrentSheetId();
    unsigned int getSheetCount();
    bool setCellBackgroundColor(unsigned int row, unsigned int column, QColor color);
    bool setCellFontColor(unsigned int row, unsigned int column, QColor color);
    bool openWorkSheet(unsigned int id);



private:
    QString   sXlsFile;     //xls文件路径
    bool      bIsOpen;      //是否已打开
    bool      bIsValid;     //是否有效
    bool      bIsANewFile;  //是否是一个新建xls文件，用来区分打开的excel是已存在文件还是有本类新建的
    bool      bIsSaveAlready;//防止重复保存

    QAxObject *pExcel;      //指向整个excel应用程序
    QAxObject *pWorkbooks;  //指向工作簿集,excel有很多工作簿
    QAxObject *pWorkbook;   //指向sXlsFile对应的工作簿
    bool      bIsVisible;   //excel是否可见

    QAxObject *pWorksheet;  //指向工作簿中的某个sheet表单
    unsigned int      mCurrentSheetId;   //当前打开的第几个sheet
    unsigned int mWorkSheetCount;
    unsigned int       nRowCount;    //行数
    unsigned int       nColumnCount; //列数
    unsigned int       nStartRow;    //开始有数据的行下标值
    unsigned int       nStartColumn; //开始有数据的列下标值


};



/*
    ExcelEngine excel; //创建excl对象
    excel.Open(QObject::tr("c:\\Test.xls"),1,false); //打开指定的xls文件的指定sheet，且指定是否可见

    int num = 0;
    for (int i=1; i<=10; i++)
    {
        for (int j=1; j<=10; j++)
        {
           excel.SetCellData(i,j,++num); //修改指定单元数据
        }
    }

    QVarient data = excel.GetCellData(1,1); //访问指定单元格数据
    excel.GetCellData(2,2);
    excel.GetCellData(3,3);
    excel.Save(); //保存
    excel.Close();
*/

//导入数据到tablewidget中
/*
    ExcelEngine excel(QObject::tr("c:\\Import.xls"));
    excel.Open();
    excel.ReadDataToTable(ui->tableWidget); //导入到widget中
    excel.Close();
*/

//把tablewidget中的数据导出到excel中
/*
    ExcelEngine excel(QObject::tr("c:\\Export.xls"));
    excel.Open();
    excel.SaveDataFrTable(ui->tableWidget); //导出报表
    excel.Close();
*/



/*
 *
 *
 * 名称                                    值 描述
xlAddIn                                 18 Microsoft Office Excel 加载项
xlAddIn8                                18 Excel 2007 加载项
xlCSV                                    6 CSV
xlCSVMac                                22 Macintosh CSV
xlCSVMSDOS                              24 MSDOS CSV
xlCSVWindows                            23 Windows CSV
xlCurrentPlatformText                -4158 当前平台文本
xlDBF2                                   7 DBF2
xlDBF3                                   8 DBF3
xlDBF4                                  11 DBF4
xlDIF                                    9 DIF
xlExcel12                               50 Excel 12
xlExcel2                                16 Excel 2
xlExcel2FarEast                         27 Excel2 FarEast
xlExcel3                                29 Excel3
xlExcel4                                33 Excel4
xlExcel4Workbook                        35 Excel4 工作簿
xlExcel5                                39 Excel5
xlExcel7                                39 Excel7
xlExcel8                                56 Excel8
xlExcel9795                             43 Excel9795
xlHtml                                  44 HTML 格式
xlIntlAddIn                             26 国际加载项
xlIntlMacro                             25 国际宏
xlOpenXMLAddIn                          55 打开 XML 加载项
xlOpenXMLTemplate                       54 打开 XML 模板
xlOpenXMLTemplateMacroEnabled           53 打开启用的 XML 模板宏
xlOpenXMLWorkbook                       51 打开 XML 工作簿
xlOpenXMLWorkbookMacroEnabled           52 打开启用的 XML 工作簿宏
xlSYLK                                   2 SYLK
xlTemplate                              17 模板
xlTemplate8                             17 模板 8
xlTextMac                               19 Macintosh 文本
xlTextMSDOS                             21 MSDOS 文本
xlTextPrinter                           36 打印机文本
xlTextWindows                           20 Windows 文本
xlUnicodeText                           42 Unicode 文本
xlWebArchive                            45 Web 档案
xlWJ2WD1                                14 WJ2WD1
xlWJ3                                   40 WJ3
xlWJ3FJ3                                41 WJ3FJ3
xlWK1                                    5 WK1
xlWK1ALL                                31 WK1ALL
xlWK1FMT                                30 WK1FMT
xlWK3                                   15 WK3
xlWK3FM3                                32 WK3FM3
xlWK4                                   38 WK4
xlWKS                                    4 工作表
xlWorkbookDefault                       51 默认工作簿
xlWorkbookNormal                     -4143 常规工作簿
xlWorks2FarEast                         28 Works2 FarEast
xlWQ1                                   34 WQ1
xlXMLSpreadsheet                        46 XML 电子表格
 *
 * */


#endif // EXCELENGINE_H
