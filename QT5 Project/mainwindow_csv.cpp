/*
**  CSV to QTableView loader
**
**
**
**
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tablemodel.h"

void MainWindow::loadListCSV(QString sFilename)
{
    //Open csv file from resources
    //QFile csvfile(":/ModbusTC100.csv");
    QFile csvfile(sFilename);
    qDebug() << "Loading" << sFilename;
    if (!csvfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "can't open CSV data file";
        return;
        }
    //Stream file to listCSV
    QTextStream csvStream(&csvfile);
    QList<QStringList> listCSV;
    QStringList listHeaderCSV;
    int  lineNo = 0;
    while(!csvStream.atEnd()) {
    QString line = csvStream.readLine().simplified();
    qDebug() << lineNo << line;
    if (!line.isEmpty() && !line.startsWith(";") ) {
            QStringList rowList = line.split(',');
            if ( lineNo > 0 )
                listCSV.append(rowList);
            else
                listHeaderCSV = rowList;
            }
        ++lineNo;
        }
    //csvStream.seek(0);
    csvfile.close();

    //qDebug() << "header" << listHeaderCSV;
    //qDebug() << "csv" << listCSV;
    //qDebug() << "data>>>" << listCSV[3][1];

    //Setup tableView UI
    QTableView *ptvModbus = ui->tableViewModbus;
    ptvModbus->setAttribute(Qt::WA_DeleteOnClose);
    ptvModbus->setSelectionBehavior(QAbstractItemView::SelectRows);
    ptvModbus->setAlternatingRowColors(true);
    ptvModbus->horizontalHeader()->setStyleSheet("QHeaderView{font-size:12pt;font-weight:bold;}");
    ptvModbus->horizontalHeader()->setStretchLastSection(true);
    ptvModbus->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter); //last colum alignCenter

    ptvModbus->horizontalHeader()->setVisible(true);
    ptvModbus->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //ptvModbus->verticalHeader()->setVisible(false);
    QHeaderView *verticalHeader = ptvModbus->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(24);

    //Setup model
    //TableModel *csvmodel = new TableModel(listCSV, listHeaderCSV, ptvModbus);
    pModelCSV = new TableModel(listCSV, listHeaderCSV, ptvModbus);

    ptvModbus->setModel(pModelCSV);
    //Show UI
    ptvModbus->selectRow(0);
    ptvModbus->show();
    ui->groupBoxModbus->setTitle(csvfile.fileName());

    //ReConnect doubleclick to lamda function
    QObject::disconnect(ptvModbus, &QTableView::doubleClicked, nullptr, nullptr);
    QObject::connect(ptvModbus, &QTableView::doubleClicked, [=]() {
        QItemSelectionModel* selmodel = ptvModbus->selectionModel();
        QModelIndex current = selmodel->currentIndex(); // the "current" item
        QModelIndexList selected = selmodel->selectedIndexes(); // list of "selected" items

        //Toggle ActRun
        if (current.column() == enumModbusCSV::eActRun)
           ptvModbus->model()->setData(current, current.data().toBool() ? QVariant::fromValue(0) : QVariant::fromValue(1) ); //toggle
        else {
            bool ok;
            QString sData="";
            //for (int i=0; i< selected.count();i++)
            //    sData += "|"+selected[i].data().toString();
            int iServerAddr = ui->serverEdit->value();
            int iRegAddr= selected[enumModbusCSV::eReg].data().toString().toInt(&ok,16);
            quint16  iCount = selected[enumModbusCSV::eCount].data().toUInt();
            QString sRW = selected[enumModbusCSV::eRW].data().toString();

            /*
            int iValue  = selected[enumModbusCSV::eValue].data().toString().toInt(&ok,16);
            _DataUnit = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, iRegAddr, iCount);
            if (iCount==1)
                _DataUnit.setValue(0, iValue);
            else{
                _DataUnit.setValue(0, iValue>>16);
                _DataUnit.setValue(1, iValue&0x0FFFF);
                }*/

            int iFC = 0;
            if (sRW.contains("Rc",Qt::CaseInsensitive)) iFC = 0x01;
            if (sRW.contains("Wc",Qt::CaseInsensitive)) iFC = 0x05;
            if (sRW.contains("Rr",Qt::CaseInsensitive)) iFC = 0x03;
            if (sRW.contains("Wr",Qt::CaseInsensitive) && iCount==1) iFC = 0x06;
            if (sRW.contains("Wr",Qt::CaseInsensitive) && iCount>1) iFC = 0x10;
            //target fcode regAddr value
            char buf[64];
            int iValue;
            switch (iFC) {
                default:
                case 0x01: //read coil
                    sprintf(buf, "%02X %02X %04X %04X", iServerAddr, iFC, iRegAddr, iCount);
                    break;
                case 0x03: //read reg
                    sprintf(buf, "%02X %02X %04X %04X", iServerAddr, iFC, iRegAddr, iCount);
                    break;
                case 0x05: //write coil
                    iValue  = selected[enumModbusCSV::eValue].data().toString().toInt(&ok,16);
                    sprintf(buf, "%02X %02X %04X %04X %04X", iServerAddr, iFC, iRegAddr, iCount, iValue);
                    break;
                case 0x06: //write reg
                    iValue  = selected[enumModbusCSV::eValue].data().toString().toInt(&ok,16);
                    sprintf(buf, "%02X %02X %04X %04X %04X", iServerAddr, iFC, iRegAddr, iCount, iValue);
                    break;
                case 0x10: //write multiple regs
                    //addr, fc, reg, count, bytes, data...
                    //sprintf(buf, "%02X %02X %04X %04X %02X %04X%04X", iServerAddr, iFC, iRegAddr, iCount, iCount*2, _DataUnit.value(0), _DataUnit.value(1));
                    QStringList slData = selected[enumModbusCSV::eValue].data().toString().split(QRegExp("[ ,;]"), QString::SkipEmptyParts);
                    qDebug() << "Data:" << slData << iCount;
                    _DataUnit = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, iRegAddr, iCount);
assert(iCount==slData.size());
                    char bufData[256];
                    QString sData;
                    for (int i=0;i<slData.size();i++) {
                        int iValue  = slData[i].toInt(&ok,16);
                        sprintf(bufData, "%04X ", iValue);
                        sData += QString(bufData);
                        _DataUnit.setValue(i, static_cast<quint16>(iValue));
                        }
                    sprintf(buf, "%02X %02X %04X %04X %02X %s", iServerAddr, iFC, iRegAddr, iCount, iCount*2, sData.toStdString().c_str());
                    break;
                }
            ui->lineEditModbusData->setText(QString(buf));
            ui->btnSend->setEnabled(true);
            }
        qDebug() << "current item = " << current.data();

        QString sData="row items =";
        for (int i=0; i< selected.count();i++)
            sData += "|"+selected[i].data().toString();
        qDebug() << sData+"|";
        });

}
