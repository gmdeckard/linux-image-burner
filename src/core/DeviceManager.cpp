#include "DeviceManager.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <unistd.h>
#include <sys/mount.h>

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_monitorTimer(new QTimer(this))
{
    // Watch /dev for device changes
    m_watcher->addPath("/dev");
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &DeviceManager::onDirectoryChanged);
    
    // Setup monitoring timer
    m_monitorTimer->setInterval(2000); // Check every 2 seconds
    connect(m_monitorTimer, &QTimer::timeout,
            this, &DeviceManager::onMonitorTimer);
}

DeviceManager::~DeviceManager()
{
    stopMonitoring();
}

QList<DeviceInfo> DeviceManager::getRemovableDevices()
{
    QList<DeviceInfo> devices;
    
    // Use lsblk to get device information without requiring root
    QProcess lsblk;
    lsblk.start("lsblk", QStringList() << "-J" << "-o" << "NAME,SIZE,TYPE,MOUNTPOINT,RM,VENDOR,MODEL,FSTYPE,UUID,TRAN");
    lsblk.waitForFinished(5000);
    
    if (lsblk.exitCode() == 0) {
        QByteArray output = lsblk.readAllStandardOutput();
        devices = parseLsblkOutput(output, true); // Only removable devices
    }
    
    return devices;
}

QList<DeviceInfo> DeviceManager::getAllStorageDevices()
{
    QList<DeviceInfo> devices;
    
    // Use lsblk to get all storage device information
    QProcess lsblk;
    lsblk.start("lsblk", QStringList() << "-J" << "-o" << "NAME,SIZE,TYPE,MOUNTPOINT,RM,VENDOR,MODEL,FSTYPE,UUID,TRAN");
    lsblk.waitForFinished(5000);
    
    if (lsblk.exitCode() == 0) {
        QByteArray output = lsblk.readAllStandardOutput();
        devices = parseLsblkOutput(output, false); // All devices
    }
    
    return devices;
}

DeviceInfo DeviceManager::getDeviceInfo(const QString &devicePath)
{
    // Use lsblk to get device info instead of direct file access
    QProcess lsblk;
    lsblk.start("lsblk", QStringList() << "-J" << "-o" << "NAME,SIZE,TYPE,MOUNTPOINT,RM,VENDOR,MODEL,FSTYPE,UUID,TRAN" << devicePath);
    lsblk.waitForFinished(5000);
    
    if (lsblk.exitCode() == 0) {
        QByteArray output = lsblk.readAllStandardOutput();
        QList<DeviceInfo> devices = parseLsblkOutput(output, false);
        if (!devices.isEmpty()) {
            return devices.first();
        }
    }
    
    // Fallback: create basic info if lsblk fails
    DeviceInfo info;
    info.path = devicePath;
    QString deviceName = QFileInfo(devicePath).fileName();
    info.name = deviceName;
    return info;
}

bool DeviceManager::unmountDevice(const QString &devicePath)
{
    QStringList mountPoints = getMountPoints(devicePath);
    
    for (const QString &mountPoint : mountPoints) {
        QProcess umount;
        umount.start("umount", QStringList() << mountPoint);
        umount.waitForFinished(5000);
        
        if (umount.exitCode() != 0) {
            qWarning() << "Failed to unmount" << mountPoint;
            return false;
        }
    }
    
    return true;
}

bool DeviceManager::unmountAllPartitions(const QString &devicePath)
{
    QDir devDir("/dev");
    QString deviceName = QFileInfo(devicePath).fileName();
    
    // Find all partitions of this device
    QStringList filters;
    filters << deviceName + "[0-9]*";
    devDir.setNameFilters(filters);
    
    QStringList partitions = devDir.entryList(QDir::System);
    
    bool success = true;
    for (const QString &partition : partitions) {
        QString partitionPath = "/dev/" + partition;
        if (!unmountDevice(partitionPath)) {
            success = false;
        }
    }
    
    return success;
}

bool DeviceManager::ejectDevice(const QString &devicePath)
{
    // First unmount all partitions
    if (!unmountAllPartitions(devicePath)) {
        return false;
    }
    
    // Then eject the device
    QProcess eject;
    eject.start("eject", QStringList() << devicePath);
    eject.waitForFinished(5000);
    
    return eject.exitCode() == 0;
}

void DeviceManager::startMonitoring()
{
    m_monitorTimer->start();
    refreshDevices();
}

void DeviceManager::stopMonitoring()
{
    m_monitorTimer->stop();
}

QString DeviceManager::formatSize(qint64 bytes)
{
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    double size = bytes;
    int unitIndex = 0;
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    return QString::number(size, 'f', (unitIndex == 0) ? 0 : 2) + " " + units[unitIndex];
}

