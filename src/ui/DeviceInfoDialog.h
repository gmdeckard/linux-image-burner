#ifndef DEVICEINFODIALOG_H
#define DEVICEINFODIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include "../core/DeviceManager.h"

class DeviceInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceInfoDialog(const DeviceInfo &deviceInfo, QWidget *parent = nullptr);

private slots:
    void refreshInfo();
    void unmountDevice();
    void ejectDevice();

private:
    void setupUi();
    void updateInfo();
    
    DeviceInfo m_deviceInfo;
    DeviceManager *m_deviceManager;
    
    QLabel *m_pathLabel;
    QLabel *m_modelLabel;
    QLabel *m_vendorLabel;
    QLabel *m_sizeLabel;
    QLabel *m_fileSystemLabel;
    QLabel *m_mountLabel;
    QLabel *m_typeLabel;
    QTextEdit *m_detailsText;
    
    QPushButton *m_refreshButton;
    QPushButton *m_unmountButton;
    QPushButton *m_ejectButton;
    QPushButton *m_closeButton;
};

#endif // DEVICEINFODIALOG_H
