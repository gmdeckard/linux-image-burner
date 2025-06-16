#include "Burner.h"
#include "DeviceManager.h"
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>
#include <QThread>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QMutex>
#include <QMutexLocker>
#include <unistd.h>
#include <sys/statvfs.h>

Burner::Burner(QObject *parent)
    : QObject(parent)
    , m_isBurning(false)
    , m_isPaused(false)
    , m_isCancelled(false)
    , m_process(nullptr)
    , m_progressTimer(new QTimer(this))
    , m_totalBytes(0)
    , m_bytesWritten(0)
    , m_lastBytesWritten(0)
    , m_lastUpdateTime()
{
    m_progressTimer->setInterval(1000); // Update every second
    connect(m_progressTimer, &QTimer::timeout, this, &Burner::onProgressTimer);
}

Burner::~Burner()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void Burner::burnImage(const BurnOptions &options)
{
    if (m_isBurning) {
        emit error("Burn operation already in progress");
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    m_currentOptions = options;
    m_isBurning = true;
    m_isPaused = false;
    m_isCancelled = false;
    m_bytesWritten = 0;
    m_lastBytesWritten = 0;
    
    // Get image size
    QFileInfo imageInfo(options.imagePath);
    if (!imageInfo.exists()) {
        emit error("Image file does not exist");
        m_isBurning = false;
        return;
    }
    
    m_totalBytes = imageInfo.size();
    m_startTime = QDateTime::currentDateTime();
    m_lastUpdateTime = m_startTime;
    
    emit burnStarted();
    emit statusChanged("Preparing device...");
    
    // Prepare the device
    if (!prepareDevice(options.devicePath, options)) {
        emit error("Failed to prepare device");
        m_isBurning = false;
        return;
    }
    
    // Start burning based on mode
    bool success = false;
    switch (options.mode) {
        case BurnMode::DDMode:
            success = burnWithDD(options);
            break;
        case BurnMode::UEFIMode:
            success = burnWithUEFI(options);
            break;
        case BurnMode::WindowsToGo:
            success = burnWithWindowsToGo(options);
            break;
        default:
            success = burnWithDD(options);
            break;
    }
    
    if (!success) {
        emit error("Failed to start burn operation");
        m_isBurning = false;
        return;
    }
    
    m_progressTimer->start();
}

void Burner::formatDevice(const QString &devicePath, FileSystem fs, const QString &label)
{
    if (m_isBurning) {
        emit error("Operation already in progress");
        return;
    }
    
    emit statusChanged("Formatting device...");
    
    // Unmount device first
    if (!unmountDevice(devicePath)) {
        emit error("Failed to unmount device");
        return;
    }
    
    // Create partition table
    if (!createPartitionTable(devicePath, PartitionScheme::MBR)) {
        emit error("Failed to create partition table");
        return;
    }
    
    // Create and format partition
    if (!createPartition(devicePath, fs, label)) {
        emit error("Failed to create partition");
        return;
    }
    
    emit burnFinished(true, "Device formatted successfully");
}

void Burner::verifyBurn(const QString &imagePath, const QString &devicePath)
{
    emit verificationStarted();
    emit statusChanged("Verifying burn...");
    
    bool success = verifyImageChecksum(imagePath, devicePath);
    
    emit verificationFinished(success, success ? "Verification successful" : "Verification failed");
}

void Burner::cancel()
{
    QMutexLocker locker(&m_mutex);
    
    m_isCancelled = true;
    
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
    
    // Clean up temporary script files
    QDir tmpDir("/tmp");
    QStringList scriptFiles = tmpDir.entryList(QStringList() << "burn_script_*.sh", QDir::Files);
    for (const QString &file : scriptFiles) {
        QFile::remove("/tmp/" + file);
    }
    
    m_progressTimer->stop();
    emit statusChanged("Cancelled");
    m_isBurning = false;
}

void Burner::pause()
{
    if (!m_isBurning || m_isPaused) {
        return;
    }
    
    m_isPaused = true;
    
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->kill(); // Send SIGTERM to pause
    }
    
    emit statusChanged("Paused");
}

