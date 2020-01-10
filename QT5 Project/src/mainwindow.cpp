/****************************************************************************
**
** jcModbusClient/Master
**
**  supports Modbus fcode
**  0x03(read single reg) 0x06(write single reg) 0x10(write multiple regsx2)
**
**  validated with IAI/Toyo Modbus PLC
**
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "writeregistermodel.h"

#include <QModbusTcpClient>
#include <QModbusRtuSerialMaster>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QUrl>
#include <QLoggingCategory>
#include "settingsdialog.h"

enum ModbusConnection {
    eModbusSerial,
    eModbusUSB,
    eModbusTcp
};

extern uint32_t rpiSerial();

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , lastRequest(nullptr)
    , modbusDevice(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("Modbus Master/Client");

    m_settingsDialog = new SettingsDialog(this);

    initActions();

    on_connectType_currentIndexChanged(eModbusSerial);
    //Iterate all files in app directory
    QDirIterator it(".", QDirIterator::NoIteratorFlags);
    QStringList slFiles;
    while (it.hasNext()) {
        QString file = it.next();
        if (file.contains(".csv"))
            slFiles.append(file);
        }
    slFiles.sort(); //Sorting order
    qDebug() << "csv files:" << slFiles;
    ui->cbCmdFile->clear();
    ui->cbCmdFile->addItems(slFiles);
    ui->cbCmdFile->setCurrentIndex(0);

    loadListCSV(ui->cbCmdFile->currentText());
#if (defined(__arm__))
    qDebug() << "RPi Serial:" << QString::number(rpiSerial(),16);
#endif
}

MainWindow::~MainWindow()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
    delete modbusDevice;

    delete ui;
}

void MainWindow::initActions()
{
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionExit->setEnabled(true);
    ui->actionOptions->setEnabled(true);

    connect(ui->actionConnect, &QAction::triggered,
            this, &MainWindow::on_connectButton_clicked);
    connect(ui->actionDisconnect, &QAction::triggered,
            this, &MainWindow::on_connectButton_clicked);

    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionOptions, &QAction::triggered, m_settingsDialog, &QDialog::show);

    connect(this, SIGNAL(sigModbusCmd(int,int,int)), this, SLOT(slotModbusCmd(int,int,int)) );
    connect(this, SIGNAL(sigModbusRegRead(int, quint16)), this, SLOT(slotModbusRegRead(int, quint16)) );
    connect(this, SIGNAL(sigModbusRegsWrite(int, QVector<quint16>)), this, SLOT(slotModbusRegsWrite(int, QVector<quint16>))) ;
    connect(this, SIGNAL(sigModbusCoilRead(int, quint16)), this, SLOT(slotModbusCoilRead(int, quint16)) );
    connect(this, SIGNAL(sigModbusCoilWrite(int, QVector<quint16>)), this, SLOT(slotModbusCoilWrite(int, QVector<quint16>))) ;

}

void MainWindow::on_connectType_currentIndexChanged(int index)
{
    qDebug() << __FUNCTION__ << index;

    if (modbusDevice) {
        modbusDevice->disconnectDevice();
        delete modbusDevice;
        modbusDevice = nullptr;
        }

    auto type = static_cast<ModbusConnection> (index);
    if (type == eModbusSerial) {
        modbusDevice = new QModbusRtuSerialMaster(this);
        //rescan serial port list
        const auto infos = QSerialPortInfo::availablePorts();
        if (infos.length() == 0) {
            qDebug() << "No Ports available!";
            return;
            }
        //ui->serialPortComboBox->clear();
        //for (const QSerialPortInfo &info : infos)
        //    ui->serialPortComboBox->addItem(info.portName());

        qDebug() << ui->connectType->currentText();
        ui->portEdit->setText(QLatin1Literal(default_serialport));

        ui->labelPort->setText("RPi Serial RTU");
    } else if (type == eModbusUSB) { //USB to Serial
        modbusDevice = new QModbusRtuSerialMaster(this);
        //rescan serial port list
        const auto infos = QSerialPortInfo::availablePorts();
        if (infos.length() == 0) {
            qDebug() << "No Ports available!";
            return;
            }
        //ui->serialPortComboBox->clear();
        //for (const QSerialPortInfo &info : infos)
        //    ui->serialPortComboBox->addItem(info.portName());

        qDebug() << ui->connectType->currentText();
        ui->portEdit->setText(QLatin1Literal(default_USBport));

        ui->labelPort->setText("RPi USB2Serial RTU");

    } else if (type == eModbusTcp) {
        modbusDevice = new QModbusTcpClient(this);
        //if (ui->portEdit->text().isEmpty())
        qDebug() << ui->connectType->currentText();
        ui->portEdit->setText(QLatin1Literal(default_modebus_ip));
        ui->labelPort->setText("Generic TCP Modbus");
    }

    connect(modbusDevice, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) {
        statusBar()->showMessage(modbusDevice->errorString(), 5000);
        });

    if (!modbusDevice) {
        ui->connectButton->setDisabled(true);
        if (type == eModbusSerial)
            statusBar()->showMessage(tr("Could not create Modbus RTU"), 5000);
        else
            statusBar()->showMessage(tr("Could not create Modbus TCP"), 5000);
    } else {
        connect(modbusDevice, &QModbusClient::stateChanged,
                this, &MainWindow::onStateChanged);
        }
}

void MainWindow::on_connectButton_clicked()
{
    if (!modbusDevice)
        return;

    statusBar()->clearMessage();
    if (modbusDevice->state() != QModbusDevice::ConnectedState) {
        qDebug() << __FUNCTION__ << ui->portEdit->text();
        qDebug() << "("<<m_settingsDialog->settings().parity << m_settingsDialog->settings().baud << m_settingsDialog->settings().dataBits << m_settingsDialog->settings().stopBits << ")";

        if ( (static_cast<ModbusConnection> (ui->connectType->currentIndex()) == eModbusSerial) ||
             (static_cast<ModbusConnection> (ui->connectType->currentIndex()) == eModbusUSB) ){
            modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,ui->portEdit->text());
            modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,  m_settingsDialog->settings().parity);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,m_settingsDialog->settings().baud);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,m_settingsDialog->settings().dataBits);
            modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,m_settingsDialog->settings().stopBits);
        } else {
            const QUrl url = QUrl::fromUserInput(ui->portEdit->text());
            modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, url.port());
            modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, url.host());
            }
        modbusDevice->setTimeout(m_settingsDialog->settings().responseTime);
        modbusDevice->setNumberOfRetries(m_settingsDialog->settings().numberOfRetries);
        qDebug() << m_settingsDialog->settings().responseTime << m_settingsDialog->settings().numberOfRetries;
        if (!modbusDevice->connectDevice()) {
            statusBar()->showMessage(tr("Connect failed: ") + modbusDevice->errorString(), 5000);
        } else {
            ui->actionConnect->setEnabled(false);
            ui->actionDisconnect->setEnabled(true);
            }
    } else {
        modbusDevice->disconnectDevice();
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
        }
}

void MainWindow::onStateChanged(int state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);
    ui->actionConnect->setEnabled(!connected);
    ui->actionDisconnect->setEnabled(connected);

    if (state == QModbusDevice::UnconnectedState) {
        ui->connectButton->setText(tr("Connect"));
        ui->plainTextConsole->setEnabled(false);
        ui->btnRun->setEnabled(false);
        }
    else if (state == QModbusDevice::ConnectedState) {
        ui->connectButton->setText(tr("Disconnect"));
        ui->plainTextConsole->setEnabled(true);
        ui->btnRun->setEnabled(true);
        }
}

void MainWindow::readReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;
qDebug() << __FUNCTION__ << reply->rawResult();
    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        int iCounts = static_cast<int>(unit.valueCount());
        char buf[32];
        sprintf(buf, "<0x%04X:|", unit.startAddress());
        QString sData = QString(buf);
        QString sString="(";
        for (int i = 0; i < iCounts; i++) {
            sprintf(buf, "%04X|",  unit.value(i));
            sData += QString(buf);
            char c1 =  (unit.value(i)&0xFF00)>>8;
            char c2 =  (unit.value(i)&0x00FF);
            sString += isprint(c1)?QString(QChar::fromLatin1(c1)):".";
            sString += isprint(c2)?QString(QChar::fromLatin1(c2)):".";
            }
        sString += ")";

        QTextCharFormat tf;
        tf = ui->plainTextConsole->currentCharFormat();
        tf.setForeground(QBrush(QColor("blue")));
        ui->plainTextConsole->setCurrentCharFormat(tf);

        ui->plainTextConsole->appendPlainText(sData+sString);
        ui->lineEditModbusData->setText(sData);

        tf.setForeground(QBrush(QColor("black")));
        ui->plainTextConsole->setCurrentCharFormat(tf);

        ui->btnSend->setEnabled(false);
        mModbusErr = 0;
        mModbusExcept = 0;
        bModbusReplyOK = true;
            /*
            //update UI upon replied address' value
            int rows= ui->tableViewModbus->model()->rowCount();
            for (int r = 0; r< rows; r++) {
                int reg = ui->tableViewModbus->model()->index(r, enumModbusCSV::eReg).data().toInt();
                if ( (unit.startAddress()+i) == reg) {
                   QModelIndex index = ui->tableViewModbus->model()->index(r, enumModbusCSV::eValue, QModelIndex());
                   ui->tableViewModbus->model()->setData(index, unit.value(i));
                   break;
                   }
                } */
    } else if (reply->error() == QModbusDevice::ProtocolError) {
        statusBar()->showMessage(tr("Read response error: %1 (Mobus exception: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->rawResult().exceptionCode(), -1, 16), 5000);
        char buf[64];
        sprintf(buf, "!!! %s: %02X", reply->errorString().toStdString().c_str(), reply->rawResult().exceptionCode());

        QTextCharFormat tf;
        tf = ui->plainTextConsole->currentCharFormat();
        tf.setForeground(QBrush(QColor("magenta")));
        ui->plainTextConsole->setCurrentCharFormat(tf);
        ui->plainTextConsole->appendPlainText(QString(buf));
qDebug() << "Except:" << QString(buf);
        mModbusExcept = reply->rawResult().exceptionCode();
    } else {
        statusBar()->showMessage(tr("Read response error: %1 (code: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->error(), -1, 16), 5000);
        char buf[64];
        sprintf(buf, "Err %s: %02X", reply->errorString().toStdString().c_str(), reply->error());

        QTextCharFormat tf;
        tf = ui->plainTextConsole->currentCharFormat();
        tf.setForeground(QBrush(QColor("red")));
        ui->plainTextConsole->setCurrentCharFormat(tf);
        ui->plainTextConsole->appendPlainText(QString(buf));
qDebug() << "Err:" << QString(buf);
        mModbusErr = reply->error();
        }
    reply->deleteLater();
}

void MainWindow::msSleep(uint ms)
{
    for (uint i=0; i<ms; i++) {
        QApplication::processEvents();
        QThread::msleep(1);
        }

}

void MainWindow::slotModbusRegRead(int iRegAddr, quint16 iRegCount)
{
    int iServerAddr = ui->serverEdit->value();
    statusBar()->clearMessage();
    QModbusDataUnit du = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, iRegAddr, iRegCount);
    qDebug() << __FUNCTION__ << QString::number(du.startAddress(),16).toUpper() << du.values();

    if (auto *reply = modbusDevice->sendReadRequest(du, iServerAddr) ) {
        if (!reply->isFinished()){
            connect(reply, &QModbusReply::finished, this, &MainWindow::readReady);
            }
        else
            delete reply; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbusDevice->errorString(), 5000);
        }
}
void MainWindow::slotModbusRegsWrite(int iRegAddr, QVector<quint16> data)
{
    int iServerAddr = ui->serverEdit->value();
    statusBar()->clearMessage();
//qDebug() << "slotRegsW" << data << data.size();
    QModbusDataUnit du = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, iRegAddr, data);
    qDebug() << __FUNCTION__ << QString::number(du.startAddress(),16).toUpper() << du.values();
    if (auto *reply = modbusDevice->sendWriteRequest(du, iServerAddr) ) {
        qApp->exec();
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &MainWindow::readReady);
        else
            delete reply; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbusDevice->errorString(), 5000);
    }
}

