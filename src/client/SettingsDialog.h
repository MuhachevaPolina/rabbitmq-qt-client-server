#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include "Config.h"

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(Config& config, QWidget *parent = nullptr);
    ~SettingsDialog() override = default;

private slots:
    void onSaveClicked();

private:
    void setupUi();
    void loadToUi();
    void saveFromUi();

    Config& m_config;

    QLineEdit* m_hostEdit = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QLineEdit* m_vhostEdit = nullptr;
    QLineEdit* m_userEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;

    QLineEdit* m_logPathEdit = nullptr;
    QComboBox* m_logLevelCombo = nullptr;

    QDialogButtonBox* m_buttonBox = nullptr;
};