void Burner::resume()
{
    if (!m_isBurning || !m_isPaused) {
        return;
    }
    
    m_isPaused = false;
    
    // Restart the process from where it left off
    // This is a simplified implementation - in practice, you'd need to track progress more carefully
    burnWithDD(m_currentOptions);
    
    emit statusChanged("Resuming...");
}

void Burner::onProgressTimer()
{
    if (!m_isBurning || m_isPaused) {
        return;
    }
    
    updateProgress();
}

void Burner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    
    m_progressTimer->stop();
    
    // Ensure we show 100% when process completes successfully
    if (exitCode == 0) {
        emit progressChanged(100);
        emit statusChanged("USB burning completed successfully!");
    }
    
    // Clean up temporary script files
    QDir tmpDir("/tmp");
    QStringList scriptFiles = tmpDir.entryList(QStringList() << "burn_script_*.sh", QDir::Files);
    for (const QString &file : scriptFiles) {
        QFile::remove("/tmp/" + file);
    }
    
    if (m_isCancelled) {
        emit burnFinished(false, "Operation cancelled");
    } else if (exitCode == 0) {
        // Note: We don't call syncDevice here as sync is already done in the script
        
        if (m_currentOptions.verifyAfterBurn) {
            verifyBurn(m_currentOptions.imagePath, m_currentOptions.devicePath);
        } else {
            emit burnFinished(true, "Burn completed successfully");
        }
    } else {
        emit burnFinished(false, "Burn failed with exit code " + QString::number(exitCode));
    }
    
    m_isBurning = false;
}

void Burner::onProcessError(QProcess::ProcessError error)
{
    QString errorMessage;
    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "Failed to start process";
            break;
        case QProcess::Crashed:
            errorMessage = "Process crashed";
            break;
        case QProcess::Timedout:
            errorMessage = "Process timed out";
            break;
        default:
            errorMessage = "Unknown process error";
            break;
    }
    
    emit this->error(errorMessage);
    m_isBurning = false;
    m_progressTimer->stop();
}

