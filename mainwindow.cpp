#include "mainwindow.h"
#include "ui_mainwindow_cn.h"

#include <qdebug.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->nodeListTableWidget->setColumnWidth(0,202);
    ui->nodeListTableWidget->setColumnWidth(1,202);
    ui->nodeListTableWidget->setColumnWidth(2,202);

    ui->statusBar->showMessage("未连接设备",0);
    isConnect = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_openFirmwareFilePushButton_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this,
                                            tr("Open files"),
                                            "",
                                            "Binary Files (*.bin);;Hex Files (*.hex);;All Files (*.*)");
    if(fileName.isNull()) {
        return;
    }
    ui->firmwareLineEdit->setText(fileName);
}

void MainWindow::on_connect_clicked()
{
    if(ui->connect->text() == "断开") {
        CAN_BL_Close();
    }
    else if(ui->connect->text() == "连接") {
        if(ui->deviceHandleComboBox->currentText() == "CANalyst-II")        nDeviceType = 4;
        else if(ui->deviceHandleComboBox->currentText() == "USB_CAN")       nDeviceType = 4;
        nCanIndex = ui->channelIndexComboBox->currentIndex();

        DWORD dwRel = VCI_OpenDevice(nDeviceType, nDeviceIndex, 0);
        if(dwRel != 1) {
            QMessageBox::warning(NULL, "warning", "Failed to open the device!\n请确认CAN下载器！", QMessageBox::Yes, QMessageBox::Yes);
            ui->statusBar->showMessage("连接设备失败",0);
            return;
        }
        else {
            isConnect = true;
            ui->deviceHandleComboBox->setEnabled(false);
            ui->channelIndexComboBox->setEnabled(false);
            ui->baudRateComboBox->setEditable(false);
            ui->statusBar->showMessage("设备正常连接",0);
            ui->connect->setText("断开");
        }
        VCI_INIT_CONFIG vic;
        setCanConfig(&vic);
        if(VCI_InitCAN(nDeviceType, nDeviceIndex, nCanIndex, &vic) != 1) {
            CAN_BL_Close();
            QMessageBox::warning(NULL, "warning", "初始化设备失败！", QMessageBox::Yes, QMessageBox::Yes);
            return;
        }
        if(VCI_StartCAN(nDeviceType, nDeviceIndex, nCanIndex) != 1) {
            CAN_BL_Close();
            QMessageBox::warning(NULL, "warning", "启动设备失败！", QMessageBox::Yes, QMessageBox::Yes);
            return;
        }
    }
}

void MainWindow::on_exitAction_triggered()
{
    this->close();
}

void MainWindow::on_openFirmwareFileAction_triggered()
{
    on_openFirmwareFilePushButton_clicked();
}

void MainWindow::on_updateFirmwarePushButton_clicked()
{
    if(ui->allNodeCheckBox->isChecked())
    {
        int rowCount = ui->nodeListTableWidget->rowCount();
        for(int i=0; i<rowCount; i++)
        {
            uint16_t NodeAddr = ui->nodeListTableWidget->item(i,0)->text().toInt(NULL,16);
            setFirmware(NodeAddr);
        }
    }
    else
    {
        if(ui->allNodeCheckBox->isChecked()) {
            if(ui->nodeListTableWidget->rowCount()<=0){
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("无任何节点！"));
                return;
            }
        }
        else {
            if(ui->nodeListTableWidget->currentIndex().row()<0){
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("请选择节点！"));
            return;
            }
        }
        uint16_t NodeAddr = ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),0)->text().toInt(NULL,16);
        qDebug()<<NodeAddr;
        setFirmware(NodeAddr);
    }
}

