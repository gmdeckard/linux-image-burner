#include "ProgressDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent)
    , m_detailsVisible(false)
    , m_finished(false)
{
    setWindowTitle("Operation Progress");
    setModal(true);
    setupUi();
}

void ProgressDialog::setupUi()
{
    setFixedSize(400, 150);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    mainLayout->addWidget(m_progressBar);
    
    // Status label
    m_statusLabel = new QLabel("Initializing...");
    mainLayout->addWidget(m_statusLabel);
    
    // Speed and time info
    QHBoxLayout *infoLayout = new QHBoxLayout();
    m_speedLabel = new QLabel();
    m_timeLabel = new QLabel();
    
    infoLayout->addWidget(m_speedLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(m_timeLabel);
    
    mainLayout->addLayout(infoLayout);
    
    // Log text (initially hidden)
    m_logText = new QTextEdit();
    m_logText->setMaximumHeight(100);
    m_logText->setReadOnly(true);
    m_logText->setVisible(false);
    mainLayout->addWidget(m_logText);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_detailsButton = new QPushButton("Show Details");
    m_cancelButton = new QPushButton("Cancel");
    
    buttonLayout->addWidget(m_detailsButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(m_cancelButton, &QPushButton::clicked, this, &ProgressDialog::onCancelClicked);
    connect(m_detailsButton, &QPushButton::clicked, this, &ProgressDialog::toggleDetails);
}

void ProgressDialog::setTitle(const QString &title)
{
    setWindowTitle(title);
}

void ProgressDialog::setProgress(int percentage)
{
    m_progressBar->setValue(percentage);
}

void ProgressDialog::setStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

void ProgressDialog::setSpeed(const QString &speed)
{
    m_speedLabel->setText(speed);
}

void ProgressDialog::setTimeRemaining(const QString &time)
{
    m_timeLabel->setText(time);
}

void ProgressDialog::addLogMessage(const QString &message)
{
    m_logText->append(message);
}

void ProgressDialog::setFinished(bool success, const QString &message)
{
    m_finished = true;
    m_cancelButton->setText("Close");
    
    if (success) {
        m_statusLabel->setText("Operation completed successfully");
        m_progressBar->setValue(100);
    } else {
        m_statusLabel->setText("Operation failed: " + message);
    }
    
    m_speedLabel->clear();
    m_timeLabel->clear();
}

void ProgressDialog::onCancelClicked()
{
    if (m_finished) {
        accept();
    } else {
        emit cancelled();
    }
}

void ProgressDialog::toggleDetails()
{
    m_detailsVisible = !m_detailsVisible;
    m_logText->setVisible(m_detailsVisible);
    m_detailsButton->setText(m_detailsVisible ? "Hide Details" : "Show Details");
    
    if (m_detailsVisible) {
        setFixedSize(400, 300);
    } else {
        setFixedSize(400, 150);
    }
}
