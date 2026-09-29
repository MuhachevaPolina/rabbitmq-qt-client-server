#pragma once

#include <QString>
#include "Logger.h"

struct BrokerConfig {
    QString host = "localhost";
    int port = 5672;
    QString vhost = "rabbitmq_qt";
    QString username = "rabbitmq_qt_user";
    QString password = "rabbitmqqt";
    QString exchange = "amq.direct";
    QString requestQueue = "serverQueue";
};

struct LoggingConfig {
    QString logPath = "app.log";
    LogLevel logLevel = LogLevel::Info;
};

class Config {
public:
    Config();
    explicit Config(const QString& filePath);

    bool load(const QString& filePath);
    bool save(const QString& filePath = QString()) const;

    const BrokerConfig& broker() const { return m_broker; }
    BrokerConfig& broker() { return m_broker; }

    const LoggingConfig& logging() const { return m_logging; }
    LoggingConfig& logging() { return m_logging; }

    QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
    BrokerConfig m_broker;
    LoggingConfig m_logging;
};