void Burner::onProcessOutput()
{
    if (!m_process) return;
    
    // Read from both stderr and stdout
    QByteArray stderrData = m_process->readAllStandardError();
    QByteArray stdoutData = m_process->readAllStandardOutput();
    
    QString output = QString::fromUtf8(stderrData) + QString::fromUtf8(stdoutData);
    
    if (!output.trimmed().isEmpty()) {
        qDebug() << "DD Output:" << output; // Debug output to see actual format
    }
    
    // Parse dd progress output with status=progress
    // DD outputs progress in different formats:
    // Format 1: "512000000 bytes (512 MB, 488 MiB) copied, 12.3456 s, 41.5 MB/s"
    // Format 2: "512+0 records in\n512+0 records out\n537919488 bytes (538 MB, 513 MiB) copied"
    // Format 3: Just the bytes on a line by itself when using status=progress
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) continue;
        
        // Try multiple regex patterns to catch different dd output formats
        QRegularExpression patterns[] = {
            QRegularExpression(R"((\d+)\s+bytes\s+\([^)]+\)\s+copied)"), // Standard format: "104857600 bytes (105 MB, 100 MiB) copied"
            QRegularExpression(R"((\d+)\s+bytes.*copied)"),               // Basic format: "104857600 bytes copied"
            QRegularExpression(R"(^(\d+)\s+bytes)"),                      // Simple: "104857600 bytes"
            QRegularExpression(R"((\d+)\+\d+\s+records\s+out)"),          // Records format: "100+0 records out"
        };
        
        bool foundMatch = false;
        qint64 bytes = 0;
        
        for (const auto &regex : patterns) {
            QRegularExpressionMatch match = regex.match(trimmedLine);
            if (match.hasMatch()) {
                bool ok = false;
                bytes = match.captured(1).toLongLong(&ok);
                if (ok && bytes > 0) {
                    foundMatch = true;
                    break;
                }
            }
        }
        
        if (foundMatch) {
            QMutexLocker locker(&m_mutex);
            
            // Only update if we got a reasonable value (not going backwards)
            if (bytes >= m_bytesWritten) {
                m_bytesWritten = bytes;
                
                // Calculate and emit progress
                int percentage = 0;
                if (m_totalBytes > 0) {
                    percentage = (int)((m_bytesWritten * 100) / m_totalBytes);
                    // Cap progress at 95% until sync is complete
                    // This prevents showing 100% while sync is still running
                    percentage = qMin(percentage, 95);
                }
                
                qDebug() << "Progress update:" << m_bytesWritten << "of" << m_totalBytes << "(" << percentage << "%)";
                
                emit progressChanged(percentage);
                
                // Calculate speed and time remaining
                QDateTime currentTime = QDateTime::currentDateTime();
                if (!m_lastUpdateTime.isValid()) {
                    m_lastUpdateTime = currentTime;
                    m_lastBytesWritten = 0;
                }
                
                qint64 timeDiff = m_lastUpdateTime.msecsTo(currentTime);
                
                if (timeDiff > 500) { // Update every 500ms to avoid too frequent updates
                    qint64 bytesDiff = m_bytesWritten - m_lastBytesWritten;
                    if (bytesDiff > 0 && timeDiff > 0) {
                        QString speed = calculateSpeed(bytesDiff, timeDiff);
                        emit speedChanged(speed);
                        
                        // Calculate time remaining
                        qint64 bytesRemaining = m_totalBytes - m_bytesWritten;
                        double speedBytesPerSec = (double)bytesDiff / (timeDiff / 1000.0);
                        if (speedBytesPerSec > 0) {
                            QString timeRemaining = calculateTimeRemaining(bytesRemaining, speedBytesPerSec);
                            emit timeRemainingChanged(timeRemaining);
                        }
                    }
                    
                    m_lastUpdateTime = currentTime;
                    m_lastBytesWritten = m_bytesWritten;
                }
            }
        }
        
        // Emit status updates for lines containing useful information
        if (trimmedLine.contains("bytes") || trimmedLine.contains("copied") || 
            trimmedLine.contains("records")) {
            emit statusChanged(QString("Writing... %1").arg(trimmedLine));
        }
        
        // Detect sync phase
        if (trimmedLine.contains("Syncing device")) {
            emit statusChanged("Syncing device - finalizing USB drive...");
            emit progressChanged(98); // Show 98% during sync
        }
        
        // Detect completion
        if (trimmedLine.contains("Burn operation completed successfully")) {
            emit statusChanged("USB burning completed successfully!");
            emit progressChanged(100); // Only now show 100%
        }
    }
}

bool Burner::prepareDevice(const QString &devicePath, const BurnOptions &options)
{
    // Unmount all partitions
    if (!unmountDevice(devicePath)) {
        return false;
    }
    
    // For DD mode, no preparation needed
    if (options.mode == BurnMode::DDMode) {
        return true;
    }
    
    // Create partition table
    if (!createPartitionTable(devicePath, options.partitionScheme)) {
        return false;
    }
    
    return true;
}

bool Burner::createPartitionTable(const QString &devicePath, PartitionScheme scheme)
{
    QString schemeStr = (scheme == PartitionScheme::GPT) ? "gpt" : "msdos";
    
    QProcess parted;
    parted.start("parted", QStringList() << devicePath << "--script" << "mklabel" << schemeStr);
    parted.waitForFinished(10000);
    
    return parted.exitCode() == 0;
}

bool Burner::createPartition(const QString &devicePath, FileSystem fs, const QString &label)
{
    // Create primary partition
    QProcess parted;
    parted.start("parted", QStringList() << devicePath << "--script" << "mkpart" << "primary" << "0%" << "100%");
    parted.waitForFinished(10000);
    
    if (parted.exitCode() != 0) {
        return false;
    }
    
    // Format the partition
    QString partitionPath = getPartitionPath(devicePath, 1);
    return formatPartition(partitionPath, fs, label);
}

bool Burner::formatPartition(const QString &partitionPath, FileSystem fs, const QString &label)
{
    QString command = getFileSystemCommand(fs);
    QStringList args = getFormatArguments(fs, label, m_currentOptions.clusterSize);
    args << partitionPath;
    
    QProcess format;
    format.start(command, args);
    format.waitForFinished(30000);
    
    return format.exitCode() == 0;
}

