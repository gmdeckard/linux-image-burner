#ifndef IMAGEHANDLER_H
#define IMAGEHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>

enum class ImageType {
    Unknown,
    ISO,
    IMG,
    DMG,
    VHD,
    VHDX,
    VMDK
};

struct ImageInfo {
    QString filePath;
    ImageType type;
    qint64 size;
    QString sizeString;
    bool isBootable;
    QString label;
    QString fileSystem;
    QString architecture;
    QStringList bootLoaders;
    bool isValid;
    QString errorMessage;
};

class ImageHandler : public QObject
{
    Q_OBJECT

public:
    explicit ImageHandler(QObject *parent = nullptr);

    // Image analysis
    ImageInfo analyzeImage(const QString &imagePath);
    bool validateImage(const QString &imagePath);
    
    // Image type detection
    static ImageType detectImageType(const QString &imagePath);
    static QString imageTypeToString(ImageType type);
    static QStringList getSupportedExtensions();
    
    // Image properties
    bool isImageBootable(const QString &imagePath);
    QString getImageLabel(const QString &imagePath);
    QString getImageFileSystem(const QString &imagePath);
    QStringList detectBootLoaders(const QString &imagePath);
    QString detectArchitecture(const QString &imagePath);
    
    // Utility functions
    static QString formatSize(qint64 bytes);
    static bool isImageFile(const QString &filePath);

signals:
    void analysisProgress(int percentage);
    void analysisFinished(const ImageInfo &info);

private:
    // Helper methods
    bool analyzeISOImage(const QString &imagePath, ImageInfo &info);
    bool analyzeIMGImage(const QString &imagePath, ImageInfo &info);
    bool analyzeDMGImage(const QString &imagePath, ImageInfo &info);
    bool analyzeVHDImage(const QString &imagePath, ImageInfo &info);
    
    // Boot detection
    bool hasISOLinuxBootloader(const QString &imagePath);
    bool hasSyslinuxBootloader(const QString &imagePath);
    bool hasGRUBBootloader(const QString &imagePath);
    bool hasEFIBootloader(const QString &imagePath);
    bool hasMBRBootloader(const QString &imagePath);
    
    // File system detection
    QString detectFileSystemFromISO(const QString &imagePath);
    QString detectFileSystemFromDevice(const QString &imagePath);
    
    // Architecture detection
    QString detectArchitectureFromELF(const QString &imagePath);
    QString detectArchitectureFromPE(const QString &imagePath);
};

#endif // IMAGEHANDLER_H
