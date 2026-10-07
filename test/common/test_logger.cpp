#include <gtest/gtest.h>
#include "Logger.h"
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>

// ============================================================================
// Тесты Logger: конвертация уровней, фильтрация и запись в файл
// ============================================================================

TEST(LoggerTest, LevelConversions) {
    EXPECT_EQ(Logger::levelToString(LogLevel::Debug), "DEBUG");
    EXPECT_EQ(Logger::levelToString(LogLevel::Info), "INFO");
    EXPECT_EQ(Logger::levelToString(LogLevel::Warning), "WARN");
    EXPECT_EQ(Logger::levelToString(LogLevel::Error), "ERROR");

    EXPECT_EQ(Logger::stringToLevel("DEBUG"), LogLevel::Debug);
    EXPECT_EQ(Logger::stringToLevel("debug"), LogLevel::Debug);
    EXPECT_EQ(Logger::stringToLevel("INFO"), LogLevel::Info);
    EXPECT_EQ(Logger::stringToLevel("info"), LogLevel::Info);
    EXPECT_EQ(Logger::stringToLevel("WARN"), LogLevel::Warning);
    EXPECT_EQ(Logger::stringToLevel("WARNING"), LogLevel::Warning);
    EXPECT_EQ(Logger::stringToLevel("ERROR"), LogLevel::Error);
    EXPECT_EQ(Logger::stringToLevel("error"), LogLevel::Error);
    EXPECT_EQ(Logger::stringToLevel("UNKNOWN_VALUE"), LogLevel::Info);
}

TEST(LoggerTest, FileOutputAndLevelFiltering) {
    QTemporaryFile tempFile;
    ASSERT_TRUE(tempFile.open());
    const QString logPath = tempFile.fileName();
    tempFile.close();

    // Инициализируем логгер с порогом WARNING
    Logger::instance().init(logPath, LogLevel::Warning);
    EXPECT_EQ(Logger::instance().logLevel(), LogLevel::Warning);
    EXPECT_EQ(Logger::instance().logPath(), logPath);

    // Записываем сообщения разных уровней
    Logger::instance().log(LogLevel::Debug, "Это отладочное сообщение (должно быть проигнорировано)");
    Logger::instance().log(LogLevel::Info, "Это информационное сообщение (должно быть проигнорировано)");
    Logger::instance().log(LogLevel::Warning, "Это тестовое предупреждение");
    Logger::instance().log(LogLevel::Error, "Это тестовая ошибка");

    // Читаем записанный лог-файл
    QFile file(logPath);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream in(&file);
    const QString content = in.readAll();
    file.close();

    // DEBUG и INFO не должны присутствовать
    EXPECT_FALSE(content.contains("отладочное сообщение"));
    EXPECT_FALSE(content.contains("информационное сообщение"));

    // WARN и ERROR должны быть записаны
    EXPECT_TRUE(content.contains("Это тестовое предупреждение"));
    EXPECT_TRUE(content.contains("Это тестовая ошибка"));
}

TEST(LoggerTest, MacrosLogging) {
    QTemporaryFile tempFile;
    ASSERT_TRUE(tempFile.open());
    const QString logPath = tempFile.fileName();
    tempFile.close();

    Logger::instance().init(logPath, LogLevel::Debug);

    LOG_DEBUG("Тест макроса DEBUG");
    LOG_INFO("Тест макроса INFO");
    LOG_WARN("Тест макроса WARN");
    LOG_ERROR("Тест макроса ERROR");

    QFile file(logPath);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream in(&file);
    const QString content = in.readAll();
    file.close();

    EXPECT_TRUE(content.contains("[DEBUG] Тест макроса DEBUG"));
    EXPECT_TRUE(content.contains("[INFO] Тест макроса INFO"));
    EXPECT_TRUE(content.contains("[WARN] Тест макроса WARN"));
    EXPECT_TRUE(content.contains("[ERROR] Тест макроса ERROR"));
}
