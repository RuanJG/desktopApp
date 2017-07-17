#include "excelengine.h"
#include "qt_windows.h"

ExcelEngine::ExcelEngine()
{
    pExcel     = NULL;
    pWorkbooks = NULL;
    pWorkbook  = NULL;
    pWorksheet = NULL;

    sXlsFile     = "";
    nRowCount    = 0;
    nColumnCount = 0;
    nStartRow    = 0;
    nStartColumn = 0;
    mWorkSheetCount = 0;
    mCurrentSheetId = 1;

    bIsOpen     = false;
    bIsValid    = false;
    bIsANewFile = false;
    bIsSaveAlready = false;

    HRESULT r = OleInitialize(0);
    if (r != S_OK && r != S_FALSE)
    {
        qDebug("Qt: Could not initialize OLE (error %x)", (unsigned int)r);
    }
}


ExcelEngine::~ExcelEngine()
{
    if ( bIsOpen )
    {
        //����ǰ���ȱ������ݣ�Ȼ��ر�workbook
        Close();
    }
    OleUninitialize();
}

/**
  *@brief ��sXlsFileָ����excel����,
  *@return true : �򿪳ɹ�
  *        false: ��ʧ��
  */
bool ExcelEngine::Open(unsigned int nSheet, bool visible, QString type)
{

    if ( bIsOpen )
    {
        //return bIsOpen;
        Close();
    }

    mCurrentSheetId = nSheet;
    bIsVisible = visible;

    if ( NULL == pExcel )
    {
        pExcel = new QAxObject(type);
        if ( pExcel )
        {
            bIsValid = true;
        }
        else
        {
            bIsValid = false;
            bIsOpen  = false;
            return bIsOpen;
        }

        pExcel->dynamicCall("SetVisible(bool)", bIsVisible);
    }

    if ( !bIsValid )
    {
        bIsOpen  = false;
        return bIsOpen;
    }

    if ( sXlsFile.isEmpty() )
    {
        bIsOpen  = false;
        return bIsOpen;
    }

    /*���ָ����ļ������ڣ�����Ҫ�½�һ��*/
    QFile f(sXlsFile);
    if (!f.exists())
    {
        bIsANewFile = true;
    }
    else
    {
        bIsANewFile = false;
    }

    if (!bIsANewFile)
    {
        pWorkbooks = pExcel->querySubObject("WorkBooks"); //��ȡ������
        pWorkbook = pWorkbooks->querySubObject("Open(QString, QVariant)",sXlsFile,QVariant(0)); //��xls��Ӧ�Ĺ�����
    }
    else
    {
        pWorkbooks = pExcel->querySubObject("WorkBooks");     //��ȡ������
        pWorkbooks->dynamicCall("Add");                       //���һ���µĹ�����
        pWorkbook  = pExcel->querySubObject("ActiveWorkBook"); //�½�һ��xls
    }

    if( openWorkSheet( mCurrentSheetId ) )
        bIsOpen = true;
    else
        bIsOpen  = false;

    return bIsOpen;
}

/**
  *@brief Open()�����غ���
  */
bool ExcelEngine::Open(QString xlsFile, unsigned int nSheet, bool visible , QString type)
{
    sXlsFile = xlsFile;
    mCurrentSheetId = nSheet;
    bIsVisible = visible;

    return Open(mCurrentSheetId,bIsVisible,type);
}




/**
  *@brief ���������ݣ�������д���ļ�
  */
void ExcelEngine::Save()
{
    if ( IsOpen() )
    {
        //should save every time called
        //if (bIsSaveAlready)
        //{
        //    return ;
        //}

        if (!bIsANewFile)
        {
            pWorkbook->dynamicCall("Save()");
        }
        else /*������ĵ����½������ģ���ʹ�����ΪCOM�ӿ�*/
        {
            //QAxWidget app =
            //pExcel->setProperty("AlertBeforeOverwriting" ,false);
            //pExcel->setProperty("DisplayAlerts",false);

            pWorkbook->dynamicCall("SaveAs(const QString&)", sXlsFile);
            bIsANewFile = false;

            //pWorkbook->dynamicCall("SaveAs (const QString&,int,const QString&,const QString&,bool,bool)",
                      //sXlsFile,56);

        }

        bIsSaveAlready = true;
    }
}

/**
  *@brief �ر�ǰ�ȱ������ݣ�Ȼ��رյ�ǰExcel COM���󣬲��ͷ��ڴ�
  */
