#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QFileSystemWatcher>

struct DeviceInfo {
    QString path;           // /dev/sdX
    QString name;           // Human readable name
    QString model;          // Device model
    QString vendor;         // Device vendor
    qint64 size;           // Size in bytes
    QString sizeString;     // Human readable size
    bool isRemovable;       // Is removable device
    bool isMounted;         // Is currently mounted
    QStringList mountPoints; // Mount points if mounted
    QString fileSystem;     // Current filesystem type
    QString uuid;           // Device UUID
    bool isUSB;            // Is USB device
    bool isMMC;            // Is MMC/SD card
};

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // Device enumeration
    QList<DeviceInfo> getRemovableDevices();
    QList<DeviceInfo> getAllStorageDevices();
    DeviceInfo getDeviceInfo(const QString &devicePath);
    
    // Device operations
    bool unmountDevice(const QString &devicePath);
    bool unmountAllPartitions(const QString &devicePath);
    bool ejectDevice(const QString &devicePath);
    
    // Device monitoring
    void startMonitoring();
    void stopMonitoring();
    
    // Utility functions
    static QString formatSize(qint64 bytes);
    static bool isDeviceWritable(const QString &devicePath);
    static bool isDeviceBusy(const QString &devicePath);

public slots:
    void refreshDevices();

signals:
    void deviceListChanged();
    void deviceInserted(const QString &devicePath);
    void deviceRemoved(const QString &devicePath);

private slots:
    void onDirectoryChanged(const QString &path);
    void onMonitorTimer();

private:
    QFileSystemWatcher *m_watcher;
    QTimer *m_monitorTimer;
    QList<DeviceInfo> m_lastDeviceList;
    
    // Helper methods
    DeviceInfo parseDeviceInfo(const QString &devicePath);
    QList<DeviceInfo> parseLsblkOutput(const QByteArray &output, bool removableOnly = false);
    qint64 parseSizeString(const QString &sizeStr);
    QString readSysfsAttribute(const QString &devicePath, const QString &attribute);
    QStringList getMountPoints(const QString &devicePath);
    bool isRemovableDevice(const QString &devicePath);
    QString getDeviceModel(const QString &devicePath);
    QString getDeviceVendor(const QString &devicePath);
    qint64 getDeviceSize(const QString &devicePath);
    QString getFileSystemType(const QString &devicePath);
    QString getDeviceUUID(const QString &devicePath);
    bool isUSBDevice(const QString &devicePath);
    bool isMMCDevice(const QString &devicePath);
};

#endif // DEVICEMANAGER_H
