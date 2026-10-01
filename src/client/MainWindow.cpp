#include "MainWindow.h"
#include "SettingsDialog.h"
#include "Logger.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    loadConfig();
    setupUi();

    // Создаем фоновый сетевой воркер на базе QThread
    m_worker = new ClientWorker(m_config, this);

    // Связываем сигналы и слоты воркера и GUI
    connect(m_worker, &ClientWorker::connected, this, &MainWindow::onConnected);
    connect(m_worker, &ClientWorker::disconnected, this, &MainWindow::onDisconnected);
    connect(m_worker, &ClientWorker::connectionError, this, &MainWindow::onConnectionError);
    connect(m_worker, &ClientWorker::statusChanged, this, &MainWindow::onStatusChanged);
    connect(m_worker, &ClientWorker::requestSent, this, &MainWindow::onRequestSent);
    connect(m_worker, &ClientWorker::responseReceived, this, &MainWindow::onResponseReceived);

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    // Запускаем фоновый поток клиента
    m_worker->start();

    LOG_INFO(QString("Клиент запущен с уникальным ID: %1").arg(m_worker->clientId()));
}

MainWindow::~MainWindow()
{
    if (m_worker) {
        m_worker->stop();
        m_worker->wait();
    }
}

void MainWindow::loadConfig()
{
    QString configPath = "configs/client.ini";
    if (!QFileInfo::exists(configPath)) {
        QString alt1 = QDir::current().filePath("../configs/client.ini");
        QString alt2 = QCoreApplication::applicationDirPath() + "/../../../configs/client.ini";
        QString alt3 = QCoreApplication::applicationDirPath() + "/../../configs/client.ini";
        if (QFileInfo::exists(alt1)) configPath = alt1;
        else if (QFileInfo::exists(alt2)) configPath = alt2;
        else if (QFileInfo::exists(alt3)) configPath = alt3;
    }
    m_config.load(configPath);
}

void MainWindow::setupUi()
{
    setWindowTitle("RabbitMQ Client (Qt5 Widgets)");
    resize(700, 480);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Верхняя панель ввода числа и отправки
    QHBoxLayout* inputLayout = new QHBoxLayout();
    QLabel* inputLabel = new QLabel("Число для удвоения:", this);
    inputLabel->setStyleSheet("font-weight: bold; font-size: 13px;");

    m_numberSpinBox = new QSpinBox(this);
    m_numberSpinBox->setRange(-1000000000, 1000000000);
    m_numberSpinBox->setValue(42);
    m_numberSpinBox->setMinimumWidth(150);

    m_sendButton = new QPushButton("Отправить запрос", this);
    m_sendButton->setStyleSheet("font-weight: bold; padding: 6px 12px;");

    inputLayout->addWidget(inputLabel);
    inputLayout->addWidget(m_numberSpinBox);
    inputLayout->addWidget(m_sendButton);
    inputLayout->addStretch();
    mainLayout->addLayout(inputLayout);

    // Центральная таблица результатов
    m_resultsTable = new QTableWidget(this);
    m_resultsTable->setColumnCount(5);
    QStringList headers = {"№", "Client ID", "Запрос (req)", "Ответ (res)", "Статус"};
    m_resultsTable->setHorizontalHeaderLabels(headers);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_resultsTable);

    // Нижняя панель со статусом и кнопкой настроек
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Статус: Инициализация...", this);
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");

    m_settingsButton = new QPushButton("⚙ Настройки...", this);

    bottomLayout->addWidget(m_statusLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_settingsButton);
    mainLayout->addLayout(bottomLayout);
}

void MainWindow::onSendClicked()
{
    int val = m_numberSpinBox->value();

    int row = m_resultsTable->rowCount();
    m_resultsTable->insertRow(row);

    m_resultsTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
    m_resultsTable->setItem(row, 1, new QTableWidgetItem(m_worker->clientId()));
    m_resultsTable->setItem(row, 2, new QTableWidgetItem(QString::number(val)));
    m_resultsTable->setItem(row, 3, new QTableWidgetItem("—"));

    auto* statusItem = new QTableWidgetItem("Ожидание ответа...");
    statusItem->setForeground(QBrush(QColor(200, 140, 0))); // Желто-оранжевый
    m_resultsTable->setItem(row, 4, statusItem);
    m_resultsTable->scrollToBottom();

    // Передаем запрос в фоновый поток
    m_worker->sendRequest(val);
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dialog(m_config, this);
    if (dialog.exec() == QDialog::Accepted) {
        LOG_INFO("Настройки сохранены пользователем, запускаем горячее переподключение...");
        m_worker->updateConfig(m_config);
    }
}

void MainWindow::onConnected()
{
    m_sendButton->setEnabled(true);
}

void MainWindow::onDisconnected()
{
    m_sendButton->setEnabled(false);
}

void MainWindow::onConnectionError(const QString& errorMessage)
{
    LOG_ERROR(QString("Ошибка связи клиента: %1").arg(errorMessage));
}

void MainWindow::onStatusChanged(const QString& statusText, bool isError)
{
    m_statusLabel->setText("Статус: " + statusText);
    if (isError) {
        m_statusLabel->setStyleSheet("color: #d9534f; font-weight: bold; font-size: 12px;");
    } else {
        m_statusLabel->setStyleSheet("color: #5cb85c; font-weight: bold; font-size: 12px;");
    }
}

void MainWindow::onRequestSent(int reqNumber)
{
    LOG_INFO(QString("GUI подтвердил отправку req=%1").arg(reqNumber));
}

void MainWindow::onResponseReceived(const QString& clientId, int reqNumber, int resNumber)
{
    // Ищем строку с этим запросом в статусе ожидания
    bool found = false;
    for (int r = 0; r < m_resultsTable->rowCount(); ++r) {
        auto* statusItem = m_resultsTable->item(r, 4);
        auto* reqItem = m_resultsTable->item(r, 2);
        if (statusItem && reqItem &&
            statusItem->text() == "Ожидание ответа..." &&
            reqItem->text().toInt() == reqNumber) {
            
            m_resultsTable->setItem(r, 3, new QTableWidgetItem(QString::number(resNumber)));
            statusItem->setText("Успешно");
            statusItem->setForeground(QBrush(QColor(40, 167, 69))); // Зеленый
            found = true;
            break;
        }
    }

    if (!found) {
        // Если не нашли подходящую строку ожидания, добавляем новую
        int row = m_resultsTable->rowCount();
        m_resultsTable->insertRow(row);
        m_resultsTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        m_resultsTable->setItem(row, 1, new QTableWidgetItem(clientId));
        m_resultsTable->setItem(row, 2, new QTableWidgetItem(QString::number(reqNumber)));
        m_resultsTable->setItem(row, 3, new QTableWidgetItem(QString::number(resNumber)));
        auto* statusItem = new QTableWidgetItem("Успешно");
        statusItem->setForeground(QBrush(QColor(40, 167, 69)));
        m_resultsTable->setItem(row, 4, statusItem);
    }

    m_resultsTable->scrollToBottom();
}