bool DeviceManager::isDeviceWritable(const QString &devicePath)
{
    // For regular users, we can't check device writability directly
    // The actual write permission will be handled by pkexec during burning
    // For now, assume removable devices are writable if they exist
    return QFile::exists(devicePath);
}

bool DeviceManager::isDeviceBusy(const QString &devicePath)
{
    QProcess lsof;
    lsof.start("lsof", QStringList() << devicePath);
    lsof.waitForFinished(2000);
    
    return !lsof.readAllStandardOutput().isEmpty();
}

void DeviceManager::refreshDevices()
{
    QList<DeviceInfo> currentDevices = getRemovableDevices();
    
    // Check for new devices
    for (const DeviceInfo &device : currentDevices) {
        bool found = false;
        for (const DeviceInfo &lastDevice : m_lastDeviceList) {
            if (device.path == lastDevice.path) {
                found = true;
                break;
            }
        }
        if (!found) {
            emit deviceInserted(device.path);
        }
    }
    
    // Check for removed devices
    for (const DeviceInfo &lastDevice : m_lastDeviceList) {
        bool found = false;
        for (const DeviceInfo &device : currentDevices) {
            if (device.path == lastDevice.path) {
                found = true;
                break;
            }
        }
        if (!found) {
            emit deviceRemoved(lastDevice.path);
        }
    }
    
    if (currentDevices.size() != m_lastDeviceList.size()) {
        emit deviceListChanged();
    }
    
    m_lastDeviceList = currentDevices;
}

void DeviceManager::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)
    refreshDevices();
}

void DeviceManager::onMonitorTimer()
{
    refreshDevices();
}

DeviceInfo DeviceManager::parseDeviceInfo(const QString &devicePath)
{
    DeviceInfo info;
    info.path = devicePath;
    
    QString deviceName = QFileInfo(devicePath).fileName();
    info.name = deviceName;
    
    info.model = getDeviceModel(devicePath);
    info.vendor = getDeviceVendor(devicePath);
    info.size = getDeviceSize(devicePath);
    info.sizeString = formatSize(info.size);
    info.isRemovable = isRemovableDevice(devicePath);
    info.mountPoints = getMountPoints(devicePath);
    info.isMounted = !info.mountPoints.isEmpty();
    info.fileSystem = getFileSystemType(devicePath);
    info.uuid = getDeviceUUID(devicePath);
    info.isUSB = isUSBDevice(devicePath);
    info.isMMC = isMMCDevice(devicePath);
    
    return info;
}

QString DeviceManager::readSysfsAttribute(const QString &devicePath, const QString &attribute)
{
    QString deviceName = QFileInfo(devicePath).fileName();
    QString sysfsPath = QString("/sys/block/%1/%2").arg(deviceName, attribute);
    
    QFile file(sysfsPath);
    if (file.open(QIODevice::ReadOnly)) {
        return file.readAll().trimmed();
    }
    
    return QString();
}

QStringList DeviceManager::getMountPoints(const QString &devicePath)
{
    QStringList mountPoints;
    QFile mounts("/proc/mounts");
    
    if (mounts.open(QIODevice::ReadOnly)) {
        QTextStream stream(&mounts);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList parts = line.split(' ');
            if (parts.size() >= 2 && parts[0].startsWith(devicePath)) {
                mountPoints.append(parts[1]);
            }
        }
    }
    
    return mountPoints;
}

bool DeviceManager::isRemovableDevice(const QString &devicePath)
{
    QString removable = readSysfsAttribute(devicePath, "removable");
    return removable == "1";
}

QString DeviceManager::getDeviceModel(const QString &devicePath)
{
    QString deviceName = QFileInfo(devicePath).fileName();
    QString modelPath = QString("/sys/block/%1/device/model").arg(deviceName);
    
    QFile file(modelPath);
    if (file.open(QIODevice::ReadOnly)) {
        return file.readAll().trimmed();
    }
    
    return QString();
}

QString DeviceManager::getDeviceVendor(const QString &devicePath)
{
    QString deviceName = QFileInfo(devicePath).fileName();
    QString vendorPath = QString("/sys/block/%1/device/vendor").arg(deviceName);
    
    QFile file(vendorPath);
    if (file.open(QIODevice::ReadOnly)) {
        return file.readAll().trimmed();
    }
    
    return QString();
}

qint64 DeviceManager::getDeviceSize(const QString &devicePath)
{
    QString size = readSysfsAttribute(devicePath, "size");
    if (!size.isEmpty()) {
        return size.toLongLong() * 512; // Size is in 512-byte sectors
    }
    
    return 0;
}

