#include "MainWindow.h"
#include "../utils/Utils.h"
#include "../utils/Validation.h"
#include "../core/FileSystemManager.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QSplitter>
#include <QTimer>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QDateTime>
#include <QRegularExpression>
#include <QTextCursor>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_logVisible(false)
    , m_isBurning(false)
    , m_advancedVisible(false)
    , m_deviceManager(new DeviceManager(this))
    , m_imageHandler(new ImageHandler(this))
    , m_burner(new Burner(this))
    , m_updateTimer(new QTimer(this))
{
    setWindowTitle("Linux Image Burner v1.0");
    setWindowIcon(QIcon(":/icons/app-icon.png"));
    resize(800, 600);
    
    setupUi();
    setupMenuBar();
    setupStatusBar();
    setupConnections();
    
    // Initialize device list
    m_deviceManager->startMonitoring();
    updateDeviceList();
    
    // Setup update timer
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::validateInputs);
    m_updateTimer->start();
    
    logMessage("Application started", "INFO");
}

MainWindow::~MainWindow()
{
    if (m_deviceManager) {
        m_deviceManager->stopMonitoring();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isBurning) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Confirm Exit",
            "A burn operation is in progress. Are you sure you want to exit?",
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
        
        m_burner->cancel();
    }
    
    event->accept();
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    // Create main splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, this);
    
    // Create main content widget
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    
    // Image selection group
    m_imageGroup = new QGroupBox("Image");
    QHBoxLayout *imageLayout = new QHBoxLayout(m_imageGroup);
    
    m_imageLabel = new QLabel("No image selected");
    m_imageLabel->setStyleSheet("QLabel { border: 1px solid gray; padding: 5px; }");
    m_selectImageButton = new QPushButton("Select Image...");
    m_imageInfoLabel = new QLabel("Select an image file to see details");
    m_imageInfoLabel->setWordWrap(true);
    
    imageLayout->addWidget(m_imageLabel, 1);
    imageLayout->addWidget(m_selectImageButton);
    imageLayout->addWidget(m_imageInfoLabel, 1);
    
    // Device selection group
    m_deviceGroup = new QGroupBox("Device");
    QGridLayout *deviceLayout = new QGridLayout(m_deviceGroup);
    
    deviceLayout->addWidget(new QLabel("Device:"), 0, 0);
    m_deviceCombo = new QComboBox();
    m_deviceCombo->setMinimumWidth(200);
    deviceLayout->addWidget(m_deviceCombo, 0, 1);
    
    m_refreshButton = new QPushButton("Refresh");
    deviceLayout->addWidget(m_refreshButton, 0, 2);
    
    m_deviceInfoButton = new QPushButton("Device Info");
    deviceLayout->addWidget(m_deviceInfoButton, 0, 3);
    
    m_deviceInfoLabel = new QLabel("Select a device to see details");
    m_deviceInfoLabel->setWordWrap(true);
    deviceLayout->addWidget(m_deviceInfoLabel, 1, 0, 1, 4);
    
    // File system options group
    m_fileSystemGroup = new QGroupBox("File System Options");
    QGridLayout *fsLayout = new QGridLayout(m_fileSystemGroup);
    
    fsLayout->addWidget(new QLabel("File System:"), 0, 0);
    m_fileSystemCombo = new QComboBox();
    m_fileSystemCombo->addItems({"FAT32", "NTFS", "exFAT", "ext4"});
    fsLayout->addWidget(m_fileSystemCombo, 0, 1);
    
    fsLayout->addWidget(new QLabel("Volume Label:"), 1, 0);
    m_volumeLabelEdit = new QLineEdit();
    m_volumeLabelEdit->setPlaceholderText("Optional volume label");
    fsLayout->addWidget(m_volumeLabelEdit, 1, 1);
    
    fsLayout->addWidget(new QLabel("Cluster Size:"), 0, 2);
    m_clusterSizeCombo = new QComboBox();
    fsLayout->addWidget(m_clusterSizeCombo, 0, 3);
    
    fsLayout->addWidget(new QLabel("Partition Scheme:"), 1, 2);
    m_partitionSchemeCombo = new QComboBox();
    m_partitionSchemeCombo->addItems({"MBR", "GPT"});
    fsLayout->addWidget(m_partitionSchemeCombo, 1, 3);
    
    // Advanced options group (initially hidden)
    m_advancedGroup = new QGroupBox("Advanced Options");
    m_advancedGroup->setVisible(false);
    QVBoxLayout *advancedLayout = new QVBoxLayout(m_advancedGroup);
    
    m_quickFormatCheck = new QCheckBox("Quick Format");
    m_quickFormatCheck->setChecked(true);
    advancedLayout->addWidget(m_quickFormatCheck);
    
    m_verifyCheck = new QCheckBox("Verify after burning");
    advancedLayout->addWidget(m_verifyCheck);
    
    m_createBootableCheck = new QCheckBox("Create bootable USB");
    m_createBootableCheck->setChecked(true);
    advancedLayout->addWidget(m_createBootableCheck);
    
    m_badBlockCheck = new QCheckBox("Check for bad blocks");
    advancedLayout->addWidget(m_badBlockCheck);
    
    // Progress group
    m_progressGroup = new QGroupBox("Progress");
    QVBoxLayout *progressLayout = new QVBoxLayout(m_progressGroup);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    progressLayout->addWidget(m_progressBar);
    
    QHBoxLayout *progressInfoLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Ready");
    m_speedLabel = new QLabel("");
    m_timeLabel = new QLabel("");
    
    progressInfoLayout->addWidget(m_statusLabel);
    progressInfoLayout->addStretch();
    progressInfoLayout->addWidget(m_speedLabel);
    progressInfoLayout->addWidget(m_timeLabel);
    
    progressLayout->addLayout(progressInfoLayout);
    
    // Action buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_startButton = new QPushButton("Start");
    m_startButton->setDefault(true);
    m_startButton->setMinimumHeight(35);
    
    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setEnabled(false);
    m_cancelButton->setMinimumHeight(35);
    
    m_formatButton = new QPushButton("Format Device");
    m_formatButton->setMinimumHeight(35);
    
    m_advancedToggle = new QPushButton("Show Advanced Options");
    m_advancedToggle->setMinimumHeight(35);
    
    m_logButton = new QPushButton("Show Log");
    m_logButton->setMinimumHeight(35);
    
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_formatButton);
    buttonLayout->addWidget(m_advancedToggle);
    buttonLayout->addWidget(m_logButton);
    
    // Add all groups to main layout
    mainLayout->addWidget(m_imageGroup);
    mainLayout->addWidget(m_deviceGroup);
    mainLayout->addWidget(m_fileSystemGroup);
    mainLayout->addWidget(m_advancedGroup);
    mainLayout->addWidget(m_progressGroup);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
    
    // Create log text widget
    m_logText = new QTextEdit();
    m_logText->setMaximumHeight(150);
    m_logText->setReadOnly(true);
    m_logText->setVisible(false);
    
    // Add to splitter
    mainSplitter->addWidget(contentWidget);
    mainSplitter->addWidget(m_logText);
    mainSplitter->setSizes({500, 100});
    
    // Set main layout
    QVBoxLayout *centralLayout = new QVBoxLayout(m_centralWidget);
    centralLayout->addWidget(mainSplitter);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    QAction *selectImageAction = fileMenu->addAction("&Select Image...");
    selectImageAction->setShortcut(QKeySequence::Open);
    connect(selectImageAction, &QAction::triggered, this, &MainWindow::selectImage);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    QMenu *deviceMenu = menuBar()->addMenu("&Device");
    
    QAction *refreshAction = deviceMenu->addAction("&Refresh Devices");
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshDevices);
    
    QAction *deviceInfoAction = deviceMenu->addAction("Device &Info...");
    connect(deviceInfoAction, &QAction::triggered, this, &MainWindow::showDeviceInfo);
    
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    
    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::setupStatusBar()
{
    m_statusBarLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusBarLabel);
    
    // Add system info
    QString sysInfo = QString("Linux | %1 | Root: %2")
                     .arg(Utils::getSystemArchitecture())
                     .arg(Utils::isRunningAsRoot() ? "Yes" : "No");
    statusBar()->addPermanentWidget(new QLabel(sysInfo));
}