void MainWindow::on_scanNodeAction_triggered()
{
    if(!isConnect) {
        QMessageBox::warning(NULL, "warning", "请先启动设备！", QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    //int startAddr=0, endAddr=0;
    ScanDevRangeDialog *pScanDevRangeDialog = new ScanDevRangeDialog();
    if(pScanDevRangeDialog->exec() == QDialog::Accepted) {
        startAddr = pScanDevRangeDialog->StartAddr;
        endAddr = pScanDevRangeDialog->EndAddr;
    }
    else {
        return;
    }
    int _startAddr=startAddr, _endAddr=endAddr;
    ui->nodeListTableWidget->verticalHeader()->hide();
    ui->nodeListTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->nodeListTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->nodeListTableWidget->setRowCount(0);
    QProgressDialog scanNodeProcess(QStringLiteral("正在扫描节点..."),QStringLiteral("取消"),0,endAddr-startAddr,this);
    scanNodeProcess.setWindowTitle(QStringLiteral("扫描节点"));
    scanNodeProcess.setModal(true);
    scanNodeProcess.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    int i=0;
    while(_startAddr <= _endAddr){
        i++;
        uint8_t appversion[4];
        uint8_t appType[4];
        int ret = CAN_BL_NodeCheck(_startAddr,appversion,appType);
        if(ret == 1)
        {
            ui->nodeListTableWidget->setRowCount(ui->nodeListTableWidget->rowCount()+1);
            ui->nodeListTableWidget->setRowHeight(ui->nodeListTableWidget->rowCount()-1,20);
            QString str;
            str.sprintf("0x%X",_startAddr);
            QTableWidgetItem *item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,0,item);
            if(appType[0] == 0x55)
            {
                str = "BOOT";
            }else
            {
                str = "APP";
            }
            item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,1,item);
            str.sprintf("v%d.%d",appversion[0]*16+appversion[1],appversion[2]*16+appversion[3]);
            item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,2,item);
        }
        scanNodeProcess.setValue(i);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        if(scanNodeProcess.wasCanceled())
        {
            return;
        }
        _startAddr++;
    }
}

void MainWindow::on_deleteAction_triggered()
{
    //ui->nodeListTableWidget->clearContents();
    int rowCount = ui->nodeListTableWidget->rowCount();
    if(rowCount>0)
    {
        for(int i=0; i<rowCount; i++)
        {
            uint16_t NodeAddr = ui->nodeListTableWidget->item(i,0)->text().toInt(NULL,16);
            //setFirmware(NodeAddr);
            //跳转到bootloader执行
            CAN_BL_Excute(NodeAddr,CAN_BL_BOOT);
            sleep(100);
        }
        if(!isConnect) {
            QMessageBox::warning(NULL, "warning", "请先启动设备！", QMessageBox::Yes, QMessageBox::Yes);
            return;
        }
        int _startAddr=startAddr, _endAddr=endAddr;
        ui->nodeListTableWidget->verticalHeader()->hide();
        ui->nodeListTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->nodeListTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->nodeListTableWidget->setRowCount(0);
        QProgressDialog scanNodeProcess(QStringLiteral("正在扫描节点..."),QStringLiteral("取消"),0,endAddr-startAddr,this);
        scanNodeProcess.setWindowTitle(QStringLiteral("扫描节点"));
        scanNodeProcess.setModal(true);
        scanNodeProcess.show();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        int i=0;
        while(_startAddr <= _endAddr){
            i++;
            uint8_t appversion[4];
            uint8_t appType[4];
            int ret = CAN_BL_NodeCheck(_startAddr,appversion,appType);
            if(ret == 1)
            {
                ui->nodeListTableWidget->setRowCount(ui->nodeListTableWidget->rowCount()+1);
                ui->nodeListTableWidget->setRowHeight(ui->nodeListTableWidget->rowCount()-1,20);
                QString str;
                str.sprintf("0x%X",_startAddr);
                QTableWidgetItem *item = new QTableWidgetItem(str);
                ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,0,item);
                if(appType[0] == 0x55)
                {
                    str = "BOOT";
                }else
                {
                    str = "APP";
                }
                item = new QTableWidgetItem(str);
                ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,1,item);
                str.sprintf("v%d.%d",appversion[0]*16+appversion[1],appversion[2]*16+appversion[3]);
                item = new QTableWidgetItem(str);
                ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,2,item);
            }
            scanNodeProcess.setValue(i);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            if(scanNodeProcess.wasCanceled())
            {
                return;
            }
            _startAddr++;
        }
    }
}