void MainWindow::slotModbusCoilWrite(int iCoilAddr, QVector<quint16> data)
{
    int iServerAddr = ui->serverEdit->value();
    statusBar()->clearMessage();
//qDebug() << "slotRegsW" << data << data.size();
    QModbusDataUnit du = QModbusDataUnit(QModbusDataUnit::Coils, iCoilAddr, data);
    qDebug() << __FUNCTION__ << QString::number(du.startAddress(),16).toUpper() << du.values();
    if (auto *reply = modbusDevice->sendWriteRequest(du, iServerAddr) ) {
        qApp->exec();
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &MainWindow::readReady);
        else
            delete reply; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbusDevice->errorString(), 5000);
    }
}

void MainWindow::slotModbusCoilRead(int iCoilAddr, quint16 iCoilCount)
{
    int iServerAddr = ui->serverEdit->value();
    statusBar()->clearMessage();
    QModbusDataUnit du = QModbusDataUnit(QModbusDataUnit::DiscreteInputs, iCoilAddr, iCoilCount);
    qDebug() << __FUNCTION__ << QString::number(du.startAddress(),16).toUpper() << du.values();

    if (auto *reply = modbusDevice->sendReadRequest(du, iServerAddr) ) {
        if (!reply->isFinished()){
            connect(reply, &QModbusReply::finished, this, &MainWindow::readReady);
            }
        else
            delete reply; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbusDevice->errorString(), 5000);
        }
}

