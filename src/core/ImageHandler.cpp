#include "ImageHandler.h"
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QDebug>
#include <QMimeDatabase>
#include <QMimeType>

ImageHandler::ImageHandler(QObject *parent)
    : QObject(parent)
{
}

ImageInfo ImageHandler::analyzeImage(const QString &imagePath)
{
    ImageInfo info;
    info.filePath = imagePath;
    info.isValid = false;
    
    if (!QFile::exists(imagePath)) {
        info.errorMessage = "File does not exist";
        return info;
    }
    
    QFileInfo fileInfo(imagePath);
    info.size = fileInfo.size();
    info.sizeString = formatSize(info.size);
    
    // Detect image type
    info.type = detectImageType(imagePath);
    
    // Analyze based on type
    bool analysisSuccess = false;
    switch (info.type) {
        case ImageType::ISO:
            analysisSuccess = analyzeISOImage(imagePath, info);
            break;
        case ImageType::IMG:
            analysisSuccess = analyzeIMGImage(imagePath, info);
            break;
        case ImageType::DMG:
            analysisSuccess = analyzeDMGImage(imagePath, info);
            break;
        case ImageType::VHD:
        case ImageType::VHDX:
            analysisSuccess = analyzeVHDImage(imagePath, info);
            break;
        default:
            info.errorMessage = "Unsupported image format";
            break;
    }
    
    if (analysisSuccess) {
        info.isValid = true;
        info.isBootable = isImageBootable(imagePath);
        info.bootLoaders = detectBootLoaders(imagePath);
        info.architecture = detectArchitecture(imagePath);
    }
    
    emit analysisFinished(info);
    return info;
}

bool ImageHandler::validateImage(const QString &imagePath)
{
    if (!QFile::exists(imagePath)) {
        return false;
    }
    
    ImageType type = detectImageType(imagePath);
    return type != ImageType::Unknown;
}

ImageType ImageHandler::detectImageType(const QString &imagePath)
{
    QFileInfo fileInfo(imagePath);
    QString extension = fileInfo.suffix().toLower();
    
    if (extension == "iso") {
        return ImageType::ISO;
    } else if (extension == "img") {
        return ImageType::IMG;
    } else if (extension == "dmg") {
        return ImageType::DMG;
    } else if (extension == "vhd") {
        return ImageType::VHD;
    } else if (extension == "vhdx") {
        return ImageType::VHDX;
    } else if (extension == "vmdk") {
        return ImageType::VMDK;
    }
    
    // Check file signature/magic bytes
    QFile file(imagePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray header = file.read(2048);
        
        // ISO 9660 signature
        if (header.mid(32769, 5) == "CD001") {
            return ImageType::ISO;
        }
        
        // VHD signature
        if (header.startsWith("conectix")) {
            return ImageType::VHD;
        }
        
        // DMG signature
        if (header.contains("koly")) {
            return ImageType::DMG;
        }
    }
    
    return ImageType::Unknown;
}

QString ImageHandler::imageTypeToString(ImageType type)
{
    switch (type) {
        case ImageType::ISO: return "ISO";
        case ImageType::IMG: return "IMG";
        case ImageType::DMG: return "DMG";
        case ImageType::VHD: return "VHD";
        case ImageType::VHDX: return "VHDX";
        case ImageType::VMDK: return "VMDK";
        default: return "Unknown";
    }
}

QStringList ImageHandler::getSupportedExtensions()
{
    return {"*.iso", "*.img", "*.dmg", "*.vhd", "*.vhdx", "*.vmdk"};
}

bool ImageHandler::isImageBootable(const QString &imagePath)
{
    ImageType type = detectImageType(imagePath);
    
    switch (type) {
        case ImageType::ISO:
            return hasISOLinuxBootloader(imagePath) || 
                   hasEFIBootloader(imagePath) ||
                   hasGRUBBootloader(imagePath);
        case ImageType::IMG:
            return hasMBRBootloader(imagePath) || hasEFIBootloader(imagePath);
        default:
            return false;
    }
}

QString ImageHandler::getImageLabel(const QString &imagePath)
{
    ImageType type = detectImageType(imagePath);
    
    if (type == ImageType::ISO) {
        QProcess isoinfo;
        isoinfo.start("isoinfo", QStringList() << "-d" << "-i" << imagePath);
        isoinfo.waitForFinished(5000);
        
        QString output = QString::fromUtf8(isoinfo.readAllStandardOutput());
        QStringList lines = output.split('\n');
        
        for (const QString &line : lines) {
            if (line.startsWith("Volume id:")) {
                return line.mid(11).trimmed();
            }
        }
    }
    
    return QString();
}