void MainWindow::setCanConfig(PVCI_INIT_CONFIG pInitConfig)
{
    pInitConfig->AccCode = 0;
    pInitConfig->AccMask = 0xFFFFFFFF;
    pInitConfig->Filter = 0;
    pInitConfig->Mode = 0;
    switch (ui->baudRateComboBox->currentIndex()) {
    case 0: //1000 Kbps
        pInitConfig->Timing1 = 0x14;
        pInitConfig->Timing0 = 0;
        break;
    case 1: //800 Kbps
        pInitConfig->Timing1 = 0x16;
        pInitConfig->Timing0 = 0;
        break;
    case 2: //500 Kbps
        pInitConfig->Timing1 = 0x1C;
        pInitConfig->Timing0 = 0;
        break;
    case 3: //300 Kbps
        pInitConfig->Timing1 = 0x1E;
        pInitConfig->Timing0 = 0;
        break;
    case 4: //250 Kbps
        pInitConfig->Timing1 = 0x1C;
        pInitConfig->Timing0 = 0x01;
        break;
    case 5: //200 Kbps
        pInitConfig->Timing1 = 0xFA;
        pInitConfig->Timing0 = 0x81;
        break;
    case 6: //150 Kbps
        pInitConfig->Timing1 = 0x1C;
        pInitConfig->Timing0 = 0x02;
        break;
    case 7: //125 Kbps
        pInitConfig->Timing1 = 0x1C;
        pInitConfig->Timing0 = 0x03;
        break;
    case 8: //100 Kbps
        pInitConfig->Timing1 = 0x1C;
        pInitConfig->Timing0 = 0x04;
        break;
    case 9: //75 Kbps
        pInitConfig->Timing1 = 0x1C;
        pInitConfig->Timing0 = 0x05;
        break;
    case 10: //50 Kbps
        pInitConfig->Timing1 = 0x1C;
        pInitConfig->Timing0 = 0x09;
        break;
    case 11: //20 Kbps
        pInitConfig->Timing1 = 0x1C;
        pInitConfig->Timing0 = 0x18;
        break;
    default:
        break;
    }
}

int MainWindow::CAN_BL_NodeCheck(uint16_t NodeAddr, uint8_t* version, uint8_t* type)
{
    VCI_CAN_OBJ vci[48];
    vci[0].ExternFlag = 1;
    vci[0].SendType = 1;
    vci[0].RemoteFlag = 0;
    vci[0].ID = NodeAddr*16+3;//(NodeAddr*16+3)*4096;
    DWORD dwRel = VCI_Transmit(nDeviceType, nDeviceIndex, nCanIndex, vci, 1);
    VCI_CAN_OBJ vco[2500];
    int i=10;
    do {
        i--;
        sleep(1);
        DWORD dwRel = VCI_Receive(nDeviceType, nDeviceIndex, nCanIndex, vco, 2500, 0);
        if(dwRel > 0) {
            for(int j=0; j<dwRel; j++) {
                int _cmd = vco[j].ID & 0xF;
                int _id = vco[j].ID >> 4;
                if((_id==NodeAddr) && (_cmd==0x08)) {
                    version[0] = vco[j].Data[0];
                    version[1] = vco[j].Data[1];
                    version[2] = vco[j].Data[2];
                    version[3] = vco[j].Data[3];
                    type[0] = vco[j].Data[4];
                    type[1] = vco[j].Data[5];
                    type[2] = vco[j].Data[6];
                    type[3] = vco[j].Data[7];
                    return 1;
                }
            }
        }
    } while(i>0);
    return 0;
}

int MainWindow::CAN_BL_Erase(uint16_t NodeAddr,qint64 _size)
{
    VCI_CAN_OBJ vci;
    vci.ExternFlag = 1;
    vci.SendType = 1;
    vci.RemoteFlag = 0;
    vci.ID = NodeAddr*16;
    vci.DataLen = 4;
    vci.Data[0] = _size >> 24;
    vci.Data[1] = _size >> 16;
    vci.Data[2] = _size >> 8;
    vci.Data[3] = _size;
    DWORD dwRel = VCI_Transmit(nDeviceType, nDeviceIndex, nCanIndex, &vci, 1);
    VCI_CAN_OBJ vco[2500];
    int i=2000;
    do {
        i--;
        sleep(1);
        DWORD dwRel = VCI_Receive(nDeviceType, nDeviceIndex, nCanIndex, vco, 2500, 0);
        if(dwRel > 0) {
            for(int j=0; j<dwRel; j++) {
                int _cmd = vco[j].ID & 0xF;
                int _id = vco[j].ID >> 4;
                if((_id==NodeAddr) && (_cmd==0x08)) {
                    return 1;
                }
            }
        }
    } while(i>0);
    return 0;
}

