#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QLineEdit>
#include <QCheckBox>
#include "../core/DeviceManager.h"
#include "../core/ImageHandler.h"
#include "../core/Burner.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMenuBar;
class QStatusBar;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void selectImage();
    void refreshDevices();
    void deviceSelectionChanged();
    void fileSystemChanged();
    void startBurn();
    void cancelBurn();
    void formatDevice();
    void showDeviceInfo();
    void showAbout();
    void showLog();
    void toggleAdvancedOptions();
    
    // Burner slots
    void onBurnStarted();
    void onBurnFinished(bool success, const QString &message);
    void onProgressChanged(int percentage);
    void onSpeedChanged(const QString &speed);
    void onStatusChanged(const QString &status);
    void onTimeRemainingChanged(const QString &timeRemaining);
    void onBurnerError(const QString &message);
    
    // Device manager slots
    void onDeviceListChanged();
    void onDeviceInserted(const QString &devicePath);
    void onDeviceRemoved(const QString &devicePath);

private:
    void setupUi();
    void setupMenuBar();
    void setupStatusBar();
    void setupConnections();
    void updateDeviceList();
    void updateImageInfo();
    void updateBurnOptions();
    void validateInputs();
    void logMessage(const QString &message, const QString &level = "INFO");
    BurnOptions getBurnOptions();
    
    // UI Components
    QWidget *m_centralWidget;
    
    // Image selection
    QGroupBox *m_imageGroup;
    QLabel *m_imageLabel;
    QPushButton *m_selectImageButton;
    QLabel *m_imageInfoLabel;
    
    // Device selection
    QGroupBox *m_deviceGroup;
    QComboBox *m_deviceCombo;
    QPushButton *m_refreshButton;
    QPushButton *m_deviceInfoButton;
    QLabel *m_deviceInfoLabel;
    
    // File system options
    QGroupBox *m_fileSystemGroup;
    QComboBox *m_fileSystemCombo;
    QLabel *m_volumeLabelLabel;
    QLineEdit *m_volumeLabelEdit;
    QComboBox *m_clusterSizeCombo;
    QLabel *m_partitionSchemeLabel;
    QComboBox *m_partitionSchemeCombo;
    
    // Advanced options
    QGroupBox *m_advancedGroup;
    QCheckBox *m_quickFormatCheck;
    QCheckBox *m_verifyCheck;
    QCheckBox *m_createBootableCheck;
    QCheckBox *m_badBlockCheck;
    QPushButton *m_advancedToggle;
    
    // Progress
    QGroupBox *m_progressGroup;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_speedLabel;
    QLabel *m_timeLabel;
    
    // Actions
    QPushButton *m_startButton;
    QPushButton *m_cancelButton;
    QPushButton *m_formatButton;
    QPushButton *m_logButton;
    
    // Log
    QTextEdit *m_logText;
    bool m_logVisible;
    
    // Menu and status
    QMenuBar *m_menuBar;
    QStatusBar *m_statusBar;
    QLabel *m_statusBarLabel;
    
    // Core components
    DeviceManager *m_deviceManager;
    ImageHandler *m_imageHandler;
    Burner *m_burner;
    
    // State
    QString m_selectedImagePath;
    QString m_selectedDevicePath;
    bool m_isBurning;
    bool m_advancedVisible;
    
    // Timer for UI updates
    QTimer *m_updateTimer;
};

#endif // MAINWINDOW_H