QString ImageHandler::getImageFileSystem(const QString &imagePath)
{
    ImageType type = detectImageType(imagePath);
    
    switch (type) {
        case ImageType::ISO:
            return detectFileSystemFromISO(imagePath);
        case ImageType::IMG:
            return detectFileSystemFromDevice(imagePath);
        default:
            return QString();
    }
}

QStringList ImageHandler::detectBootLoaders(const QString &imagePath)
{
    QStringList bootLoaders;
    
    if (hasISOLinuxBootloader(imagePath)) {
        bootLoaders << "ISOLINUX";
    }
    if (hasSyslinuxBootloader(imagePath)) {
        bootLoaders << "SYSLINUX";
    }
    if (hasGRUBBootloader(imagePath)) {
        bootLoaders << "GRUB";
    }
    if (hasEFIBootloader(imagePath)) {
        bootLoaders << "EFI";
    }
    
    return bootLoaders;
}

QString ImageHandler::detectArchitecture(const QString &imagePath)
{
    // Try to mount and analyze the image
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return QString();
    }
    
    QString mountPoint = tempDir.path() + "/mount";
    QDir().mkpath(mountPoint);
    
    QProcess mount;
    mount.start("mount", QStringList() << "-o" << "loop,ro" << imagePath << mountPoint);
    mount.waitForFinished(5000);
    
    if (mount.exitCode() != 0) {
        return QString();
    }
    
    QString arch;
    
    // Look for common architecture indicators
    QStringList archPaths = {
        mountPoint + "/boot/vmlinuz*",
        mountPoint + "/casper/vmlinuz*",
        mountPoint + "/live/vmlinuz*"
    };
    
    for (const QString &pattern : archPaths) {
        QProcess find;
        find.start("find", QStringList() << mountPoint << "-name" << QFileInfo(pattern).fileName());
        find.waitForFinished(2000);
        
        QString kernelPath = QString::fromUtf8(find.readAllStandardOutput()).trimmed();
        if (!kernelPath.isEmpty()) {
            arch = detectArchitectureFromELF(kernelPath);
            if (!arch.isEmpty()) {
                break;
            }
        }
    }
    
    // Unmount
    QProcess umount;
    umount.start("umount", QStringList() << mountPoint);
    umount.waitForFinished(5000);
    
    return arch;
}

QString ImageHandler::formatSize(qint64 bytes)
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

bool ImageHandler::isImageFile(const QString &filePath)
{
    return detectImageType(filePath) != ImageType::Unknown;
}

bool ImageHandler::analyzeISOImage(const QString &imagePath, ImageInfo &info)
{
    // Use isoinfo to get ISO details
    QProcess isoinfo;
    isoinfo.start("isoinfo", QStringList() << "-d" << "-i" << imagePath);
    isoinfo.waitForFinished(5000);
    
    if (isoinfo.exitCode() != 0) {
        info.errorMessage = "Failed to analyze ISO image";
        return false;
    }
    
    QString output = QString::fromUtf8(isoinfo.readAllStandardOutput());
    QStringList lines = output.split('\n');
    
    for (const QString &line : lines) {
        if (line.startsWith("Volume id:")) {
            info.label = line.mid(11).trimmed();
        }
    }
    
    info.fileSystem = "ISO 9660";
    return true;
}

bool ImageHandler::analyzeIMGImage(const QString &imagePath, ImageInfo &info)
{
    // Use file command to get basic info
    QProcess file;
    file.start("file", QStringList() << imagePath);
    file.waitForFinished(5000);
    
    if (file.exitCode() != 0) {
        info.errorMessage = "Failed to analyze IMG image";
        return false;
    }
    
    QString output = QString::fromUtf8(file.readAllStandardOutput());
    
    if (output.contains("filesystem")) {
        if (output.contains("ext")) {
            info.fileSystem = "ext";
        } else if (output.contains("FAT")) {
            info.fileSystem = "FAT";
        } else if (output.contains("NTFS")) {
            info.fileSystem = "NTFS";
        }
    }
    
    return true;
}

bool ImageHandler::analyzeDMGImage(const QString &imagePath, ImageInfo &info)
{
    Q_UNUSED(imagePath)
    info.fileSystem = "HFS+";
    info.errorMessage = "DMG support requires additional tools";
    return false;
}