void MainWindow::setupConnections()
{
    // Image selection
    connect(m_selectImageButton, &QPushButton::clicked, this, &MainWindow::selectImage);
    
    // Device management
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshDevices);
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::deviceSelectionChanged);
    connect(m_deviceInfoButton, &QPushButton::clicked, this, &MainWindow::showDeviceInfo);
    
    // File system options
    connect(m_fileSystemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::fileSystemChanged);
    
    // Actions
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startBurn);
    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::cancelBurn);
    connect(m_formatButton, &QPushButton::clicked, this, &MainWindow::formatDevice);
    connect(m_advancedToggle, &QPushButton::clicked, this, &MainWindow::toggleAdvancedOptions);
    connect(m_logButton, &QPushButton::clicked, this, &MainWindow::showLog);
    
    // Device manager
    connect(m_deviceManager, &DeviceManager::deviceListChanged,
            this, &MainWindow::onDeviceListChanged);
    connect(m_deviceManager, &DeviceManager::deviceInserted,
            this, &MainWindow::onDeviceInserted);
    connect(m_deviceManager, &DeviceManager::deviceRemoved,
            this, &MainWindow::onDeviceRemoved);
    
    // Burner
    connect(m_burner, &Burner::burnStarted, this, &MainWindow::onBurnStarted);
    connect(m_burner, &Burner::burnFinished, this, &MainWindow::onBurnFinished);
    connect(m_burner, &Burner::progressChanged, this, &MainWindow::onProgressChanged);
    connect(m_burner, &Burner::speedChanged, this, &MainWindow::onSpeedChanged);
    connect(m_burner, &Burner::statusChanged, this, &MainWindow::onStatusChanged);
    connect(m_burner, &Burner::timeRemainingChanged, this, &MainWindow::onTimeRemainingChanged);
    connect(m_burner, &Burner::error, this, &MainWindow::onBurnerError);
}

