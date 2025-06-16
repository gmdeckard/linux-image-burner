#include "Validation.h"
#include "Utils.h"
#include "../core/ImageHandler.h"
#include "../core/FileSystemManager.h"
#include <QFileInfo>
#include <QFile>
#include <QProcess>
#include <QDir>
#include <QTextStream>
#include <QStorageInfo>
#include <QRegularExpression>

bool Validation::isValidImageFile(const QString &filePath)
{
    return checkImageFileExists(filePath) &&
           checkImageFileReadable(filePath) &&
           checkImageFileSize(filePath) &&
           isValidImageFormat(filePath);
}

bool Validation::isValidImageFormat(const QString &filePath)
{
    ImageType type = ImageHandler::detectImageType(filePath);
    return type != ImageType::Unknown;
}

bool Validation::isImageAccessible(const QString &filePath)
{
    return checkImageFileExists(filePath) && checkImageFileReadable(filePath);
}

QString Validation::getImageValidationError(const QString &filePath)
{
    if (!checkImageFileExists(filePath)) {
        return "Image file does not exist";
    }
    
    if (!checkImageFileReadable(filePath)) {
        return "Image file is not readable (check permissions)";
    }
    
    if (!checkImageFileSize(filePath)) {
        return "Image file is empty or too large";
    }
    
    if (!isValidImageFormat(filePath)) {
        return "Unsupported image format";
    }
    
    return QString(); // No error
}

bool Validation::isValidDevice(const QString &devicePath)
{
    return checkDeviceExists(devicePath) &&
           checkDeviceBlockDevice(devicePath) &&
           checkDevicePermissions(devicePath);
}

bool Validation::isDeviceWritable(const QString &devicePath)
{
    // For regular users, we can't check device writability directly
    // The actual write permission will be handled by pkexec during burning
    // For now, assume removable devices are writable if they exist
    return QFileInfo(devicePath).exists();
}

bool Validation::isDeviceRemovable(const QString &devicePath)
{
    QString deviceName = QFileInfo(devicePath).fileName();
    QString removablePath = QString("/sys/block/%1/removable").arg(deviceName);
    
    QFile removableFile(removablePath);
    if (removableFile.open(QIODevice::ReadOnly)) {
        QString removable = QString::fromUtf8(removableFile.readAll()).trimmed();
        return removable == "1";
    }
    
    return false;
}

bool Validation::isDeviceSafeToWrite(const QString &devicePath)
{
    return !isSystemDisk(devicePath) && 
           isDeviceRemovable(devicePath) &&
           !isMountedDevice(devicePath);
}

QString Validation::getDeviceValidationError(const QString &devicePath)
{
    if (!checkDeviceExists(devicePath)) {
        return "Device does not exist";
    }
    
    if (!checkDeviceBlockDevice(devicePath)) {
        return "Not a valid block device";
    }
    
    if (!checkDevicePermissions(devicePath)) {
        return "Insufficient permissions to access device";
    }
    
    if (isSystemDisk(devicePath)) {
        return "Cannot write to system disk (safety check)";
    }
    
    return QString(); // No error
}

bool Validation::isValidFileSystemType(const QString &fsType)
{
    QStringList supported = getSupportedFileSystemTypes();
    return supported.contains(fsType, Qt::CaseInsensitive);
}

bool Validation::isValidVolumeLabel(const QString &fsType, const QString &label)
{
    return FileSystemManager::isValidVolumeLabel(fsType, label);
}

bool Validation::isValidClusterSize(const QString &fsType, int clusterSize)
{
    return FileSystemManager::isValidClusterSize(fsType, clusterSize);
}

bool Validation::isFileSystemCompatible(const QString &fsType, qint64 deviceSize)
{
    FileSystemInfo info = FileSystemManager::getFileSystemInfo(fsType);
    return info.maxVolumeSize >= deviceSize;
}

bool Validation::isValidSize(qint64 size)
{
    return size > 0 && size < (1024LL * 1024 * 1024 * 1024 * 1024); // < 1PB
}

bool Validation::isImageFitsOnDevice(const QString &imagePath, const QString &devicePath)
{
    qint64 imageSize = Utils::getFileSize(imagePath);
    
    QString deviceName = QFileInfo(devicePath).fileName();
    QString sizePath = QString("/sys/block/%1/size").arg(deviceName);
    
    QFile sizeFile(sizePath);
    if (sizeFile.open(QIODevice::ReadOnly)) {
        QString sizeStr = QString::fromUtf8(sizeFile.readAll()).trimmed();
        qint64 deviceSize = sizeStr.toLongLong() * 512; // Size is in 512-byte sectors
        
        return isImageFitsOnDevice(imageSize, deviceSize);
    }
    
    return false;
}

bool Validation::isImageFitsOnDevice(qint64 imageSize, qint64 deviceSize)
{
    return imageSize <= deviceSize;
}

bool Validation::hasRequiredPermissions()
{
    return Utils::isRunningAsRoot();
}

bool Validation::canAccessDevice(const QString &devicePath)
{
    return QFileInfo(devicePath).exists();
}

bool Validation::canWriteToDevice(const QString &devicePath)
{
    // For regular users, we can't check device writability directly
    // The actual write permission will be handled by pkexec during burning
    // For now, assume devices are writable if they exist
    return QFileInfo(devicePath).exists();
}