void MainWindow::slotModbusCmd(int row, int iWAIT, int iLOOP)
{
    QList<QStringList> listCmds = pModelCSV->getStringLists();
    if (QVariant(listCmds[row][enumModbusCSV::eActRun]).toBool()) {

        ui->tableViewModbus->selectRow(row);
        ui->plainTextConsole->appendPlainText("> "+QString::number(row)+" "+listCmds[row][enumModbusCSV::eCategory]+" "+listCmds[row][enumModbusCSV::eDescription]);
        bool  ok;
        int iRun    = iLOOP?iLOOP:listCmds[row][enumModbusCSV::eActRun].toInt(&ok, 10);
        if (iRun == 0) return;

        int iRegAddr= listCmds[row][enumModbusCSV::eReg].toInt(&ok, 16);
        int  iCount = listCmds[row][enumModbusCSV::eCount].toInt(&ok, 10);
        QString sRW = listCmds[row][enumModbusCSV::eRW].toLocal8Bit();
        int iValue  = listCmds[row][enumModbusCSV::eValue].toInt(&ok, 16);
        int iWait   = iWAIT?iWAIT:listCmds[row][enumModbusCSV::eWait].toInt(&ok, 10);
        int iLoop   = iLOOP?iLOOP:listCmds[row][enumModbusCSV::eLoop].toInt(&ok, 10);
        char buf[128];
        for (int l=0;l<iLoop;l++) {
            if (sRW.contains("Rc",Qt::CaseInsensitive) ) { //Read coil
                if (!isDryRun) emit sigModbusCoilRead(iRegAddr, static_cast<quint16>(iCount));
                sprintf(buf, "  %s %d @0x%04X ", sRW.toStdString().c_str(), iCount, iRegAddr);
                ui->plainTextConsole->appendPlainText(buf);
                msSleep(static_cast<uint>(iWait));
                }
            if (sRW.contains("Rr",Qt::CaseInsensitive) ) { //Read regs
                if (!isDryRun) emit sigModbusRegRead(iRegAddr, static_cast<quint16>(iCount));
                sprintf(buf, "  %s %d @0x%04X ", sRW.toStdString().c_str(), iCount, iRegAddr);
                ui->plainTextConsole->appendPlainText(buf);
                msSleep(static_cast<uint>(iWait));
                }
            if (sRW.contains("Wc", Qt::CaseInsensitive) ) { //Write single ccoil
                QVector<quint16> data;
                data.resize(1);
                data[0] = static_cast<quint16>(iValue);

                if (!isDryRun) emit sigModbusCoilWrite(iRegAddr, data);
                sprintf(buf, "  %s @0x%X >0x%04X ", sRW.toStdString().c_str(), iRegAddr, iValue);
                ui->plainTextConsole->appendPlainText(buf);
                msSleep(static_cast<uint>(iWait));
                }
            if (sRW.contains("Wr", Qt::CaseInsensitive) ) { //Write regs
                QVector<quint16> data;
                if (iCount==1) {
                    data.resize(1);
                    data[0] = static_cast<quint16>(iValue);
                    }
                else{
                    data.resize(2);
                    data[0] = static_cast<quint16>(iValue>>16);
                    data[1] = iValue&0x0FFFF;
                    }
                if (!isDryRun) emit sigModbusRegsWrite(iRegAddr, data);
                sprintf(buf, "  %s %d @0x%X >0x%04X ", sRW.toStdString().c_str(), iCount, iRegAddr, iValue);
                ui->plainTextConsole->appendPlainText(buf);
                msSleep(static_cast<uint>(iWait));
                }
            }
        //msSleep(100); //for displaying selection
        }
}

