#include <gtest/gtest.h>
#include "SettingsDialog.h"
#include "Config.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTemporaryFile>

// ============================================================================
// Тесты SettingsDialog: отображение настроек в UI и сохранение в Config
// ============================================================================

TEST(SettingsDialogTest, LoadConfigToUi) {
    Config cfg;
    cfg.broker().host = "test-host.local";
    cfg.broker().port = 5690;
    cfg.broker().vhost = "test-vhost";
    cfg.broker().username = "test-user";
    cfg.broker().password = "secret123";
    cfg.logging().logPath = "test.log";
    cfg.logging().logLevel = LogLevel::Warning;

    SettingsDialog dlg(cfg);

    auto* hostEdit = dlg.findChild<QLineEdit*>("hostEdit");
    ASSERT_NE(hostEdit, nullptr);
    EXPECT_EQ(hostEdit->text(), "test-host.local");

    auto* portSpin = dlg.findChild<QSpinBox*>("portSpin");
    ASSERT_NE(portSpin, nullptr);
    EXPECT_EQ(portSpin->value(), 5690);

    auto* vhostEdit = dlg.findChild<QLineEdit*>("vhostEdit");
    ASSERT_NE(vhostEdit, nullptr);
    EXPECT_EQ(vhostEdit->text(), "test-vhost");

    auto* userEdit = dlg.findChild<QLineEdit*>("userEdit");
    ASSERT_NE(userEdit, nullptr);
    EXPECT_EQ(userEdit->text(), "test-user");

    auto* passEdit = dlg.findChild<QLineEdit*>("passwordEdit");
    ASSERT_NE(passEdit, nullptr);
    EXPECT_EQ(passEdit->text(), "secret123");

    auto* logPathEdit = dlg.findChild<QLineEdit*>("logPathEdit");
    ASSERT_NE(logPathEdit, nullptr);
    EXPECT_EQ(logPathEdit->text(), "test.log");

    auto* logLevelCombo = dlg.findChild<QComboBox*>("logLevelCombo");
    ASSERT_NE(logLevelCombo, nullptr);
    EXPECT_EQ(logLevelCombo->currentText(), "WARN");
}

TEST(SettingsDialogTest, SaveUiToConfig) {
    QTemporaryFile tempFile;
    ASSERT_TRUE(tempFile.open());
    const QString tempPath = tempFile.fileName();
    tempFile.close();

    Config cfg;
    ASSERT_TRUE(cfg.save(tempPath));
    ASSERT_TRUE(cfg.load(tempPath));

    SettingsDialog dlg(cfg);

    auto* hostEdit = dlg.findChild<QLineEdit*>("hostEdit");
    auto* portSpin = dlg.findChild<QSpinBox*>("portSpin");
    auto* vhostEdit = dlg.findChild<QLineEdit*>("vhostEdit");
    auto* logLevelCombo = dlg.findChild<QComboBox*>("logLevelCombo");
    auto* buttonBox = dlg.findChild<QDialogButtonBox*>("buttonBox");

    ASSERT_NE(hostEdit, nullptr);
    ASSERT_NE(portSpin, nullptr);
    ASSERT_NE(vhostEdit, nullptr);
    ASSERT_NE(logLevelCombo, nullptr);
    ASSERT_NE(buttonBox, nullptr);

    hostEdit->setText("updated-broker");
    portSpin->setValue(5699);
    vhostEdit->setText("vhost_updated");
    logLevelCombo->setCurrentText("ERROR");

    // Имитируем клик по кнопке "Save"
    QPushButton* saveBtn = buttonBox->button(QDialogButtonBox::Save);
    ASSERT_NE(saveBtn, nullptr);
    saveBtn->click();

    // Проверяем обновление объекта Config
    EXPECT_EQ(cfg.broker().host, "updated-broker");
    EXPECT_EQ(cfg.broker().port, 5699);
    EXPECT_EQ(cfg.broker().vhost, "vhost_updated");
    EXPECT_EQ(cfg.logging().logLevel, LogLevel::Error);

    // Проверяем сохранение в файл
    Config reloadedCfg;
    ASSERT_TRUE(reloadedCfg.load(tempPath));
    EXPECT_EQ(reloadedCfg.broker().host, "updated-broker");
    EXPECT_EQ(reloadedCfg.broker().port, 5699);
    EXPECT_EQ(reloadedCfg.broker().vhost, "vhost_updated");
    EXPECT_EQ(reloadedCfg.logging().logLevel, LogLevel::Error);
}

TEST(SettingsDialogTest, PortSpinBoxConstraints) {
    Config cfg;
    SettingsDialog dlg(cfg);

    auto* portSpin = dlg.findChild<QSpinBox*>("portSpin");
    ASSERT_NE(portSpin, nullptr);
    EXPECT_EQ(portSpin->minimum(), 1);
    EXPECT_EQ(portSpin->maximum(), 65535);
}