bool Burner::burnWithDD(const BurnOptions &options)
{
    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Burner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &Burner::onProcessError);
    connect(m_process, &QProcess::readyReadStandardError, this, &Burner::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &Burner::onProcessOutput);
    
    emit statusChanged("Preparing to write image to device...");
    
    // First check if pkexec is available
    QProcess pkexecCheck;
    pkexecCheck.start("which", QStringList() << "pkexec");
    pkexecCheck.waitForFinished(3000);
    
    if (pkexecCheck.exitCode() != 0) {
        emit error("pkexec not found. Please install policykit-1 package or run as root.");
        return false;
    }
    
    // Create a script that will be executed with pkexec
    QString scriptPath = QString("/tmp/burn_script_%1.sh").arg(QDateTime::currentMSecsSinceEpoch());
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit error("Failed to create temporary script. Check /tmp directory permissions.");
        return false;
    }
    
    QTextStream out(&scriptFile);
    out << "#!/bin/bash\n";
    out << "set -e\n";
    out << "# Linux Image Burner - Burn Script\n";
    out << QString("echo 'Starting burn operation: %1 -> %2'\n")
           .arg(QFileInfo(options.imagePath).fileName()).arg(options.devicePath);
    out << "# Ensure progress output is not buffered\n";
    out << "export LC_ALL=C\n";
    out << QString("dd if='%1' of='%2' bs=1M conv=fdatasync status=progress oflag=direct 2>&1 | tee /dev/stderr\n")
           .arg(options.imagePath).arg(options.devicePath);
    out << "echo 'Syncing device...'\n";
    out << "sync\n";
    out << "echo 'Burn operation completed successfully'\n";
    scriptFile.close();
    
    // Make script executable
    QProcess chmod;
    chmod.start("chmod", QStringList() << "+x" << scriptPath);
    chmod.waitForFinished();
    
    if (chmod.exitCode() != 0) {
        emit error("Failed to make script executable");
        QFile::remove(scriptPath);
        return false;
    }
    
    emit statusChanged("Requesting administrator privileges...");
    
    // Execute with pkexec
    QStringList args;
    args << scriptPath;
    
    m_process->start("pkexec", args);
    
    if (!m_process->waitForStarted(10000)) {
        emit error("Failed to start burning process. User may have cancelled authentication.");
        QFile::remove(scriptPath);
        return false;
    }
    
    emit statusChanged("Writing image to device...");
    return true;
}

bool Burner::burnWithUEFI(const BurnOptions &options)
{
    // For UEFI mode, we need to:
    // 1. Create GPT partition table
    // 2. Create EFI system partition
    // 3. Copy image contents
    
    if (!createPartitionTable(options.devicePath, PartitionScheme::GPT)) {
        return false;
    }
    
    // Create EFI system partition
    QProcess parted;
    parted.start("parted", QStringList() << options.devicePath << "--script" 
                 << "mkpart" << "ESP" << "fat32" << "1MiB" << "100%");
    parted.waitForFinished(10000);
    
    if (parted.exitCode() != 0) {
        return false;
    }
    
    // Set boot flag
    QProcess bootFlag;
    bootFlag.start("parted", QStringList() << options.devicePath << "--script" << "set" << "1" << "boot" << "on");
    bootFlag.waitForFinished(5000);
    
    // Format as FAT32
    QString partitionPath = getPartitionPath(options.devicePath, 1);
    if (!formatPartition(partitionPath, FileSystem::FAT32, options.volumeLabel)) {
        return false;
    }
    
    // Now use dd to write the image
    return burnWithDD(options);
}

bool Burner::burnWithWindowsToGo(const BurnOptions &options)
{
    // Windows To Go requires special handling
    // This is a simplified implementation
    return burnWithUEFI(options);
}

bool Burner::addBootFiles(const QString &devicePath, const BurnOptions &options)
{
    Q_UNUSED(devicePath)
    Q_UNUSED(options)
    // Implementation for adding additional boot files
    return true;
}

void Burner::updateProgress()
{
    // Progress is now handled directly in onProcessOutput()
    // This method is kept for compatibility with the timer-based approach
    if (m_totalBytes > 0) {
        int percentage = (int)((m_bytesWritten * 100) / m_totalBytes);
        percentage = qMin(percentage, 100); // Cap at 100%
        emit progressChanged(percentage);
    }
}

