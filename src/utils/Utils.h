#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QStringList>
#include <QDateTime>

class Utils
{
public:
    // File and path utilities
    static QString getFileName(const QString &filePath);
    static QString getFileExtension(const QString &filePath);
    static QString getFileBaseName(const QString &filePath);
    static QString getDirectoryPath(const QString &filePath);
    static bool isValidFilePath(const QString &filePath);
    static bool isValidDevicePath(const QString &devicePath);
    
    // Size formatting
    static QString formatBytes(qint64 bytes, int precision = 2);
    static QString formatBytesPerSecond(qint64 bytesPerSecond);
    static qint64 parseSize(const QString &sizeString);
    
    // Time formatting
    static QString formatDuration(qint64 seconds);
    static QString formatDurationMs(qint64 milliseconds);
    static QString formatTimeRemaining(qint64 seconds);
    static QString getCurrentDateTime();
    
    // String utilities
    static QString sanitizeFileName(const QString &fileName);
    static QString capitalizeFirst(const QString &text);
    static QStringList splitCommandLine(const QString &command);
    static QString joinWithCommas(const QStringList &list);
    
    // System utilities
    static bool isRunningAsRoot();
    static QString getSystemArchitecture();
    static QString getKernelVersion();
    static QString getDistributionName();
    static qint64 getAvailableMemory();
    static qint64 getTotalMemory();
    
    // File operations
    static bool isFileReadable(const QString &filePath);
    static bool isFileWritable(const QString &filePath);
    static qint64 getFileSize(const QString &filePath);
    static QString getFileMD5(const QString &filePath);
    static QString getFileSHA256(const QString &filePath);
    
    // Device utilities
    static QStringList getBlockDevices();
    static QStringList getRemovableDevices();
    static bool isBlockDevice(const QString &devicePath);
    static bool isCharacterDevice(const QString &devicePath);
    
    // Math utilities
    static int roundToNearestPowerOfTwo(int value);
    static bool isPowerOfTwo(int value);
    static int getNextPowerOfTwo(int value);
    static double calculatePercentage(qint64 current, qint64 total);
    
    // Validation utilities
    static bool isValidIPAddress(const QString &ip);
    static bool isValidMacAddress(const QString &mac);
    static bool isValidUUID(const QString &uuid);
    static bool isValidHexString(const QString &hex);
    
private:
    Utils() = delete; // Static class, no instantiation
};

#endif // UTILS_H
