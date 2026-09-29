#include "Config.h"

#include <QSettings>
#include <QFileInfo>
#include <QDir>

Config::Config()
{
}

Config::Config(const QString& filePath)
{
    load(filePath);
}

bool Config::load(const QString& filePath)
{
    m_filePath = filePath;
    QFileInfo checkFile(filePath);
    if (!checkFile.exists() || !checkFile.isFile()) {
        // Return false if file doesn't exist, keeping default values
        return false;
    }

    QSettings settings(filePath, QSettings::IniFormat);

    settings.beginGroup("Broker");
    m_broker.host = settings.value("host", m_broker.host).toString();
    m_broker.port = settings.value("port", m_broker.port).toInt();
    m_broker.vhost = settings.value("vhost", m_broker.vhost).toString();
    m_broker.username = settings.value("username", m_broker.username).toString();
    m_broker.password = settings.value("password", m_broker.password).toString();
    m_broker.exchange = settings.value("exchange", m_broker.exchange).toString();
    m_broker.requestQueue = settings.value("request_queue", m_broker.requestQueue).toString();
    settings.endGroup();

    settings.beginGroup("Logging");
    m_logging.logPath = settings.value("log_path", m_logging.logPath).toString();
    QString levelStr = settings.value("log_level", Logger::levelToString(m_logging.logLevel)).toString();
    m_logging.logLevel = Logger::stringToLevel(levelStr);
    settings.endGroup();

    return true;
}

bool Config::save(const QString& filePath) const
{
    const QString targetPath = filePath.isEmpty() ? m_filePath : filePath;
    if (targetPath.isEmpty()) {
        return false;
    }

    QFileInfo info(targetPath);
    QDir dir = info.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QSettings settings(targetPath, QSettings::IniFormat);

    settings.beginGroup("Broker");
    settings.setValue("host", m_broker.host);
    settings.setValue("port", m_broker.port);
    settings.setValue("vhost", m_broker.vhost);
    settings.setValue("username", m_broker.username);
    settings.setValue("password", m_broker.password);
    settings.setValue("exchange", m_broker.exchange);
    settings.setValue("request_queue", m_broker.requestQueue);
    settings.endGroup();

    settings.beginGroup("Logging");
    settings.setValue("log_path", m_logging.logPath);
    settings.setValue("log_level", Logger::levelToString(m_logging.logLevel));
    settings.endGroup();

    settings.sync();
    return (settings.status() == QSettings::NoError);
}