void MainWindow::on_btnSend_clicked()
{
    QStringList slDU = ui->lineEditModbusData->text().split(" ");
    qDebug() << slDU;
    bool ok;
    int iFC   = slDU[1].toInt(&ok,16);
    int iReg  = slDU[2].toInt(&ok,16);
    int iCount= slDU[3].toInt();
    int iValue;
    QVector<quint16> data(iCount);
    switch (iFC) {
        case 1:
            qDebug() << iFC<< iReg<< iCount;
            emit sigModbusCoilRead(iReg, static_cast<quint16>(iCount));
            break;
        case 3:
            qDebug() << iFC<< iReg<< iCount;
            emit sigModbusRegRead(iReg, static_cast<quint16>(iCount));
            break;
        case 5:
            iValue= slDU[4].toInt(&ok,16);
            qDebug() << __FUNCTION__<<iFC<<iReg<< iValue;
            data[0]=static_cast<quint16>(iValue);
            //data.resize(1); //for single write
            emit sigModbusCoilWrite(iReg, data);
            break;
        case 6:
            iValue= slDU[4].toInt(&ok,16);
            qDebug() << iFC<<iReg<< iCount<< iValue;
            data[0]=static_cast<quint16>(iValue);
            //data.resize(1); //for single write
            emit sigModbusRegsWrite(iReg, data);
            break;
        case 16:
            for (int i=0;i<iCount;i++) {
                iValue= slDU[5+i].toInt(&ok,16);     //data start from #5
                data[i]=static_cast<quint16>(iValue);
                }
            qDebug() << iFC<< iReg<< iCount<< data;
            emit sigModbusRegsWrite(iReg, data);
            break;
            }

}