qint64 Burner::getBytesWritten(const QString &devicePath)
{
    Q_UNUSED(devicePath)
    // Return current bytes written as tracked by the process output parsing
    return m_bytesWritten;
}

QString Burner::calculateSpeed(qint64 bytes, qint64 timeMs)
{
    if (timeMs == 0) return "0 B/s";
    
    double bytesPerSec = (double)bytes / (timeMs / 1000.0);
    
    const QStringList units = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unitIndex = 0;
    
    while (bytesPerSec >= 1024.0 && unitIndex < units.size() - 1) {
        bytesPerSec /= 1024.0;
        unitIndex++;
    }
    
    return QString::number(bytesPerSec, 'f', 2) + " " + units[unitIndex];
}

QString Burner::calculateTimeRemaining(qint64 bytesRemaining, double speedBytesPerSec)
{
    if (speedBytesPerSec <= 0) return "Unknown";
    
    int secondsRemaining = (int)(bytesRemaining / speedBytesPerSec);
    
    int hours = secondsRemaining / 3600;
    int minutes = (secondsRemaining % 3600) / 60;
    int seconds = secondsRemaining % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
    }
}

QString Burner::getFileSystemCommand(FileSystem fs)
{
    switch (fs) {
        case FileSystem::FAT32: return "mkfs.fat";
        case FileSystem::NTFS: return "mkfs.ntfs";
        case FileSystem::exFAT: return "mkfs.exfat";
        case FileSystem::ext4: return "mkfs.ext4";
        default: return "mkfs.fat";
    }
}

QStringList Burner::getFormatArguments(FileSystem fs, const QString &label, int clusterSize)
{
    QStringList args;
    
    switch (fs) {
        case FileSystem::FAT32:
            args << "-F" << "32";
            if (!label.isEmpty()) {
                args << "-n" << label;
            }
            if (clusterSize > 0) {
                args << "-s" << QString::number(clusterSize);
            }
            break;
        case FileSystem::NTFS:
            args << "-f";
            if (!label.isEmpty()) {
                args << "-L" << label;
            }
            if (clusterSize > 0) {
                args << "-c" << QString::number(clusterSize);
            }
            break;
        case FileSystem::exFAT:
            if (!label.isEmpty()) {
                args << "-n" << label;
            }
            break;
        case FileSystem::ext4:
            args << "-F";
            if (!label.isEmpty()) {
                args << "-L" << label;
            }
            break;
    }
    
    return args;
}

bool Burner::unmountDevice(const QString &devicePath)
{
    DeviceManager deviceManager;
    return deviceManager.unmountAllPartitions(devicePath);
}

bool Burner::syncDevice(const QString &devicePath)
{
    Q_UNUSED(devicePath)
    
    QProcess sync;
    sync.start("sync");
    sync.waitForFinished(10000);
    
    return sync.exitCode() == 0;
}

QString Burner::getPartitionPath(const QString &devicePath, int partitionNumber)
{
    QString deviceName = QFileInfo(devicePath).fileName();
    
    // Handle different device naming schemes
    if (deviceName.startsWith("sd") || deviceName.startsWith("hd")) {
        return devicePath + QString::number(partitionNumber);
    } else if (deviceName.startsWith("mmcblk") || deviceName.startsWith("nvme")) {
        return devicePath + "p" + QString::number(partitionNumber);
    }
    
    return devicePath + QString::number(partitionNumber);
}

bool Burner::verifyImageChecksum(const QString &imagePath, const QString &devicePath)
{
    QString imageHash = calculateSHA256(imagePath);
    QString deviceHash = calculateSHA256(devicePath);
    
    return !imageHash.isEmpty() && imageHash == deviceHash;
}

QString Burner::calculateMD5(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QCryptographicHash hash(QCryptographicHash::Md5);
    if (hash.addData(&file)) {
        return hash.result().toHex();
    }
    
    return QString();
}

QString Burner::calculateSHA256(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (hash.addData(&file)) {
        return hash.result().toHex();
    }
    
    return QString();
}


