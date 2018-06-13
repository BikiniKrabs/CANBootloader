#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QTimer>
#include <QProgressDialog>

#include "scandevrangedialog.h"
//CAN
#include "ControlCAN.h"

#include "crc16.h"

#define CAN_BL_BOOT     0x55555555
#define CAN_BL_APP      0xAAAAAAAA

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
    void setCanConfig(PVCI_INIT_CONFIG pInitConfig);

    void on_openFirmwareFilePushButton_clicked();

    void on_connect_clicked();

    void on_exitAction_triggered();

    void on_openFirmwareFileAction_triggered();

    void on_updateFirmwarePushButton_clicked();

    void on_scanNodeAction_triggered();

    void on_deleteAction_triggered();

    void on_setaddressPushButton_clicked();

    void on_contactUsAction_triggered();

private:
    void CAN_BL_Close();

    int CAN_BL_NodeCheck(uint16_t NodeAddr, uint8_t* version, uint8_t* type);

    int CAN_BL_Erase(uint16_t NodeAddr,qint64 _size);

    int CAN_BL_Write(uint16_t NodeAddr,unsigned int AddrOffset,unsigned char *pData,unsigned int DataNum);

    int CAN_BL_Excute(uint16_t NodeAddr,DWORD bootType);

    int CAN_BL_Write_Addr(uint16_t NodeAddr, uint16_t newNodeAddr);

    void setFirmware(uint16_t NodeAddr);

    void sleep(unsigned int msec);

private:
    Ui::MainWindow *ui;

    int nDeviceType = 4;    //USBCAN-2A或者USBCAN-2C或者CANalyst-II
    int nDeviceIndex = 0;   //多设备选择
    int nCanIndex = 0;      //CAN通道

    bool isConnect;

    int startAddr=0, endAddr=0;
};

#endif // MAINWINDOW_H
