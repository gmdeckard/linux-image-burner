#include "DeviceInfoDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include <QMessageBox>

DeviceInfoDialog::DeviceInfoDialog(const DeviceInfo &deviceInfo, QWidget *parent)
    : QDialog(parent)
    , m_deviceInfo(deviceInfo)
    , m_deviceManager(new DeviceManager(this))
{
    setWindowTitle("Device Information - " + deviceInfo.name);
    setModal(true);
    setupUi();
    updateInfo();
}

void DeviceInfoDialog::setupUi()
{
    resize(500, 400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Basic info group
    QGroupBox *basicGroup = new QGroupBox("Basic Information");
    QGridLayout *basicLayout = new QGridLayout(basicGroup);
    
    basicLayout->addWidget(new QLabel("Device Path:"), 0, 0);
    m_pathLabel = new QLabel();
    basicLayout->addWidget(m_pathLabel, 0, 1);
    
    basicLayout->addWidget(new QLabel("Model:"), 1, 0);
    m_modelLabel = new QLabel();
    basicLayout->addWidget(m_modelLabel, 1, 1);
    
    basicLayout->addWidget(new QLabel("Vendor:"), 2, 0);
    m_vendorLabel = new QLabel();
    basicLayout->addWidget(m_vendorLabel, 2, 1);
    
    basicLayout->addWidget(new QLabel("Size:"), 3, 0);
    m_sizeLabel = new QLabel();
    basicLayout->addWidget(m_sizeLabel, 3, 1);
    
    basicLayout->addWidget(new QLabel("File System:"), 4, 0);
    m_fileSystemLabel = new QLabel();
    basicLayout->addWidget(m_fileSystemLabel, 4, 1);
    
    basicLayout->addWidget(new QLabel("Mount Status:"), 5, 0);
    m_mountLabel = new QLabel();
    basicLayout->addWidget(m_mountLabel, 5, 1);
    
    basicLayout->addWidget(new QLabel("Device Type:"), 6, 0);
    m_typeLabel = new QLabel();
    basicLayout->addWidget(m_typeLabel, 6, 1);
    
    mainLayout->addWidget(basicGroup);
    
    // Detailed info
    QGroupBox *detailsGroup = new QGroupBox("Detailed Information");
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);
    
    m_detailsText = new QTextEdit();
    m_detailsText->setReadOnly(true);
    m_detailsText->setMaximumHeight(120);
    detailsLayout->addWidget(m_detailsText);
    
    mainLayout->addWidget(detailsGroup);
    
    // Action buttons
    QHBoxLayout *actionLayout = new QHBoxLayout();
    
    m_refreshButton = new QPushButton("Refresh");
    m_unmountButton = new QPushButton("Unmount");
    m_ejectButton = new QPushButton("Eject");
    
    actionLayout->addWidget(m_refreshButton);
    actionLayout->addWidget(m_unmountButton);
    actionLayout->addWidget(m_ejectButton);
    actionLayout->addStretch();
    
    mainLayout->addLayout(actionLayout);
    
    // Close button
    QHBoxLayout *closeLayout = new QHBoxLayout();
    m_closeButton = new QPushButton("Close");
    closeLayout->addStretch();
    closeLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(closeLayout);
    
    // Connections
    connect(m_refreshButton, &QPushButton::clicked, this, &DeviceInfoDialog::refreshInfo);
    connect(m_unmountButton, &QPushButton::clicked, this, &DeviceInfoDialog::unmountDevice);
    connect(m_ejectButton, &QPushButton::clicked, this, &DeviceInfoDialog::ejectDevice);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void DeviceInfoDialog::updateInfo()
{
    m_pathLabel->setText(m_deviceInfo.path);
    m_modelLabel->setText(m_deviceInfo.model.isEmpty() ? "Unknown" : m_deviceInfo.model);
    m_vendorLabel->setText(m_deviceInfo.vendor.isEmpty() ? "Unknown" : m_deviceInfo.vendor);
    m_sizeLabel->setText(QString("%1 (%2 bytes)").arg(m_deviceInfo.sizeString).arg(m_deviceInfo.size));
    m_fileSystemLabel->setText(m_deviceInfo.fileSystem.isEmpty() ? "Unknown" : m_deviceInfo.fileSystem);
    
    if (m_deviceInfo.isMounted) {
        m_mountLabel->setText("Mounted at: " + m_deviceInfo.mountPoints.join(", "));
        m_unmountButton->setEnabled(true);
    } else {
        m_mountLabel->setText("Not mounted");
        m_unmountButton->setEnabled(false);
    }
    
    QStringList typeInfo;
    if (m_deviceInfo.isRemovable) typeInfo << "Removable";
    if (m_deviceInfo.isUSB) typeInfo << "USB";
    if (m_deviceInfo.isMMC) typeInfo << "MMC/SD";
    
    m_typeLabel->setText(typeInfo.isEmpty() ? "Fixed" : typeInfo.join(", "));
    
    // Detailed info
    QString details = QString(
        "Device Name: %1\n"
        "UUID: %2\n"
        "Removable: %3\n"
        "USB Device: %4\n"
        "MMC Device: %5\n"
        "Currently Mounted: %6\n"
    ).arg(m_deviceInfo.name)
     .arg(m_deviceInfo.uuid.isEmpty() ? "None" : m_deviceInfo.uuid)
     .arg(m_deviceInfo.isRemovable ? "Yes" : "No")
     .arg(m_deviceInfo.isUSB ? "Yes" : "No")
     .arg(m_deviceInfo.isMMC ? "Yes" : "No")
     .arg(m_deviceInfo.isMounted ? "Yes" : "No");
    
    if (m_deviceInfo.isMounted && !m_deviceInfo.mountPoints.isEmpty()) {
        details += "Mount Points:\n";
        for (const QString &mountPoint : m_deviceInfo.mountPoints) {
            details += "  " + mountPoint + "\n";
        }
    }
    
    m_detailsText->setPlainText(details);
}

void DeviceInfoDialog::refreshInfo()
{
    m_deviceInfo = m_deviceManager->getDeviceInfo(m_deviceInfo.path);
    updateInfo();
}

void DeviceInfoDialog::unmountDevice()
{
    if (m_deviceManager->unmountDevice(m_deviceInfo.path)) {
        QMessageBox::information(this, "Success", "Device unmounted successfully");
        refreshInfo();
    } else {
        QMessageBox::warning(this, "Error", "Failed to unmount device");
    }
}

void DeviceInfoDialog::ejectDevice()
{
    if (m_deviceManager->ejectDevice(m_deviceInfo.path)) {
        QMessageBox::information(this, "Success", "Device ejected successfully");
        refreshInfo();
    } else {
        QMessageBox::warning(this, "Error", "Failed to eject device");
    }
}