void ExcelEngine::Close()
{
    //�ر�ǰ�ȱ�������
    Save();

    if ( IsValid() )
    {
        pWorkbook->dynamicCall("Close(bool)", true);
        pExcel->dynamicCall("Quit()");

        delete pExcel;
        pExcel = NULL;

        bIsOpen     = false;
        bIsValid    = false;
        bIsANewFile = false;
        bIsSaveAlready = true;
    }
}

/**
  *@brief ��tableWidget�е����ݱ��浽excel��
  *@param tableWidget : ָ��GUI�е�tablewidgetָ��
  *@return ����ɹ���� true : �ɹ�
  *                  false: ʧ��
  */
bool ExcelEngine::SaveDataFrTable(QTableWidget *tableWidget)
{
    if ( NULL == tableWidget )
    {
        return false;
    }
    if ( !bIsOpen )
    {
        return false;
    }

    int tableR = tableWidget->rowCount();
    int tableC = tableWidget->columnCount();

    //��ȡ��ͷд����һ��
    for (int i=0; i<tableC; i++)
    {
        if ( tableWidget->horizontalHeaderItem(i) != NULL )
        {
            this->SetCellData(1,i+1,tableWidget->horizontalHeaderItem(i)->text());
        }
    }

    //д����
    for (int i=0; i<tableR; i++)
    {
        for (int j=0; j<tableC; j++)
        {
            if ( tableWidget->item(i,j) != NULL )
            {
                this->SetCellData(i+2,j+1,tableWidget->item(i,j)->text());
            }
        }
    }

    //����
    Save();

    return true;
}

/**
  *@brief ��ָ����xls�ļ��а����ݵ��뵽tableWidget��
  *@param tableWidget : ִ��Ҫ���뵽��tablewidgetָ��
  *@return ����ɹ���� true : �ɹ�
  *                   false: ʧ��
  */
bool ExcelEngine::ReadDataToTable(QTableWidget *tableWidget)
{
    if ( NULL == tableWidget )
    {
        return false;
    }

    //�Ȱ�table���������
    int tableColumn = tableWidget->columnCount();
    tableWidget->clear();
    for (int n=0; n<tableColumn; n++)
    {
        tableWidget->removeColumn(0);
    }

    int rowcnt    = nStartRow + nRowCount;
    int columncnt = nStartColumn + nColumnCount;

    //��ȡexcel�еĵ�һ��������Ϊ��ͷ
    QStringList headerList;
    for (int n = nStartColumn; n<columncnt; n++ )
    {
        QAxObject * cell = pWorksheet->querySubObject("Cells(int,int)",nStartRow, n);
        if ( cell )
        {
            headerList<<cell->dynamicCall("Value2()").toString();
        }
    }

    //���´�����ͷ
    tableWidget->setColumnCount(nColumnCount);
    tableWidget->setHorizontalHeaderLabels(headerList);


    //����������
    for (int i = nStartRow+1, r = 0; i < rowcnt; i++, r++ )  //��
    {
        tableWidget->insertRow(r); //��������
        for (int j = nStartColumn, c = 0; j < columncnt; j++, c++ )  //��
        {
            QAxObject * cell = pWorksheet->querySubObject("Cells(int,int)", i, j );//��ȡ��Ԫ��

            //��r�����������������
            if ( cell )
            {
                tableWidget->setItem(r,c,new QTableWidgetItem(cell->dynamicCall("Value2()").toString()));
            }
        }
    }

    return true;
}

/**
  *@brief ��ȡָ����Ԫ�������
  *@param row : ��Ԫ����к�
  *@param column : ��Ԫ����к�
  *@return [row,column]��Ԫ���Ӧ������
  */
QVariant ExcelEngine::GetCellData(unsigned int row, unsigned int column)
{
    QVariant data;

    if( ! IsOpen() )
        return false;

    QAxObject *cell = pWorksheet->querySubObject("Cells(int,int)",row,column);//��ȡ��Ԫ�����
    if ( cell )
    {
        data = cell->dynamicCall("Value2()");
    }

    return data;
}

/**
  *@brief �޸�ָ����Ԫ�������
  *@param row : ��Ԫ����к�
  *@param column : ��Ԫ��ָ�����к�
  *@param data : ��Ԫ��Ҫ�޸�Ϊ��������
  *@return �޸��Ƿ�ɹ� true : �ɹ�
  *                   false: ʧ��
  */
