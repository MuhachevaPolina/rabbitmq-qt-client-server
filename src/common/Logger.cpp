#include "Logger.h"

#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QMutexLocker>
#include <iostream>

Logger::Logger()
    : m_level(LogLevel::Info)
{
}

Logger::~Logger()
{
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_stream->flush();
        m_file.close();
    }
}

Logger& Logger::instance()
{
    static Logger s_instance;
    return s_instance;
}

void Logger::init(const QString& filePath, LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_level = level;
    m_filePath = filePath;

    if (m_file.isOpen()) {
        m_stream->flush();
        m_file.close();
    }

    if (!filePath.isEmpty()) {
        QFileInfo info(filePath);
        QDir dir = info.dir();
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        m_file.setFileName(filePath);
        if (m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_stream = std::make_unique<QTextStream>(&m_file);
        } else {
            std::cerr << "Failed to open log file: " << filePath.toStdString() << std::endl;
        }
    }
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_level = level;
}

void Logger::setLogPath(const QString& filePath)
{
    init(filePath, m_level);
}

LogLevel Logger::logLevel() const
{
    return m_level;
}

QString Logger::logPath() const
{
    return m_filePath;
}

void Logger::log(LogLevel level, const QString& message)
{
    if (level < m_level) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    const QString levelStr = levelToString(level);
    const QString line = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    // Also output to console (stdout / stderr)
    if (level == LogLevel::Error) {
        std::cerr << line.toStdString() << std::endl;
    } else {
        std::cout << line.toStdString() << std::endl;
    }

    if (m_stream) {
        *m_stream << line << "\n";
        m_stream->flush();
    }
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
    }
    return "UNKNOWN";
}

LogLevel Logger::stringToLevel(const QString& levelStr)
{
    const QString upper = levelStr.trimmed().toUpper();
    if (upper == "DEBUG")   return LogLevel::Debug;
    if (upper == "INFO")    return LogLevel::Info;
    if (upper == "WARN" || upper == "WARNING") return LogLevel::Warning;
    if (upper == "ERROR")   return LogLevel::Error;
    return LogLevel::Info;
}
