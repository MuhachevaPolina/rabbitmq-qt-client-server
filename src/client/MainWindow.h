#pragma once

#include <QMainWindow>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include "Config.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onSendClicked();
    void onSettingsClicked();

private:
    void setupUi();

    QSpinBox* m_numberSpinBox = nullptr;
    QPushButton* m_sendButton = nullptr;
    QTableWidget* m_resultsTable = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_settingsButton = nullptr;

    Config m_config;
};
