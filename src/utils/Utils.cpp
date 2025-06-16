#include "Utils.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QDateTime>
#include <QStandardPaths>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

QString Utils::getFileName(const QString &filePath)
{
    return QFileInfo(filePath).fileName();
}

QString Utils::getFileExtension(const QString &filePath)
{
    return QFileInfo(filePath).suffix();
}

QString Utils::getFileBaseName(const QString &filePath)
{
    return QFileInfo(filePath).baseName();
}

QString Utils::getDirectoryPath(const QString &filePath)
{
    return QFileInfo(filePath).absolutePath();
}

bool Utils::isValidFilePath(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }
    
    QFileInfo info(filePath);
    return info.exists() && info.isFile();
}

bool Utils::isValidDevicePath(const QString &devicePath)
{
    if (devicePath.isEmpty() || !devicePath.startsWith("/dev/")) {
        return false;
    }
    
    return isBlockDevice(devicePath);
}

QString Utils::formatBytes(qint64 bytes, int precision)
{
    const QStringList units = {"B", "KB", "MB", "GB", "TB", "PB"};
    double size = bytes;
    int unitIndex = 0;
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    return QString::number(size, 'f', (unitIndex == 0) ? 0 : precision) + " " + units[unitIndex];
}

QString Utils::formatBytesPerSecond(qint64 bytesPerSecond)
{
    return formatBytes(bytesPerSecond) + "/s";
}

qint64 Utils::parseSize(const QString &sizeString)
{
    QString cleanString = sizeString.trimmed().toUpper();
    QRegularExpression re("^(\\d+(?:\\.\\d+)?)\\s*([KMGTPE]?B?)$");
    QRegularExpressionMatch match = re.match(cleanString);
    
    if (!match.hasMatch()) {
        return -1;
    }
    
    double value = match.captured(1).toDouble();
    QString unit = match.captured(2);
    
    if (unit.isEmpty() || unit == "B") {
        return qint64(value);
    } else if (unit == "KB" || unit == "K") {
        return qint64(value * 1024);
    } else if (unit == "MB" || unit == "M") {
        return qint64(value * 1024 * 1024);
    } else if (unit == "GB" || unit == "G") {
        return qint64(value * 1024 * 1024 * 1024);
    } else if (unit == "TB" || unit == "T") {
        return qint64(value * 1024LL * 1024 * 1024 * 1024);
    } else if (unit == "PB" || unit == "P") {
        return qint64(value * 1024LL * 1024 * 1024 * 1024 * 1024);
    }
    
    return -1;
}

QString Utils::formatDuration(qint64 seconds)
{
    if (seconds < 60) {
        return QString("%1s").arg(seconds);
    } else if (seconds < 3600) {
        int mins = seconds / 60;
        int secs = seconds % 60;
        return QString("%1m %2s").arg(mins).arg(secs);
    } else {
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        int secs = seconds % 60;
        return QString("%1h %2m %3s").arg(hours).arg(mins).arg(secs);
    }
}

QString Utils::formatDurationMs(qint64 milliseconds)
{
    return formatDuration(milliseconds / 1000);
}

QString Utils::formatTimeRemaining(qint64 seconds)
{
    if (seconds < 0) {
        return "Unknown";
    }
    
    return formatDuration(seconds);
}

QString Utils::getCurrentDateTime()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

QString Utils::sanitizeFileName(const QString &fileName)
{
    QString sanitized = fileName;
    
    // Remove or replace invalid characters
    QRegularExpression invalidChars("[<>:\"/\\\\|?*]");
    sanitized.replace(invalidChars, "_");
    
    // Remove leading/trailing spaces and dots
    sanitized = sanitized.trimmed();
    while (sanitized.endsWith('.')) {
        sanitized.chop(1);
    }
    
    // Ensure it's not empty
    if (sanitized.isEmpty()) {
        sanitized = "untitled";
    }
    
    return sanitized;
}

QString Utils::capitalizeFirst(const QString &text)
{
    if (text.isEmpty()) {
        return text;
    }
    
    return text.at(0).toUpper() + text.mid(1);
}