void MainWindow::on_btnRun_clicked()
{
static bool bRun=false;

    bRun = !bRun;
    if (bRun)
        ui->btnRun->setText("Stop");
    else
        ui->btnRun->setText("Run");

    isDryRun = false;
    int iLoop = ui->spinBoxRunLoop->value();
    while ((iLoop >0) && bRun) {
        QDateTime local(QDateTime::currentDateTime());
        QString sDateTime = local.toString("hh:mm:ss");
        ui->plainTextConsole->appendPlainText("<"+sDateTime+">"+QString::number(iLoop));
        for (int r=0; r<ui->tableViewModbus->model()->rowCount(); r++) {
            QList<QStringList> listCmds = pModelCSV->getStringLists();
            bool ok;
            int iRun    = listCmds[r][enumModbusCSV::eActRun].toInt(&ok, 10);
            if (iRun == 0) { qDebug() << "Skipped row" << r; continue;}

            //loop untill timeout or ready
            bModbusReplyOK = false;
            int iTimeout = 0; //1000ms

            while ((iTimeout < 3000) && !bModbusReplyOK && bRun){
                if ((iTimeout%1000)==0) emit sigModbusCmd(r);
                msSleep(1);
                //qApp->exec();
                iTimeout++;
                }
            /*
            //Modbus run state machine
            int iRetry=0;
            int iState=0;
            while (bRun) {
                switch (iState) {
                    case 0: //init
                        emit sigModbusCmd(r);
                        iRetry++;
                        if (iRetry>=3)
                            iState=9; //stop
                        else
                            iState=1; //next state
                        break;
                    case 1: //wait modbus write OK or error! or timeout!
                        msSleep(1);qApp->exec();
                        iTimeout++;
                        if (iTimeout>3000)  {iState=9; qDebug()<<"Modbus Timeout!";} //stop
                        if (bModbusReplyOK) {iState=9; qDebug()<<"Modbus ReplyOK!";}
                        if (mModbusExcept>0){iState=0; qDebug()<<"Modbus Exception:"<<mModbusExcept;} //retry
                        if (mModbusErr>0)   {iState=0; qDebug()<<"Modbus Error:"<<mModbusErr;} //stop
                        break;
                    case 2:
                        break; //OK, next
                    default:
                    case 9:
                        bRun=false; //STOP
                        break;
                    }
                }*/
            }
        ui->plainTextConsole->appendPlainText("--------------------");
        ui->tableViewModbus->selectRow(0);
        iLoop = iLoop-1;
        ui->spinBoxRunLoop->setValue(iLoop);
        }
    bRun=false;
    ui->btnRun->setText("Run");
    ui->spinBoxRunLoop->setValue(1);
}


