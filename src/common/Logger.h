#pragma once

#include <QString>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <memory>

enum class LogLevel {
    Debug = 0,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& instance();

    void init(const QString& filePath, LogLevel level = LogLevel::Info);
    void setLogLevel(LogLevel level);
    void setLogPath(const QString& filePath);
    LogLevel logLevel() const;
    QString logPath() const;

    void log(LogLevel level, const QString& message);

    static QString levelToString(LogLevel level);
    static LogLevel stringToLevel(const QString& levelStr);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QMutex m_mutex;
    QFile m_file;
    std::unique_ptr<QTextStream> m_stream;
    LogLevel m_level;
    QString m_filePath;
};

#define LOG_DEBUG(msg) Logger::instance().log(LogLevel::Debug, (msg))
#define LOG_INFO(msg)  Logger::instance().log(LogLevel::Info, (msg))
#define LOG_WARN(msg)  Logger::instance().log(LogLevel::Warning, (msg))
#define LOG_ERROR(msg) Logger::instance().log(LogLevel::Error, (msg))