QStringList Utils::splitCommandLine(const QString &command)
{
    QStringList args;
    QString current;
    bool inQuotes = false;
    bool escape = false;
    
    for (const QChar &c : command) {
        if (escape) {
            current += c;
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!current.isEmpty()) {
                args << current;
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.isEmpty()) {
        args << current;
    }
    
    return args;
}

QString Utils::joinWithCommas(const QStringList &list)
{
    if (list.isEmpty()) {
        return QString();
    } else if (list.size() == 1) {
        return list.first();
    } else if (list.size() == 2) {
        return list.first() + " and " + list.last();
    } else {
        QStringList copy = list;
        QString last = copy.takeLast();
        return copy.join(", ") + ", and " + last;
    }
}

bool Utils::isRunningAsRoot()
{
    return geteuid() == 0;
}

QString Utils::getSystemArchitecture()
{
    QProcess uname;
    uname.start("uname", QStringList() << "-m");
    uname.waitForFinished(2000);
    
    if (uname.exitCode() == 0) {
        return QString::fromUtf8(uname.readAllStandardOutput()).trimmed();
    }
    
    return "unknown";
}

QString Utils::getKernelVersion()
{
    QProcess uname;
    uname.start("uname", QStringList() << "-r");
    uname.waitForFinished(2000);
    
    if (uname.exitCode() == 0) {
        return QString::fromUtf8(uname.readAllStandardOutput()).trimmed();
    }
    
    return "unknown";
}

QString Utils::getDistributionName()
{
    // Try lsb_release first
    QProcess lsb;
    lsb.start("lsb_release", QStringList() << "-d" << "-s");
    lsb.waitForFinished(2000);
    
    if (lsb.exitCode() == 0) {
        QString result = QString::fromUtf8(lsb.readAllStandardOutput()).trimmed();
        result.remove('"');
        return result;
    }
    
    // Try reading /etc/os-release
    QFile osRelease("/etc/os-release");
    if (osRelease.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(osRelease.readAll());
        QRegularExpression re("PRETTY_NAME=\"?([^\"\\n]+)\"?");
        QRegularExpressionMatch match = re.match(content);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    
    return "Linux";
}

qint64 Utils::getAvailableMemory()
{
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.freeram * info.mem_unit;
    }
    return 0;
}

qint64 Utils::getTotalMemory()
{
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.totalram * info.mem_unit;
    }
    return 0;
}

bool Utils::isFileReadable(const QString &filePath)
{
    return QFileInfo(filePath).isReadable();
}

bool Utils::isFileWritable(const QString &filePath)
{
    return QFileInfo(filePath).isWritable();
}

qint64 Utils::getFileSize(const QString &filePath)
{
    return QFileInfo(filePath).size();
}

QString Utils::getFileMD5(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QCryptographicHash hash(QCryptographicHash::Md5);
    if (hash.addData(&file)) {
        return hash.result().toHex();
    }
    
    return QString();
}

QString Utils::getFileSHA256(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (hash.addData(&file)) {
        return hash.result().toHex();
    }
    
    return QString();
}

QStringList Utils::getBlockDevices()
{
    QStringList devices;
    QDir devDir("/dev");
    
    QStringList filters;
    filters << "sd[a-z]" << "hd[a-z]" << "mmcblk[0-9]*" << "nvme[0-9]*n[0-9]*";
    devDir.setNameFilters(filters);
    
    QStringList deviceNames = devDir.entryList(QDir::System);
    
    for (const QString &deviceName : deviceNames) {
        // Skip partitions
        if (deviceName.contains(QRegularExpression("\\d+$"))) {
            continue;
        }
        
        QString devicePath = "/dev/" + deviceName;
        if (isBlockDevice(devicePath)) {
            devices << devicePath;
        }
    }
    
    return devices;
}

QStringList Utils::getRemovableDevices()
{
    QStringList removableDevices;
    QStringList allDevices = getBlockDevices();
    
    for (const QString &device : allDevices) {
        QString deviceName = QFileInfo(device).fileName();
        QString removablePath = QString("/sys/block/%1/removable").arg(deviceName);
        
        QFile removableFile(removablePath);
        if (removableFile.open(QIODevice::ReadOnly)) {
            QString removable = QString::fromUtf8(removableFile.readAll()).trimmed();
            if (removable == "1") {
                removableDevices << device;
            }
        }
    }
    
    return removableDevices;
}

bool Utils::isBlockDevice(const QString &devicePath)
{
    struct stat st;
    if (stat(devicePath.toLocal8Bit().data(), &st) == 0) {
        return S_ISBLK(st.st_mode);
    }
    return false;
}

bool Utils::isCharacterDevice(const QString &devicePath)
{
    struct stat st;
    if (stat(devicePath.toLocal8Bit().data(), &st) == 0) {
        return S_ISCHR(st.st_mode);
    }
    return false;
}

int Utils::roundToNearestPowerOfTwo(int value)
{
    if (value <= 0) {
        return 1;
    }
    
    int power = 1;
    while (power < value) {
        power *= 2;
    }
    
    // Choose the closer power of 2
    int lowerPower = power / 2;
    if (value - lowerPower < power - value) {
        return lowerPower;
    } else {
        return power;
    }
}

bool Utils::isPowerOfTwo(int value)
{
    return value > 0 && (value & (value - 1)) == 0;
}

int Utils::getNextPowerOfTwo(int value)
{
    if (value <= 0) {
        return 1;
    }
    
    int power = 1;
    while (power < value) {
        power *= 2;
    }
    
    return power;
}

double Utils::calculatePercentage(qint64 current, qint64 total)
{
    if (total == 0) {
        return 0.0;
    }
    
    return (double(current) / double(total)) * 100.0;
}

bool Utils::isValidIPAddress(const QString &ip)
{
    QRegularExpression ipv4Regex("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return ipv4Regex.match(ip).hasMatch();
}

bool Utils::isValidMacAddress(const QString &mac)
{
    QRegularExpression macRegex("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$");
    return macRegex.match(mac).hasMatch();
}

bool Utils::isValidUUID(const QString &uuid)
{
    QRegularExpression uuidRegex("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
    return uuidRegex.match(uuid).hasMatch();
}

bool Utils::isValidHexString(const QString &hex)
{
    QRegularExpression hexRegex("^[0-9a-fA-F]+$");
    return hexRegex.match(hex).hasMatch();
}