int MainWindow::CAN_BL_Write_Addr(uint16_t NodeAddr, uint16_t newNodeAddr)
{
    //跳转到bootloader执行
    CAN_BL_Excute(NodeAddr,CAN_BL_BOOT);
    sleep(100);
    //查询是否跳转到bootloader
    uint8_t appversion[4];
    uint8_t appType[4];
    int ret = CAN_BL_NodeCheck(NodeAddr,appversion,appType);
    if(ret == 1)
    {
        if(appType[0] != 0x55)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件类型为APP，请重新执行！"));
            return 0;
        }
    }
    else
    {
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("跳转BOOT失败！"));
        return 0;
    }

    VCI_CAN_OBJ vci;
    vci.ExternFlag = 1;
    vci.SendType = 1;
    vci.RemoteFlag = 0;
    vci.ID = NodeAddr*16 + 6;
    vci.DataLen = 2;
    vci.Data[1] = newNodeAddr >> 8;
    vci.Data[0] = newNodeAddr;
    DWORD dwRel = VCI_Transmit(nDeviceType, nDeviceIndex, nCanIndex, &vci, 1);
    VCI_CAN_OBJ vco[2500];
    int i=2000;
    do {
        i--;
        sleep(1);
        DWORD dwRel = VCI_Receive(nDeviceType, nDeviceIndex, nCanIndex, vco, 2500, 0);
        if(dwRel > 0) {
            for(int j=0; j<dwRel; j++) {
                int _cmd = vco[j].ID & 0xF;
                int _id = vco[j].ID >> 4;
                if((_id==newNodeAddr) && (_cmd==0x08)) {
                    return 1;
                }
            }
        }
    } while(i>0);
    return 0;
}

int MainWindow::CAN_BL_Write(uint16_t NodeAddr,unsigned int AddrOffset,unsigned char *pData,unsigned int DataNum)
{
    unsigned char write_data[1028];
    int i_=0;
    for(i_=0; i_<DataNum; i_++) {
        write_data[i_] = pData[i_];
    }
    unsigned short _crc = crc16_ccitt(pData, DataNum);
    write_data[i_++] = _crc>>8;
    write_data[i_++] = _crc;
    VCI_CAN_OBJ vci;
    vci.ExternFlag = 1;
    vci.SendType = 1;
    vci.RemoteFlag = 0;
    vci.ID = NodeAddr*16 + 1;
    vci.DataLen = 8;
    vci.Data[0] = AddrOffset >> 24;
    vci.Data[1] = AddrOffset >> 16;
    vci.Data[2] = AddrOffset >> 8;
    vci.Data[3] = AddrOffset;
    unsigned int _DataNum = DataNum + 2;
    vci.Data[4] = _DataNum >> 24;
    vci.Data[5] = _DataNum >> 16;
    vci.Data[6] = _DataNum >> 8;
    vci.Data[7] = _DataNum;
    DWORD dwRel = VCI_Transmit(nDeviceType, nDeviceIndex, nCanIndex, &vci, 1);
    VCI_CAN_OBJ vco[2500];
    bool isInfo = false;
    int i=200;
    do {
        i--;
        sleep(1);
        DWORD dwRel = VCI_Receive(nDeviceType, nDeviceIndex, nCanIndex, vco, 2500, 0);
        if(dwRel > 0) {
            for(int j=0; j<dwRel; j++) {
                int _cmd = vco[j].ID & 0xF;
                int _id = vco[j].ID >> 4;
                if((_id==NodeAddr) && (_cmd==0x08)) {
                    isInfo = true;
                    i = 0;
                }
            }
        }
    } while(i>0);
    if(!isInfo) return 0;

    int data_8_len = _DataNum;
    for(int _i=0; _i<_DataNum; _i+=8) {
        VCI_CAN_OBJ vci_d;
        vci_d.ExternFlag = 1;
        vci_d.SendType = 1;
        vci_d.RemoteFlag = 0;
        vci_d.ID = NodeAddr*16 + 2;
        if(data_8_len<8) {
            vci_d.DataLen = data_8_len;
            for(int _j=0; _j<data_8_len; _j++) {
                vci_d.Data[_j] = write_data[_i+_j];
            }
        }
        else {
            vci_d.DataLen = 8;
            for(int _j=0; _j<8; _j++) {
                vci_d.Data[_j] = write_data[_i+_j];
            }
        }
        data_8_len -= 8;
        DWORD dwRel = VCI_Transmit(nDeviceType, nDeviceIndex, nCanIndex, &vci_d, 1);
    }

    VCI_CAN_OBJ _vco[2500];
    bool isWrite = false;
    i=200;
    do {
        i--;
        sleep(1);
        DWORD dwRel = VCI_Receive(nDeviceType, nDeviceIndex, nCanIndex, _vco, 2500, 0);
        if(dwRel > 0) {
            for(int j=0; j<dwRel; j++) {
                int _cmd = _vco[j].ID & 0xF;
                int _id = _vco[j].ID >> 4;
                if((_id==NodeAddr) && (_cmd==0x08)) {
                    isWrite = true;
                    i = 0;
                }
                else {
                    i=0;
                }
            }
        }
    } while(i>0);
    if(isWrite) {
        qDebug()<<"yes";
        return 1;
    }
    else {
        qDebug()<<"error";
        return 0;
    }
}

