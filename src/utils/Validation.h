#ifndef VALIDATION_H
#define VALIDATION_H

#include <QString>
#include <QStringList>

class Validation
{
public:
    // Image file validation
    static bool isValidImageFile(const QString &filePath);
    static bool isValidImageFormat(const QString &filePath);
    static bool isImageAccessible(const QString &filePath);
    static QString getImageValidationError(const QString &filePath);
    
    // Device validation
    static bool isValidDevice(const QString &devicePath);
    static bool isDeviceWritable(const QString &devicePath);
    static bool isDeviceRemovable(const QString &devicePath);
    static bool isDeviceSafeToWrite(const QString &devicePath);
    static QString getDeviceValidationError(const QString &devicePath);
    
    // File system validation
    static bool isValidFileSystemType(const QString &fsType);
    static bool isValidVolumeLabel(const QString &fsType, const QString &label);
    static bool isValidClusterSize(const QString &fsType, int clusterSize);
    static bool isFileSystemCompatible(const QString &fsType, qint64 deviceSize);
    
    // Size validation
    static bool isValidSize(qint64 size);
    static bool isImageFitsOnDevice(const QString &imagePath, const QString &devicePath);
    static bool isImageFitsOnDevice(qint64 imageSize, qint64 deviceSize);
    
    // Permission validation
    static bool hasRequiredPermissions();
    static bool canAccessDevice(const QString &devicePath);
    static bool canWriteToDevice(const QString &devicePath);
    
    // Safety checks
    static bool isSystemDisk(const QString &devicePath);
    static bool isMountedDevice(const QString &devicePath);
    static bool hasImportantData(const QString &devicePath);
    static QStringList getWarnings(const QString &devicePath);
    
    // Burn options validation
    static bool validateBurnOptions(const QString &imagePath, const QString &devicePath, 
                                   const QString &fsType, const QString &label);
    static QStringList getBurnOptionsErrors(const QString &imagePath, const QString &devicePath,
                                           const QString &fsType, const QString &label);
    
    // Supported formats
    static QStringList getSupportedImageFormats();
    static QStringList getSupportedFileSystemTypes();
    static bool isFormatSupported(const QString &format);
    
private:
    Validation() = delete; // Static class, no instantiation
    
    // Helper methods
    static bool checkImageFileExists(const QString &filePath);
    static bool checkImageFileReadable(const QString &filePath);
    static bool checkImageFileSize(const QString &filePath);
    static bool checkDeviceExists(const QString &devicePath);
    static bool checkDeviceBlockDevice(const QString &devicePath);
    static bool checkDevicePermissions(const QString &devicePath);
    static QStringList getSystemDisks();
    static QStringList getMountedDevices();
};

#endif // VALIDATION_H