bool ExcelEngine::SetCellData(unsigned int row, unsigned int column, QVariant data)
{
    bool op = false;

    if( ! IsOpen() )
        return false;

    QAxObject *cell = pWorksheet->querySubObject("Cells(int,int)",row,column);//��ȡ��Ԫ�����
    if ( cell )
    {
        QString strData = data.toString(); //excel ��Ȼֻ�ܲ����ַ��������ͣ��������޷�����
        cell->dynamicCall("SetValue(const QVariant&)",strData); //�޸ĵ�Ԫ�������
        //cell->setProperty("HoriontalAligment", -4108);    // ����루xlLeft����-4131 �����У�xlCenter����-4108 ���Ҷ��루xlRight����-4152
        cell->setProperty("ColumnWidth", 15);
        op = true;
    }
    else
    {
        op = false;
    }

    return op;
}


bool ExcelEngine::setCellBackgroundColor(unsigned int row, unsigned int column, QColor color)
{
    if( ! IsOpen() )
        return false;

    QAxObject *cell = pWorksheet->querySubObject("Cells(int,int)",row,column);//��ȡ��Ԫ�����
    if ( cell )
    {
        QAxObject* interior = cell->querySubObject("Interior");
        interior->setProperty("Color", color);   //���õ�Ԫ�񱳾�ɫ
        return true;
    }

    return false;
}

bool ExcelEngine::setCellFontColor(unsigned int row, unsigned int column, QColor color)
{
    if( ! IsOpen() )
        return false;

    QAxObject *cell = pWorksheet->querySubObject("Cells(int,int)",row,column);//��ȡ��Ԫ�����
    if ( cell )
    {
        QAxObject *font = cell->querySubObject("Font");  //��ȡ��Ԫ������
        font->setProperty("Color", color);  //���õ�Ԫ��������ɫ����ɫ��
        /*
         * QAxObject* border = cell->querySubObject("Borders");
        border->setProperty("Color", QColor(0, 0, 255));   //���õ�Ԫ��߿�ɫ����ɫ��
        */
        return true;
    }

    return false;
}

bool ExcelEngine::openWorkSheet( unsigned int id )
{

    if( !pWorkbook  )
        return false;

    QAxObject *worksheets = pWorkbook->querySubObject("WorkSheets");  //SheetsҲ�ɻ���WorkSheets
    mWorkSheetCount = worksheets->property("Count").toInt();

    if( id > mWorkSheetCount )
        return false;

    pWorksheet = pWorkbook->querySubObject("Sheets(int)", id);
    if( NULL ==  pWorksheet ){
        bIsOpen = false;
        return false;
    }

    //�����Ѵ򿪣���ʼ��ȡ��Ӧ����
    QAxObject *usedrange = pWorksheet->querySubObject("UsedRange");//��ȡ��sheet��ʹ�÷�Χ����
    QAxObject *rows = usedrange->querySubObject("Rows");
    QAxObject *columns = usedrange->querySubObject("Columns");

    //��Ϊexcel���Դ��������������ݶ���һ���Ǵ�0,0��ʼ�����Ҫ��ȡ�������±�
    nStartRow    = usedrange->property("Row").toInt();    //��һ�е���ʼλ��
    nStartColumn = usedrange->property("Column").toInt(); //��һ�е���ʼλ��

    nRowCount    = rows->property("Count").toInt();       //��ȡ����
    nColumnCount = columns->property("Count").toInt();    //��ȡ����

    return true;
}



/**
  *@brief �ж�excel�Ƿ��ѱ���
  *@return true : �Ѵ�
  *        false: δ��
  */
bool ExcelEngine::IsOpen()
{
    return bIsOpen;
}

/**
  *@brief �ж�excel COM�����Ƿ���óɹ���excel�Ƿ����
  *@return true : ����
  *        false: ������
  */
bool ExcelEngine::IsValid()
{
    return bIsValid;
}

/**
  *@brief ��ȡexcel������
  */
unsigned int ExcelEngine::GetRowCount()const
{
    return nRowCount;
}

unsigned int ExcelEngine::getStartRow()
{
    return nStartRow;
}

/**
  *@brief ��ȡexcel������
  */
unsigned int ExcelEngine::GetColumnCount()const
{
    return nColumnCount;
}

unsigned int ExcelEngine::getStartColumn()
{
    return nStartColumn;
}


unsigned int ExcelEngine::getCurrentSheetId()
{
    return mCurrentSheetId;
}

unsigned int ExcelEngine::getSheetCount()
{
    return mWorkSheetCount;
}