int MainWindow::CAN_BL_Excute(uint16_t NodeAddr,DWORD bootType)
{
    VCI_CAN_OBJ vci;
    vci.ExternFlag = 1;
    vci.SendType = 1;
    vci.RemoteFlag = 0;
    vci.DataLen = 8;
    vci.ID = NodeAddr*16 + 5;
    for(int i=0; i<4; i++) {
        if(bootType == CAN_BL_APP)
            vci.Data[i] = 0xAA;
        else if(bootType == CAN_BL_BOOT)
            vci.Data[i] = 0x55;
    }
    VCI_Transmit(nDeviceType, nDeviceIndex, nCanIndex, &vci, 1);
    //跳转指令无返回
    return 1;
}

void MainWindow::CAN_BL_Close()
{
    isConnect = false;
    int dwRel = VCI_CloseDevice(nDeviceType, nDeviceIndex);
    if(dwRel != 1) {
        QMessageBox::warning(NULL, "warning", "Failed to turn off the device!\n请确认CAN下载器是否正常连接！", QMessageBox::Yes, QMessageBox::Yes);
    }
    ui->deviceHandleComboBox->setEnabled(true);
    ui->channelIndexComboBox->setEnabled(true);
    ui->baudRateComboBox->setEditable(true);
    ui->connect->setText("连接");
    ui->statusBar->showMessage("未连接设备",0);
}


void MainWindow::setFirmware(uint16_t NodeAddr)
{
    //跳转到bootloader执行
    CAN_BL_Excute(NodeAddr,CAN_BL_BOOT);
    sleep(100);
    //查询是否跳转到bootloader
    uint8_t appversion[4];
    uint8_t appType[4];
    int ret = CAN_BL_NodeCheck(NodeAddr,appversion,appType);
    if(ret == 1)
    {
        if(appType[0] != 0x55)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件类型为APP，请重新执行！"));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件程序失败！"));
        return;
    }
    //打开固件
    QFile firmwareFile(ui->firmwareLineEdit->text());
    if(firmwareFile.open(QFile::ReadOnly))
    {
        int ret_erase = CAN_BL_Erase(NodeAddr,firmwareFile.size());
        if(ret_erase != 1)
        {
            QMessageBox::warning(this,"警告",QString().sprintf("擦除Flash失败！%d",ret_erase));
            return;
        }
        QProgressDialog writeDataProcess(QStringLiteral("正在更新固件..."),QStringLiteral("取消"),0,firmwareFile.size(),this);
        writeDataProcess.setWindowTitle(QStringLiteral("更新固件"));
        writeDataProcess.setModal(true);
        writeDataProcess.show();
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        int PackSize = 512;
        uint8_t FirmwareData[1026]={0};
        for(int i=0; i<firmwareFile.size(); i+=PackSize) {
            int read_data_num = firmwareFile.read((char*)FirmwareData, PackSize);
            int ret = CAN_BL_Write(NodeAddr,i,FirmwareData,read_data_num);
            if(!ret)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("传输固件程序失败！"));
                return;

            }
            writeDataProcess.setValue(i);
            if(writeDataProcess.wasCanceled()){
                return;
            }
        }

        //跳转到app执行
        //CAN_BL_Excute(NodeAddr,CAN_BL_APP);
    }
    else{
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("打开固件文件失败！"));
        return;
    }
}

