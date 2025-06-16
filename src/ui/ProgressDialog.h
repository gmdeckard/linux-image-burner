#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    
    void setTitle(const QString &title);
    void setProgress(int percentage);
    void setStatus(const QString &status);
    void setSpeed(const QString &speed);
    void setTimeRemaining(const QString &time);
    void addLogMessage(const QString &message);
    void setFinished(bool success, const QString &message);

signals:
    void cancelled();

private slots:
    void onCancelClicked();
    void toggleDetails();

private:
    void setupUi();
    
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_speedLabel;
    QLabel *m_timeLabel;
    QPushButton *m_cancelButton;
    QPushButton *m_detailsButton;
    QTextEdit *m_logText;
    
    bool m_detailsVisible;
    bool m_finished;
};

#endif // PROGRESSDIALOG_H
