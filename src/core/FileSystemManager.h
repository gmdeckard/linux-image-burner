#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>

struct FileSystemInfo {
    QString name;
    QString displayName;
    QStringList supportedSizes;
    int minClusterSize;
    int maxClusterSize;
    int defaultClusterSize;
    qint64 maxVolumeSize;
    bool supportsCompression;
    bool supportsEncryption;
    bool isBootable;
};

class FileSystemManager : public QObject
{
    Q_OBJECT

public:
    explicit FileSystemManager(QObject *parent = nullptr);

    // File system information
    static QList<FileSystemInfo> getSupportedFileSystems();
    static FileSystemInfo getFileSystemInfo(const QString &fsType);
    static QStringList getRecommendedFileSystems(qint64 deviceSize);
    
    // Cluster size calculations
    static QStringList getAvailableClusterSizes(const QString &fsType, qint64 volumeSize);
    static int getRecommendedClusterSize(const QString &fsType, qint64 volumeSize);
    static int getOptimalClusterSize(const QString &fsType, qint64 volumeSize);
    
    // Validation
    static bool isValidVolumeLabel(const QString &fsType, const QString &label);
    static QString sanitizeVolumeLabel(const QString &fsType, const QString &label);
    static bool isValidClusterSize(const QString &fsType, int clusterSize);
    
    // Utility functions
    static QString formatSizeToString(qint64 bytes);
    static qint64 calculateUsableSpace(const QString &fsType, qint64 totalSpace);
    static double getOverheadPercentage(const QString &fsType);

private:
    static void initializeFileSystemInfo();
    static QList<FileSystemInfo> s_fileSystems;
    static bool s_initialized;
};

#endif // FILESYSTEMMANAGER_H
