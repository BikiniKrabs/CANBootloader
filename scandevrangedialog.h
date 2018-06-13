#ifndef SCANDEVRANGEDIALOG_H
#define SCANDEVRANGEDIALOG_H

#include <QDialog>

namespace Ui {
class ScanDevRangeDialog;
}

class ScanDevRangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScanDevRangeDialog(QWidget *parent = 0);
    ~ScanDevRangeDialog();

public:
    int StartAddr,EndAddr;

private slots:
    void on_pushButtonConfirm_clicked();

    void on_pushButtonCancel_clicked();

    void on_spinBoxEndAddr_valueChanged(int arg1);

    void on_spinBoxStartAddr_valueChanged(int arg1);

private:
    Ui::ScanDevRangeDialog *ui;
};

#endif // SCANDEVRANGEDIALOG_H
