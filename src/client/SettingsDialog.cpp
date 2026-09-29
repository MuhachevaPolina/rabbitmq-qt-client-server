#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

SettingsDialog::SettingsDialog(Config& config, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
{
    setupUi();
    loadToUi();

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onSaveClicked);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::setupUi()
{
    setWindowTitle("Настройки подключения и логирования");
    resize(420, 360);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Group 1: RabbitMQ Broker
    QGroupBox* brokerGroup = new QGroupBox("RabbitMQ Broker", this);
    QFormLayout* brokerLayout = new QFormLayout(brokerGroup);

    m_hostEdit = new QLineEdit(this);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(5672);

    m_vhostEdit = new QLineEdit(this);
    m_userEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    brokerLayout->addRow("Хост:", m_hostEdit);
    brokerLayout->addRow("Порт:", m_portSpin);
    brokerLayout->addRow("VHost:", m_vhostEdit);
    brokerLayout->addRow("Пользователь:", m_userEdit);
    brokerLayout->addRow("Пароль:", m_passwordEdit);
    mainLayout->addWidget(brokerGroup);

    // Group 2: Logging
    QGroupBox* logGroup = new QGroupBox("Логирование", this);
    QFormLayout* logLayout = new QFormLayout(logGroup);

    m_logPathEdit = new QLineEdit(this);
    m_logLevelCombo = new QComboBox(this);
    m_logLevelCombo->addItems({"DEBUG", "INFO", "WARN", "ERROR"});

    logLayout->addRow("Путь к логу:", m_logPathEdit);
    logLayout->addRow("Уровень лога:", m_logLevelCombo);
    mainLayout->addWidget(logGroup);

    // Dialog buttons: Save / Cancel
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);
}

void SettingsDialog::loadToUi()
{
    const auto& broker = m_config.broker();
    m_hostEdit->setText(broker.host);
    m_portSpin->setValue(broker.port);
    m_vhostEdit->setText(broker.vhost);
    m_userEdit->setText(broker.username);
    m_passwordEdit->setText(broker.password);

    const auto& logging = m_config.logging();
    m_logPathEdit->setText(logging.logPath);
    m_logLevelCombo->setCurrentText(Logger::levelToString(logging.logLevel));
}

void SettingsDialog::saveFromUi()
{
    auto& broker = m_config.broker();
    broker.host = m_hostEdit->text().trimmed();
    broker.port = m_portSpin->value();
    broker.vhost = m_vhostEdit->text().trimmed();
    broker.username = m_userEdit->text().trimmed();
    broker.password = m_passwordEdit->text();

    auto& logging = m_config.logging();
    logging.logPath = m_logPathEdit->text().trimmed();
    logging.logLevel = Logger::stringToLevel(m_logLevelCombo->currentText());

    m_config.save();
}

void SettingsDialog::onSaveClicked()
{
    saveFromUi();
    accept();
}
