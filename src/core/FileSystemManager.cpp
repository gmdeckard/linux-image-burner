#include "FileSystemManager.h"
#include <QRegularExpression>
#include <QDebug>

QList<FileSystemInfo> FileSystemManager::s_fileSystems;
bool FileSystemManager::s_initialized = false;

FileSystemManager::FileSystemManager(QObject *parent)
    : QObject(parent)
{
    if (!s_initialized) {
        initializeFileSystemInfo();
        s_initialized = true;
    }
}

QList<FileSystemInfo> FileSystemManager::getSupportedFileSystems()
{
    if (!s_initialized) {
        initializeFileSystemInfo();
        s_initialized = true;
    }
    return s_fileSystems;
}

FileSystemInfo FileSystemManager::getFileSystemInfo(const QString &fsType)
{
    QList<FileSystemInfo> fileSystems = getSupportedFileSystems();
    
    for (const FileSystemInfo &fs : fileSystems) {
        if (fs.name.compare(fsType, Qt::CaseInsensitive) == 0) {
            return fs;
        }
    }
    
    return FileSystemInfo(); // Return empty if not found
}

QStringList FileSystemManager::getRecommendedFileSystems(qint64 deviceSize)
{
    QStringList recommended;
    
    // Recommendations based on device size
    if (deviceSize <= 2LL * 1024 * 1024 * 1024) { // <= 2GB
        recommended << "FAT32";
    } else if (deviceSize <= 32LL * 1024 * 1024 * 1024) { // <= 32GB
        recommended << "FAT32" << "exFAT" << "ext4";
    } else if (deviceSize <= 2LL * 1024 * 1024 * 1024 * 1024) { // <= 2TB
        recommended << "exFAT" << "NTFS" << "ext4";
    } else { // > 2TB
        recommended << "exFAT" << "NTFS" << "ext4";
    }
    
    return recommended;
}

QStringList FileSystemManager::getAvailableClusterSizes(const QString &fsType, qint64 volumeSize)
{
    QStringList sizes;
    FileSystemInfo info = getFileSystemInfo(fsType);
    
    if (info.name.isEmpty()) {
        return sizes;
    }
    
    // Generate cluster sizes based on file system and volume size
    int minSize = info.minClusterSize;
    int maxSize = info.maxClusterSize;
    
    // Adjust max size based on volume size for efficiency
    if (volumeSize < 1024LL * 1024 * 1024) { // < 1GB
        maxSize = qMin(maxSize, 32768); // 32KB max
    } else if (volumeSize < 32LL * 1024 * 1024 * 1024) { // < 32GB
        maxSize = qMin(maxSize, 65536); // 64KB max
    }
    
    // Generate power-of-2 sizes
    for (int size = minSize; size <= maxSize; size *= 2) {
        if (fsType.compare("FAT32", Qt::CaseInsensitive) == 0) {
            sizes << QString("%1 bytes (%2 sectors)").arg(size).arg(size / 512);
        } else {
            sizes << QString("%1 bytes").arg(size);
        }
    }
    
    return sizes;
}

int FileSystemManager::getRecommendedClusterSize(const QString &fsType, qint64 volumeSize)
{
    FileSystemInfo info = getFileSystemInfo(fsType);
    
    if (info.name.isEmpty()) {
        return 4096; // Default 4KB
    }
    
    // Recommendations based on volume size and file system
    if (fsType.compare("FAT32", Qt::CaseInsensitive) == 0) {
        if (volumeSize <= 256LL * 1024 * 1024) { // <= 256MB
            return 512;
        } else if (volumeSize <= 8LL * 1024 * 1024 * 1024) { // <= 8GB
            return 4096;
        } else if (volumeSize <= 16LL * 1024 * 1024 * 1024) { // <= 16GB
            return 8192;
        } else if (volumeSize <= 32LL * 1024 * 1024 * 1024) { // <= 32GB
            return 16384;
        } else {
            return 32768;
        }
    } else if (fsType.compare("NTFS", Qt::CaseInsensitive) == 0) {
        if (volumeSize <= 16LL * 1024 * 1024 * 1024) { // <= 16GB
            return 4096;
        } else if (volumeSize <= 2LL * 1024 * 1024 * 1024 * 1024) { // <= 2TB
            return 4096;
        } else {
            return 8192;
        }
    } else if (fsType.compare("exFAT", Qt::CaseInsensitive) == 0) {
        if (volumeSize <= 32LL * 1024 * 1024 * 1024) { // <= 32GB
            return 32768;
        } else {
            return 131072; // 128KB
        }
    } else if (fsType.compare("ext4", Qt::CaseInsensitive) == 0) {
        return 4096; // Standard 4KB for ext4
    }
    
    return info.defaultClusterSize;
}

int FileSystemManager::getOptimalClusterSize(const QString &fsType, qint64 volumeSize)
{
    // For now, optimal is the same as recommended
    return getRecommendedClusterSize(fsType, volumeSize);
}

bool FileSystemManager::isValidVolumeLabel(const QString &fsType, const QString &label)
{
    if (label.isEmpty()) {
        return true; // Empty labels are generally allowed
    }
    
    if (fsType.compare("FAT32", Qt::CaseInsensitive) == 0) {
        // FAT32 labels: max 11 chars, ASCII only, no special chars
        if (label.length() > 11) {
            return false;
        }
        QRegularExpression re("^[A-Za-z0-9 _-]+$");
        return re.match(label).hasMatch();
    } else if (fsType.compare("NTFS", Qt::CaseInsensitive) == 0) {
        // NTFS labels: max 32 chars, Unicode allowed
        return label.length() <= 32;
    } else if (fsType.compare("exFAT", Qt::CaseInsensitive) == 0) {
        // exFAT labels: max 15 chars, Unicode allowed
        return label.length() <= 15;
    } else if (fsType.compare("ext4", Qt::CaseInsensitive) == 0) {
        // ext4 labels: max 16 chars
        return label.length() <= 16;
    }
    
    return true;
}

