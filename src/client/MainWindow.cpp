#include "MainWindow.h"
#include "SettingsDialog.h"
#include "Logger.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();

    // Try loading client.ini from default paths
    if (!m_config.load("configs/client.ini")) {
        m_config.load("client.ini");
    }

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    LOG_INFO("Client GUI initialized (C++ programmatic layout)");
}

void MainWindow::setupUi()
{
    setWindowTitle("RabbitMQ Client (Qt5)");
    resize(600, 450);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Top panel: input and send button
    QHBoxLayout* inputLayout = new QHBoxLayout();
    QLabel* inputLabel = new QLabel("Число для удвоения:", this);
    m_numberSpinBox = new QSpinBox(this);
    m_numberSpinBox->setRange(-1000000, 1000000);
    m_numberSpinBox->setValue(21);

    m_sendButton = new QPushButton("Отправить запрос", this);

    inputLayout->addWidget(inputLabel);
    inputLayout->addWidget(m_numberSpinBox);
    inputLayout->addWidget(m_sendButton);
    mainLayout->addLayout(inputLayout);

    // Center: results table
    m_resultsTable = new QTableWidget(this);
    m_resultsTable->setColumnCount(4);
    QStringList headers = {"ID запроса", "Запрос (req)", "Ответ (res)", "Статус"};
    m_resultsTable->setHorizontalHeaderLabels(headers);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_resultsTable);

    // Bottom panel: status and settings button
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Статус: Не подключен", this);
    m_settingsButton = new QPushButton("Настройки...", this);

    bottomLayout->addWidget(m_statusLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_settingsButton);
    mainLayout->addLayout(bottomLayout);
}

void MainWindow::onSendClicked()
{
    int val = m_numberSpinBox->value();
    LOG_INFO(QString("Send clicked with number: %1").arg(val));
    // Background RabbitMQ worker integration will be completed in Step 3
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dialog(m_config, this);
    if (dialog.exec() == QDialog::Accepted) {
        LOG_INFO("Settings updated and saved via GUI");
    }
}