void MainWindow::on_btnDryRun_clicked()
{
    static bool bRun=false;

    bRun = !bRun;
    if (bRun)
       ui->btnDryRun->setText("Stop");
    else
       ui->btnDryRun->setText("DryRun");

    isDryRun = true;
    int iLoop = ui->spinBoxRunLoop->value();
    while ((iLoop >0) && bRun) {
        ui->plainTextConsole->appendPlainText("< Dry Run >"+QString::number(iLoop));
        for (int r=0; r<ui->tableViewModbus->model()->rowCount(); r++) {
            QList<QStringList> listCmds = pModelCSV->getStringLists();
            bool ok;
            int iRun    = listCmds[r][enumModbusCSV::eActRun].toInt(&ok, 10);
            if (iRun == 0) { qDebug() << "Skipped row" << r; continue;}

            emit sigModbusCmd(r);
            }
        ui->plainTextConsole->appendPlainText("--------------------");
        ui->tableViewModbus->selectRow(0);
        iLoop = iLoop-1;
        ui->spinBoxRunLoop->setValue(iLoop);
        }
    bRun=false;
    ui->btnDryRun->setText("DryRun");
    ui->spinBoxRunLoop->setValue(1);
}

void MainWindow::on_cbCmdFile_currentTextChanged(const QString &arg1)
{
    loadListCSV(arg1);
}