QString FileSystemManager::sanitizeVolumeLabel(const QString &fsType, const QString &label)
{
    QString sanitized = label;
    
    if (fsType.compare("FAT32", Qt::CaseInsensitive) == 0) {
        // Convert to uppercase and remove invalid characters
        sanitized = sanitized.toUpper();
        sanitized.replace(QRegularExpression("[^A-Z0-9 _-]"), "");
        if (sanitized.length() > 11) {
            sanitized.truncate(11);
        }
    } else if (fsType.compare("NTFS", Qt::CaseInsensitive) == 0) {
        if (sanitized.length() > 32) {
            sanitized.truncate(32);
        }
    } else if (fsType.compare("exFAT", Qt::CaseInsensitive) == 0) {
        if (sanitized.length() > 15) {
            sanitized.truncate(15);
        }
    } else if (fsType.compare("ext4", Qt::CaseInsensitive) == 0) {
        if (sanitized.length() > 16) {
            sanitized.truncate(16);
        }
    }
    
    return sanitized;
}

bool FileSystemManager::isValidClusterSize(const QString &fsType, int clusterSize)
{
    FileSystemInfo info = getFileSystemInfo(fsType);
    
    if (info.name.isEmpty()) {
        return false;
    }
    
    // Check if cluster size is within valid range
    if (clusterSize < info.minClusterSize || clusterSize > info.maxClusterSize) {
        return false;
    }
    
    // Check if cluster size is a power of 2
    return (clusterSize & (clusterSize - 1)) == 0;
}

QString FileSystemManager::formatSizeToString(qint64 bytes)
{
    const QStringList units = {"B", "KB", "MB", "GB", "TB", "PB"};
    double size = bytes;
    int unitIndex = 0;
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    return QString::number(size, 'f', (unitIndex == 0) ? 0 : 2) + " " + units[unitIndex];
}

qint64 FileSystemManager::calculateUsableSpace(const QString &fsType, qint64 totalSpace)
{
    double overhead = getOverheadPercentage(fsType);
    return qint64(totalSpace * (1.0 - overhead));
}

double FileSystemManager::getOverheadPercentage(const QString &fsType)
{
    if (fsType.compare("FAT32", Qt::CaseInsensitive) == 0) {
        return 0.02; // ~2% overhead
    } else if (fsType.compare("NTFS", Qt::CaseInsensitive) == 0) {
        return 0.05; // ~5% overhead
    } else if (fsType.compare("exFAT", Qt::CaseInsensitive) == 0) {
        return 0.01; // ~1% overhead
    } else if (fsType.compare("ext4", Qt::CaseInsensitive) == 0) {
        return 0.05; // ~5% overhead (reserved blocks)
    }
    
    return 0.05; // Default 5%
}

void FileSystemManager::initializeFileSystemInfo()
{
    s_fileSystems.clear();
    
    // FAT32
    FileSystemInfo fat32;
    fat32.name = "FAT32";
    fat32.displayName = "FAT32";
    fat32.supportedSizes << "Up to 2TB";
    fat32.minClusterSize = 512;
    fat32.maxClusterSize = 32768;
    fat32.defaultClusterSize = 4096;
    fat32.maxVolumeSize = 2LL * 1024 * 1024 * 1024 * 1024; // 2TB
    fat32.supportsCompression = false;
    fat32.supportsEncryption = false;
    fat32.isBootable = true;
    s_fileSystems.append(fat32);
    
    // NTFS
    FileSystemInfo ntfs;
    ntfs.name = "NTFS";
    ntfs.displayName = "NTFS";
    ntfs.supportedSizes << "Up to 256TB";
    ntfs.minClusterSize = 512;
    ntfs.maxClusterSize = 65536;
    ntfs.defaultClusterSize = 4096;
    ntfs.maxVolumeSize = 256LL * 1024 * 1024 * 1024 * 1024; // 256TB
    ntfs.supportsCompression = true;
    ntfs.supportsEncryption = true;
    ntfs.isBootable = true;
    s_fileSystems.append(ntfs);
    
    // exFAT
    FileSystemInfo exfat;
    exfat.name = "exFAT";
    exfat.displayName = "exFAT";
    exfat.supportedSizes << "Up to 128PB";
    exfat.minClusterSize = 512;
    exfat.maxClusterSize = 33554432; // 32MB
    exfat.defaultClusterSize = 131072; // 128KB
    exfat.maxVolumeSize = 128LL * 1024 * 1024 * 1024 * 1024 * 1024; // 128PB
    exfat.supportsCompression = false;
    exfat.supportsEncryption = false;
    exfat.isBootable = false;
    s_fileSystems.append(exfat);
    
    // ext4
    FileSystemInfo ext4;
    ext4.name = "ext4";
    ext4.displayName = "ext4";
    ext4.supportedSizes << "Up to 1EB";
    ext4.minClusterSize = 1024;
    ext4.maxClusterSize = 65536;
    ext4.defaultClusterSize = 4096;
    ext4.maxVolumeSize = 1LL * 1024 * 1024 * 1024 * 1024 * 1024 * 1024; // 1EB
    ext4.supportsCompression = false;
    ext4.supportsEncryption = true;
    ext4.isBootable = true;
    s_fileSystems.append(ext4);
}