bool Validation::isSystemDisk(const QString &devicePath)
{
    QStringList systemDisks = getSystemDisks();
    QString deviceName = QFileInfo(devicePath).fileName();
    
    // Check if this device contains the root filesystem
    for (const QString &sysDisk : systemDisks) {
        if (devicePath.startsWith(sysDisk)) {
            return true;
        }
    }
    
    return false;
}

bool Validation::isMountedDevice(const QString &devicePath)
{
    QStringList mountedDevices = getMountedDevices();
    QString deviceName = QFileInfo(devicePath).fileName();
    
    for (const QString &mounted : mountedDevices) {
        if (mounted.startsWith(devicePath) || mounted.startsWith(deviceName)) {
            return true;
        }
    }
    
    return false;
}

bool Validation::hasImportantData(const QString &devicePath)
{
    // This is a simplified check - in practice, you might want to 
    // check for specific file types or directory structures
    Q_UNUSED(devicePath)
    return false;
}

QStringList Validation::getWarnings(const QString &devicePath)
{
    QStringList warnings;
    
    if (isMountedDevice(devicePath)) {
        warnings << "Device is currently mounted and will be unmounted";
    }
    
    if (!isDeviceRemovable(devicePath)) {
        warnings << "Device is not marked as removable";
    }
    
    if (hasImportantData(devicePath)) {
        warnings << "Device may contain important data";
    }
    
    return warnings;
}

bool Validation::validateBurnOptions(const QString &imagePath, const QString &devicePath,
                                    const QString &fsType, const QString &label)
{
    return isValidImageFile(imagePath) &&
           isValidDevice(devicePath) &&
           isValidFileSystemType(fsType) &&
           isValidVolumeLabel(fsType, label) &&
           isImageFitsOnDevice(imagePath, devicePath);
}

QStringList Validation::getBurnOptionsErrors(const QString &imagePath, const QString &devicePath,
                                            const QString &fsType, const QString &label)
{
    QStringList errors;
    
    QString imageError = getImageValidationError(imagePath);
    if (!imageError.isEmpty()) {
        errors << imageError;
    }
    
    QString deviceError = getDeviceValidationError(devicePath);
    if (!deviceError.isEmpty()) {
        errors << deviceError;
    }
    
    if (!isValidFileSystemType(fsType)) {
        errors << "Invalid file system type: " + fsType;
    }
    
    if (!isValidVolumeLabel(fsType, label)) {
        errors << "Invalid volume label for " + fsType + " file system";
    }
    
    if (!isImageFitsOnDevice(imagePath, devicePath)) {
        errors << "Image is too large for the selected device";
    }
    
    return errors;
}

QStringList Validation::getSupportedImageFormats()
{
    return {"ISO", "IMG", "DMG", "VHD", "VHDX", "VMDK"};
}

QStringList Validation::getSupportedFileSystemTypes()
{
    return {"FAT32", "NTFS", "exFAT", "ext4"};
}

bool Validation::isFormatSupported(const QString &format)
{
    QStringList supported = getSupportedImageFormats();
    return supported.contains(format, Qt::CaseInsensitive);
}

bool Validation::checkImageFileExists(const QString &filePath)
{
    return QFileInfo(filePath).exists() && QFileInfo(filePath).isFile();
}

bool Validation::checkImageFileReadable(const QString &filePath)
{
    return QFileInfo(filePath).isReadable();
}

bool Validation::checkImageFileSize(const QString &filePath)
{
    qint64 size = QFileInfo(filePath).size();
    return size > 0 && size < (100LL * 1024 * 1024 * 1024); // < 100GB
}

bool Validation::checkDeviceExists(const QString &devicePath)
{
    return QFileInfo(devicePath).exists();
}

bool Validation::checkDeviceBlockDevice(const QString &devicePath)
{
    return Utils::isBlockDevice(devicePath);
}

bool Validation::checkDevicePermissions(const QString &devicePath)
{
    // For regular users, we can't check device permissions directly
    // The actual permission check will be handled by pkexec during burning
    // For now, assume devices are accessible if they exist
    return QFileInfo(devicePath).exists();
}

QStringList Validation::getSystemDisks()
{
    QStringList systemDisks;
    
    // Read /proc/mounts to find system disks
    QFile mounts("/proc/mounts");
    if (mounts.open(QIODevice::ReadOnly)) {
        QTextStream stream(&mounts);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList parts = line.split(' ');
            
            if (parts.size() >= 2) {
                QString device = parts[0];
                QString mountPoint = parts[1];
                
                // Check for system mount points
                if (mountPoint == "/" || mountPoint == "/boot" || 
                    mountPoint == "/usr" || mountPoint == "/var") {
                    
                    // Extract base device name (remove partition number)
                    QString baseDevice = device;
                    baseDevice.remove(QRegularExpression("\\d+$"));
                    
                    if (!systemDisks.contains(baseDevice)) {
                        systemDisks << baseDevice;
                    }
                }
            }
        }
    }
    
    return systemDisks;
}

QStringList Validation::getMountedDevices()
{
    QStringList mountedDevices;
    
    QFile mounts("/proc/mounts");
    if (mounts.open(QIODevice::ReadOnly)) {
        QTextStream stream(&mounts);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList parts = line.split(' ');
            
            if (parts.size() >= 1) {
                QString device = parts[0];
                if (device.startsWith("/dev/")) {
                    mountedDevices << device;
                }
            }
        }
    }
    
    return mountedDevices;
}