bool ImageHandler::analyzeVHDImage(const QString &imagePath, ImageInfo &info)
{
    Q_UNUSED(imagePath)
    info.fileSystem = "VHD";
    info.errorMessage = "VHD support requires additional tools";
    return false;
}

bool ImageHandler::hasISOLinuxBootloader(const QString &imagePath)
{
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return false;
    }
    
    QString mountPoint = tempDir.path() + "/mount";
    QDir().mkpath(mountPoint);
    
    QProcess mount;
    mount.start("mount", QStringList() << "-o" << "loop,ro" << imagePath << mountPoint);
    mount.waitForFinished(5000);
    
    if (mount.exitCode() != 0) {
        return false;
    }
    
    bool hasIsolinux = QFile::exists(mountPoint + "/isolinux/isolinux.bin") ||
                       QFile::exists(mountPoint + "/boot/isolinux/isolinux.bin");
    
    QProcess umount;
    umount.start("umount", QStringList() << mountPoint);
    umount.waitForFinished(5000);
    
    return hasIsolinux;
}

bool ImageHandler::hasSyslinuxBootloader(const QString &imagePath)
{
    Q_UNUSED(imagePath)
    // Implementation for syslinux detection
    return false;
}

bool ImageHandler::hasGRUBBootloader(const QString &imagePath)
{
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return false;
    }
    
    QString mountPoint = tempDir.path() + "/mount";
    QDir().mkpath(mountPoint);
    
    QProcess mount;
    mount.start("mount", QStringList() << "-o" << "loop,ro" << imagePath << mountPoint);
    mount.waitForFinished(5000);
    
    if (mount.exitCode() != 0) {
        return false;
    }
    
    bool hasGrub = QFile::exists(mountPoint + "/boot/grub") ||
                   QFile::exists(mountPoint + "/boot/grub2");
    
    QProcess umount;
    umount.start("umount", QStringList() << mountPoint);
    umount.waitForFinished(5000);
    
    return hasGrub;
}

bool ImageHandler::hasEFIBootloader(const QString &imagePath)
{
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return false;
    }
    
    QString mountPoint = tempDir.path() + "/mount";
    QDir().mkpath(mountPoint);
    
    QProcess mount;
    mount.start("mount", QStringList() << "-o" << "loop,ro" << imagePath << mountPoint);
    mount.waitForFinished(5000);
    
    if (mount.exitCode() != 0) {
        return false;
    }
    
    bool hasEfi = QFile::exists(mountPoint + "/EFI") ||
                  QFile::exists(mountPoint + "/efi");
    
    QProcess umount;
    umount.start("umount", QStringList() << mountPoint);
    umount.waitForFinished(5000);
    
    return hasEfi;
}

bool ImageHandler::hasMBRBootloader(const QString &imagePath)
{
    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    // Check for MBR signature
    file.seek(510);
    QByteArray signature = file.read(2);
    
    return signature == QByteArray::fromHex("55AA");
}

QString ImageHandler::detectFileSystemFromISO(const QString &imagePath)
{
    Q_UNUSED(imagePath)
    return "ISO 9660";
}

QString ImageHandler::detectFileSystemFromDevice(const QString &imagePath)
{
    QProcess blkid;
    blkid.start("blkid", QStringList() << imagePath);
    blkid.waitForFinished(2000);
    
    QString output = QString::fromUtf8(blkid.readAllStandardOutput());
    
    if (output.contains("TYPE=\"")) {
        int start = output.indexOf("TYPE=\"") + 6;
        int end = output.indexOf("\"", start);
        return output.mid(start, end - start);
    }
    
    return QString();
}

QString ImageHandler::detectArchitectureFromELF(const QString &imagePath)
{
    QProcess file;
    file.start("file", QStringList() << imagePath);
    file.waitForFinished(2000);
    
    QString output = QString::fromUtf8(file.readAllStandardOutput());
    
    if (output.contains("x86-64")) {
        return "x86_64";
    } else if (output.contains("i386")) {
        return "i386";
    } else if (output.contains("ARM")) {
        return "ARM";
    } else if (output.contains("aarch64")) {
        return "aarch64";
    }
    
    return QString();
}

QString ImageHandler::detectArchitectureFromPE(const QString &imagePath)
{
    Q_UNUSED(imagePath)
    // Implementation for PE (Windows) executable analysis
    return QString();
}
