#pragma once

#include <QMainWindow>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include "Config.h"
#include "ClientWorker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSendClicked();
    void onSettingsClicked();
    void onConnected();
    void onDisconnected();
    void onConnectionError(const QString& errorMessage);
    void onStatusChanged(const QString& statusText, bool isError);
    void onRequestSent(int reqNumber);
    void onResponseReceived(const QString& clientId, int reqNumber, int resNumber);

private:
    void setupUi();
    void loadConfig();

    QSpinBox* m_numberSpinBox = nullptr;
    QPushButton* m_sendButton = nullptr;
    QTableWidget* m_resultsTable = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_settingsButton = nullptr;

    Config m_config;
    ClientWorker* m_worker = nullptr;
};