void MainWindow::selectImage()
{
    QStringList filters;
    filters << "All Supported Images (*.iso *.img *.dmg *.vhd *.vhdx *.vmdk)"
            << "ISO Images (*.iso)"
            << "IMG Images (*.img)"
            << "DMG Images (*.dmg)"  
            << "VHD Images (*.vhd *.vhdx)"
            << "VMDK Images (*.vmdk)"
            << "All Files (*)";
    
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Image File",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        filters.join(";;")
    );
    
    if (!fileName.isEmpty()) {
        m_selectedImagePath = fileName;
        m_imageLabel->setText(Utils::getFileName(fileName));
        updateImageInfo();
        validateInputs();
        
        logMessage("Selected image: " + fileName, "INFO");
    }
}

void MainWindow::refreshDevices()
{
    logMessage("Refreshing device list", "INFO");
    updateDeviceList();
}

void MainWindow::updateDeviceList()
{
    m_deviceCombo->clear();
    
    QList<DeviceInfo> devices = m_deviceManager->getRemovableDevices();
    
    if (devices.isEmpty()) {
        m_deviceCombo->addItem("No devices found");
        m_deviceInfoLabel->setText("No removable devices detected");
    } else {
        for (const DeviceInfo &device : devices) {
            QString displayText = QString("%1 (%2) - %3")
                                 .arg(device.name)
                                 .arg(device.sizeString)
                                 .arg(device.model.isEmpty() ? "Unknown" : device.model);
            
            m_deviceCombo->addItem(displayText, device.path);
        }
    }
    
    deviceSelectionChanged();
}

void MainWindow::deviceSelectionChanged()
{
    QString devicePath = m_deviceCombo->currentData().toString();
    m_selectedDevicePath = devicePath;
    
    if (!devicePath.isEmpty()) {
        DeviceInfo info = m_deviceManager->getDeviceInfo(devicePath);
        
        QString infoText = QString("Device: %1\nSize: %2\nModel: %3\nVendor: %4\nFile System: %5")
                          .arg(info.path)
                          .arg(info.sizeString)
                          .arg(info.model.isEmpty() ? "Unknown" : info.model)
                          .arg(info.vendor.isEmpty() ? "Unknown" : info.vendor)
                          .arg(info.fileSystem.isEmpty() ? "Unknown" : info.fileSystem);
        
        if (info.isMounted) {
            infoText += QString("\nMounted at: %1").arg(info.mountPoints.join(", "));
        }
        
        m_deviceInfoLabel->setText(infoText);
        
        // Update cluster size options based on device size
        fileSystemChanged();
    } else {
        m_deviceInfoLabel->setText("No device selected");
    }
    
    validateInputs();
}