void MainWindow::sleep(unsigned int msec)
{
    QTime reachTime = QTime::currentTime().addMSecs(msec);
    while (QTime::currentTime() < reachTime) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

void MainWindow::on_setaddressPushButton_clicked()
{
    if(ui->allNodeCheckBox->isChecked())
    {
        if(ui->nodeListTableWidget->rowCount()<=0){
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("无任何节点！"));
            return;
        }
    }
    else {
        if(ui->nodeListTableWidget->currentIndex().row()<0)
        {
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("请选择节点！"));
        return;
        }
    }
    uint16_t NodeAddr = ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),0)->text().toInt(NULL,16);
    uint16_t newNodeAddr = ui->addrSpinBox->value();
    int ret = CAN_BL_Write_Addr(NodeAddr, newNodeAddr);
    if(ret)
    {
        //跳转到app执行
        CAN_BL_Excute(NodeAddr,CAN_BL_APP);

        QMessageBox message(QMessageBox::NoIcon, "节点地址", "节点地址修改成功.");
        message.setIconPixmap(QPixmap(":/images/gif_48_007.gif"));
        message.exec();
    }
    else
    {
        QMessageBox::warning(this,QStringLiteral("节点地址"),QStringLiteral("节点地址修改失败！"));
    }
}

void MainWindow::on_contactUsAction_triggered()
{
    //ui->nodeListTableWidget->clearContents();
    int rowCount = ui->nodeListTableWidget->rowCount();
    if(rowCount>0)
    {
        for(int i=0; i<rowCount; i++)
        {
            uint16_t NodeAddr = ui->nodeListTableWidget->item(i,0)->text().toInt(NULL,16);
            //跳转到app执行
            CAN_BL_Excute(NodeAddr,CAN_BL_APP);
            sleep(400);
        }
        sleep(2000);
        if(!isConnect) {
            QMessageBox::warning(NULL, "warning", "请先启动设备！", QMessageBox::Yes, QMessageBox::Yes);
            return;
        }
        int _startAddr=startAddr, _endAddr=endAddr;
        ui->nodeListTableWidget->verticalHeader()->hide();
        ui->nodeListTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->nodeListTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->nodeListTableWidget->setRowCount(0);
        QProgressDialog scanNodeProcess(QStringLiteral("正在扫描节点..."),QStringLiteral("取消"),0,endAddr-startAddr,this);
        scanNodeProcess.setWindowTitle(QStringLiteral("扫描节点"));
        scanNodeProcess.setModal(true);
        scanNodeProcess.show();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        int i=0;
        while(_startAddr <= _endAddr){
            i++;
            uint8_t appversion[4];
            uint8_t appType[4];
            int ret = CAN_BL_NodeCheck(_startAddr,appversion,appType);
            if(ret == 1)
            {
                ui->nodeListTableWidget->setRowCount(ui->nodeListTableWidget->rowCount()+1);
                ui->nodeListTableWidget->setRowHeight(ui->nodeListTableWidget->rowCount()-1,20);
                QString str;
                str.sprintf("0x%X",_startAddr);
                QTableWidgetItem *item = new QTableWidgetItem(str);
                ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,0,item);
                if(appType[0] == 0x55)
                {
                    str = "BOOT";
                }else
                {
                    str = "APP";
                }
                item = new QTableWidgetItem(str);
                ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,1,item);
                str.sprintf("v%d.%d",appversion[0]*16+appversion[1],appversion[2]*16+appversion[3]);
                item = new QTableWidgetItem(str);
                ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,2,item);
            }
            scanNodeProcess.setValue(i);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            if(scanNodeProcess.wasCanceled())
            {
                return;
            }
            _startAddr++;
        }
    }
}
