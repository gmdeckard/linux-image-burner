#ifndef BURNER_H
#define BURNER_H

#include <QObject>
#include <QThread>
#include <QProcess>
#include <QTimer>
#include <QMutex>
#include <QDateTime>

enum class BurnMode {
    DDMode,          // Direct disk copy (dd)
    ISOHybridMode,   // ISO hybrid mode
    UEFIMode,        // UEFI compatible mode
    WindowsToGo      // Windows To Go mode
};

enum class PartitionScheme {
    MBR,
    GPT
};

enum class FileSystem {
    FAT32,
    NTFS,
    exFAT,
    ext4
};

struct BurnOptions {
    QString imagePath;
    QString devicePath;
    BurnMode mode;
    PartitionScheme partitionScheme;
    FileSystem fileSystem;
    QString volumeLabel;
    bool quickFormat;
    bool verifyAfterBurn;
    bool createBootableUSB;
    bool addFixupFiles;
    int clusterSize;
    bool badBlockCheck;
};

class Burner : public QObject
{
    Q_OBJECT

public:
    explicit Burner(QObject *parent = nullptr);
    ~Burner();

    // Main burning operations
    void burnImage(const BurnOptions &options);
    void formatDevice(const QString &devicePath, FileSystem fs, const QString &label = QString());
    void verifyBurn(const QString &imagePath, const QString &devicePath);
    
    // Control operations
    void cancel();
    void pause();
    void resume();
    
    // Status
    bool isBurning() const { return m_isBurning; }
    bool isPaused() const { return m_isPaused; }
    bool isCancelled() const { return m_isCancelled; }

signals:
    void progressChanged(int percentage);
    void speedChanged(const QString &speed);
    void statusChanged(const QString &status);
    void timeRemainingChanged(const QString &timeRemaining);
    void burnStarted();
    void burnFinished(bool success, const QString &message);
    void verificationStarted();
    void verificationFinished(bool success, const QString &message);
    void error(const QString &message);

private slots:
    void onProgressTimer();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessOutput();

private:
    bool m_isBurning;
    bool m_isPaused;
    bool m_isCancelled;
    
    QProcess *m_process;
    QTimer *m_progressTimer;
    QMutex m_mutex;
    
    BurnOptions m_currentOptions;
    qint64 m_totalBytes;
    qint64 m_bytesWritten;
    QDateTime m_startTime;
    QDateTime m_lastUpdateTime;
    qint64 m_lastBytesWritten;
    
    // Helper methods
    bool prepareDevice(const QString &devicePath, const BurnOptions &options);
    bool createPartitionTable(const QString &devicePath, PartitionScheme scheme);
    bool createPartition(const QString &devicePath, FileSystem fs, const QString &label);
    bool formatPartition(const QString &partitionPath, FileSystem fs, const QString &label);
    bool burnWithDD(const BurnOptions &options);
    bool burnWithUEFI(const BurnOptions &options);
    bool burnWithWindowsToGo(const BurnOptions &options);
    bool addBootFiles(const QString &devicePath, const BurnOptions &options);
    
    // Progress tracking
    void updateProgress();
    qint64 getBytesWritten(const QString &devicePath);
    QString calculateSpeed(qint64 bytes, qint64 timeMs);
    QString calculateTimeRemaining(qint64 bytesRemaining, double speedBytesPerSec);
    
    // Utility functions
    QString getFileSystemCommand(FileSystem fs);
    QStringList getFormatArguments(FileSystem fs, const QString &label, int clusterSize);
    bool unmountDevice(const QString &devicePath);
    bool syncDevice(const QString &devicePath);
    QString getPartitionPath(const QString &devicePath, int partitionNumber);
    
    // Verification
    bool verifyImageChecksum(const QString &imagePath, const QString &devicePath);
    QString calculateMD5(const QString &filePath);
    QString calculateSHA256(const QString &filePath);
};

#endif // BURNER_H