void MainWindow::fileSystemChanged()
{
    QString fsType = m_fileSystemCombo->currentText();
    m_clusterSizeCombo->clear();
    
    if (!m_selectedDevicePath.isEmpty()) {
        DeviceInfo info = m_deviceManager->getDeviceInfo(m_selectedDevicePath);
        QStringList clusterSizes = FileSystemManager::getAvailableClusterSizes(fsType, info.size);
        m_clusterSizeCombo->addItems(clusterSizes);
        
        // Set recommended cluster size
        int recommended = FileSystemManager::getRecommendedClusterSize(fsType, info.size);
        for (int i = 0; i < m_clusterSizeCombo->count(); ++i) {
            QString text = m_clusterSizeCombo->itemText(i);
            if (text.contains(QString::number(recommended))) {
                m_clusterSizeCombo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void MainWindow::updateImageInfo()
{
    if (m_selectedImagePath.isEmpty()) {
        m_imageInfoLabel->setText("Select an image file to see details");
        return;
    }
    
    ImageInfo info = m_imageHandler->analyzeImage(m_selectedImagePath);
    
    if (info.isValid) {
        QString infoText = QString("Size: %1\nType: %2\nBootable: %3")
                          .arg(info.sizeString)
                          .arg(ImageHandler::imageTypeToString(info.type))
                          .arg(info.isBootable ? "Yes" : "No");
        
        if (!info.label.isEmpty()) {
            infoText += QString("\nLabel: %1").arg(info.label);
        }
        
        if (!info.architecture.isEmpty()) {
            infoText += QString("\nArchitecture: %1").arg(info.architecture);
        }
        
        if (!info.bootLoaders.isEmpty()) {
            infoText += QString("\nBoot Loaders: %1").arg(info.bootLoaders.join(", "));
        }
        
        m_imageInfoLabel->setText(infoText);
    } else {
        m_imageInfoLabel->setText("Error: " + info.errorMessage);
    }
}

void MainWindow::validateInputs()
{
    bool canStart = !m_selectedImagePath.isEmpty() && 
                   !m_selectedDevicePath.isEmpty() &&
                   !m_isBurning;
    
    m_startButton->setEnabled(canStart);
    m_formatButton->setEnabled(!m_selectedDevicePath.isEmpty() && !m_isBurning);
    m_deviceInfoButton->setEnabled(!m_selectedDevicePath.isEmpty());
    
    // Show warnings if needed
    if (!m_selectedDevicePath.isEmpty()) {
        QStringList warnings = Validation::getWarnings(m_selectedDevicePath);
        if (!warnings.isEmpty()) {
            m_statusBarLabel->setText("Warning: " + warnings.join("; "));
        } else if (canStart) {
            m_statusBarLabel->setText("Ready to burn");
        }
    }
}

void MainWindow::startBurn()
{
    if (!Validation::validateBurnOptions(m_selectedImagePath, m_selectedDevicePath,
                                        m_fileSystemCombo->currentText(),
                                        m_volumeLabelEdit->text())) {
        QStringList errors = Validation::getBurnOptionsErrors(m_selectedImagePath,
                                                             m_selectedDevicePath,
                                                             m_fileSystemCombo->currentText(),
                                                             m_volumeLabelEdit->text());
        
        QMessageBox::warning(this, "Validation Error", 
                           "Cannot start burn operation:\n\n" + errors.join("\n"));
        return;
    }
    
    // Show confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, "Confirm Burn Operation",
        QString("This will completely erase all data on %1.\n\n"
                "Are you sure you want to continue?")
                .arg(m_selectedDevicePath),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    BurnOptions options = getBurnOptions();
    m_burner->burnImage(options);
    
    logMessage("Started burn operation", "INFO");
}

void MainWindow::cancelBurn()
{
    m_burner->cancel();
    logMessage("Burn operation cancelled", "INFO");
}

void MainWindow::formatDevice()
{
    if (m_selectedDevicePath.isEmpty()) {
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, "Confirm Format",
        QString("This will format %1 and erase all data.\n\n"
                "Are you sure you want to continue?")
                .arg(m_selectedDevicePath),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    QString fsType = m_fileSystemCombo->currentText();
    QString label = m_volumeLabelEdit->text();
    
    FileSystem fs = FileSystem::FAT32;
    if (fsType == "NTFS") fs = FileSystem::NTFS;
    else if (fsType == "exFAT") fs = FileSystem::exFAT;
    else if (fsType == "ext4") fs = FileSystem::ext4;
    
    m_burner->formatDevice(m_selectedDevicePath, fs, label);
    
    logMessage("Started format operation", "INFO");
}

void MainWindow::showDeviceInfo()
{
    // This would open a detailed device info dialog
    // For now, just show a message box with device info
    if (!m_selectedDevicePath.isEmpty()) {
        DeviceInfo info = m_deviceManager->getDeviceInfo(m_selectedDevicePath);
        
        QString detailedInfo = QString(
            "Device Path: %1\n"
            "Model: %2\n"
            "Vendor: %3\n"
            "Size: %4 (%5 bytes)\n"
            "File System: %6\n"
            "UUID: %7\n"
            "Removable: %8\n"
            "USB Device: %9\n"
            "MMC Device: %10\n"
            "Mounted: %11"
        ).arg(info.path)
         .arg(info.model.isEmpty() ? "Unknown" : info.model)
         .arg(info.vendor.isEmpty() ? "Unknown" : info.vendor)
         .arg(info.sizeString)
         .arg(info.size)
         .arg(info.fileSystem.isEmpty() ? "Unknown" : info.fileSystem)
         .arg(info.uuid.isEmpty() ? "None" : info.uuid)
         .arg(info.isRemovable ? "Yes" : "No")
         .arg(info.isUSB ? "Yes" : "No")
         .arg(info.isMMC ? "Yes" : "No")
         .arg(info.isMounted ? "Yes (" + info.mountPoints.join(", ") + ")" : "No");
        
        QMessageBox::information(this, "Device Information", detailedInfo);
    }
}

void MainWindow::showAbout()
{
    QString aboutText = QString(
        "<h3>Linux Image Burner v1.0</h3>"
        "<p>A full-featured USB/DVD image burning tool for Linux</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Support for ISO, IMG, DMG, VHD, VMDK formats</li>"
        "<li>Multiple file systems: FAT32, NTFS, exFAT, ext4</li>"
        "<li>Bootable USB creation</li>"
        "<li>Device verification</li>"
        "<li>Progress monitoring</li>"
        "</ul>"
        "<p><b>System Information:</b></p>"
        "<p>Architecture: %1<br>"
        "Kernel: %2<br>"
        "Distribution: %3</p>"
        "<p>Built with Qt %4</p>"
    ).arg(Utils::getSystemArchitecture())
     .arg(Utils::getKernelVersion())
     .arg(Utils::getDistributionName())
     .arg(QT_VERSION_STR);
    
    QMessageBox::about(this, "About Linux Image Burner", aboutText);
}

void MainWindow::showLog()
{
    m_logVisible = !m_logVisible;
    m_logText->setVisible(m_logVisible);
    m_logButton->setText(m_logVisible ? "Hide Log" : "Show Log");
}

void MainWindow::toggleAdvancedOptions()
{
    m_advancedVisible = !m_advancedVisible;
    m_advancedGroup->setVisible(m_advancedVisible);
    m_advancedToggle->setText(m_advancedVisible ? "Hide Advanced Options" : "Show Advanced Options");
}

// Burner event handlers
void MainWindow::onBurnStarted()
{
    m_isBurning = true;
    m_startButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    m_formatButton->setEnabled(false);
    m_selectImageButton->setEnabled(false);
    m_refreshButton->setEnabled(false);
    
    m_progressBar->setValue(0);
    m_statusLabel->setText("Starting burn...");
    m_speedLabel->clear();
    m_timeLabel->clear();
    
    logMessage("Burn started", "INFO");
}

void MainWindow::onBurnFinished(bool success, const QString &message)
{
    m_isBurning = false;
    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    m_formatButton->setEnabled(true);
    m_selectImageButton->setEnabled(true);
    m_refreshButton->setEnabled(true);
    
    if (success) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("Burn completed successfully");
        QMessageBox::information(this, "Burn Complete", 
                               "The image has been successfully burned to the device.");
        logMessage("Burn completed successfully", "SUCCESS");
    } else {
        m_statusLabel->setText("Burn failed");
        QMessageBox::critical(this, "Burn Failed", 
                            "The burn operation failed:\n\n" + message);
        logMessage("Burn failed: " + message, "ERROR");
    }
    
    validateInputs();
}

void MainWindow::onProgressChanged(int percentage)
{
    m_progressBar->setValue(percentage);
}

void MainWindow::onSpeedChanged(const QString &speed)
{
    m_speedLabel->setText(speed);
}

void MainWindow::onStatusChanged(const QString &status)
{
    m_statusLabel->setText(status);
    logMessage("Status: " + status, "INFO");
}

void MainWindow::onTimeRemainingChanged(const QString &timeRemaining)
{
    m_timeLabel->setText("Time remaining: " + timeRemaining);
}

void MainWindow::onBurnerError(const QString &message)
{
    logMessage("Burner error: " + message, "ERROR");
    QMessageBox::critical(this, "Burn Error", message);
}

// Device manager event handlers
void MainWindow::onDeviceListChanged()
{
    updateDeviceList();
    logMessage("Device list changed", "INFO");
}

void MainWindow::onDeviceInserted(const QString &devicePath)
{
    updateDeviceList();
    logMessage("Device inserted: " + devicePath, "INFO");
}

void MainWindow::onDeviceRemoved(const QString &devicePath)
{
    updateDeviceList();
    logMessage("Device removed: " + devicePath, "INFO");
}

void MainWindow::logMessage(const QString &message, const QString &level)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2: %3").arg(timestamp, level, message);
    
    m_logText->append(logEntry);
    
    // Keep log size manageable
    if (m_logText->document()->blockCount() > 1000) {
        QTextCursor cursor = m_logText->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 100);
        cursor.removeSelectedText();
    }
}

BurnOptions MainWindow::getBurnOptions()
{
    BurnOptions options;
    
    options.imagePath = m_selectedImagePath;
    options.devicePath = m_selectedDevicePath;
    options.mode = BurnMode::DDMode; // Default mode
    
    // Parse partition scheme
    QString partitionScheme = m_partitionSchemeCombo->currentText();
    options.partitionScheme = (partitionScheme == "GPT") ? PartitionScheme::GPT : PartitionScheme::MBR;
    
    // Parse file system
    QString fsType = m_fileSystemCombo->currentText();
    if (fsType == "FAT32") options.fileSystem = FileSystem::FAT32;
    else if (fsType == "NTFS") options.fileSystem = FileSystem::NTFS;
    else if (fsType == "exFAT") options.fileSystem = FileSystem::exFAT;
    else if (fsType == "ext4") options.fileSystem = FileSystem::ext4;
    else options.fileSystem = FileSystem::FAT32;
    
    options.volumeLabel = m_volumeLabelEdit->text();
    options.quickFormat = m_quickFormatCheck->isChecked();
    options.verifyAfterBurn = m_verifyCheck->isChecked();
    options.createBootableUSB = m_createBootableCheck->isChecked();
    options.badBlockCheck = m_badBlockCheck->isChecked();
    
    // Parse cluster size
    QString clusterSizeText = m_clusterSizeCombo->currentText();
    QRegularExpression re("(\\d+)");
    QRegularExpressionMatch match = re.match(clusterSizeText);
    if (match.hasMatch()) {
        options.clusterSize = match.captured(1).toInt();
    } else {
        options.clusterSize = 4096; // Default 4KB
    }
    
    return options;
}