QString DeviceManager::getFileSystemType(const QString &devicePath)
{
    QProcess blkid;
    blkid.start("blkid", QStringList() << "-o" << "value" << "-s" << "TYPE" << devicePath);
    blkid.waitForFinished(2000);
    
    return QString::fromUtf8(blkid.readAllStandardOutput()).trimmed();
}

QString DeviceManager::getDeviceUUID(const QString &devicePath)
{
    QProcess blkid;
    blkid.start("blkid", QStringList() << "-o" << "value" << "-s" << "UUID" << devicePath);
    blkid.waitForFinished(2000);
    
    return QString::fromUtf8(blkid.readAllStandardOutput()).trimmed();
}

bool DeviceManager::isUSBDevice(const QString &devicePath)
{
    QString deviceName = QFileInfo(devicePath).fileName();
    
    // Check if device is connected via USB
    QString subsystemPath = QString("/sys/block/%1/device/subsystem").arg(deviceName);
    QFileInfo subsystemInfo(subsystemPath);
    
    if (subsystemInfo.exists() && subsystemInfo.isSymLink()) {
        QString target = subsystemInfo.symLinkTarget();
        return target.contains("usb");
    }
    
    return false;
}

bool DeviceManager::isMMCDevice(const QString &devicePath)
{
    QString deviceName = QFileInfo(devicePath).fileName();
    return deviceName.startsWith("mmcblk");
}

QList<DeviceInfo> DeviceManager::parseLsblkOutput(const QByteArray &output, bool removableOnly)
{
    QList<DeviceInfo> devices;
    
    // Parse JSON output from lsblk
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(output, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse lsblk output:" << error.errorString();
        return devices;
    }
    
    QJsonObject root = doc.object();
    QJsonArray blockDevices = root["blockdevices"].toArray();
    
    for (const QJsonValue &value : blockDevices) {
        QJsonObject device = value.toObject();
        
        // Skip if not a disk (only want whole devices, not partitions)
        if (device["type"].toString() != "disk") {
            continue;
        }
        
        DeviceInfo info;
        info.path = "/dev/" + device["name"].toString();
        info.name = device["name"].toString();
        info.model = device["model"].toString();
        info.vendor = device["vendor"].toString();
        
        // Handle both boolean and string values for rm field
        QJsonValue rmValue = device["rm"];
        if (rmValue.isBool()) {
            info.isRemovable = rmValue.toBool();
        } else {
            info.isRemovable = rmValue.toString() == "1";
        }
        
        info.isMounted = !device["mountpoint"].toString().isEmpty();
        
        // Get filesystem info from lsblk output
        info.fileSystem = device["fstype"].toString();
        info.uuid = device["uuid"].toString();
        
        // Parse size
        QString sizeStr = device["size"].toString();
        info.sizeString = sizeStr;
        info.size = parseSizeString(sizeStr);
        
        // Determine device type - use TRAN field for better USB detection
        QString transport = device["tran"].toString();
        QString deviceName = device["name"].toString();
        info.isUSB = (transport == "usb") || deviceName.startsWith("sd");
        info.isMMC = deviceName.startsWith("mmcblk");
        
        // Get mount points without requiring device access
        QStringList mountPoints;
        QString mountPoint = device["mountpoint"].toString();
        if (!mountPoint.isEmpty()) {
            mountPoints.append(mountPoint);
        }
        info.mountPoints = mountPoints;
        
        // Skip devices with zero size
        if (info.size == 0) {
            continue;
        }
        
        // Only add removable devices if requested
        if (removableOnly && !info.isRemovable) {
            continue;
        }
        
        qDebug() << "Found device:" << info.path << "Size:" << info.sizeString 
                 << "Removable:" << info.isRemovable << "USB:" << info.isUSB;
        
        devices.append(info);
    }
    
    return devices;
}

qint64 DeviceManager::parseSizeString(const QString &sizeStr)
{
    if (sizeStr.isEmpty()) return 0;
    
    QRegularExpression sizeRegex(R"((\d+(?:\.\d+)?)\s*([KMGT]?)B?)");
    QRegularExpressionMatch match = sizeRegex.match(sizeStr);
    
    if (!match.hasMatch()) return 0;
    
    double value = match.captured(1).toDouble();
    QString unit = match.captured(2).toUpper();
    
    if (unit == "K") value *= 1024;
    else if (unit == "M") value *= 1024 * 1024;
    else if (unit == "G") value *= 1024 * 1024 * 1024;
    else if (unit == "T") value *= 1024LL * 1024 * 1024 * 1024;
    
    return static_cast<qint64>(value);
}
