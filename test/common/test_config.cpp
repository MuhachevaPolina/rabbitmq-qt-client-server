#include <gtest/gtest.h>
#include "Config.h"
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>

// ============================================================================
// Тесты Config: значения по умолчанию, сохранение, загрузка и обработка ошибок
// ============================================================================

TEST(ConfigTest, DefaultsAndSaveLoad) {
    Config cfg;
    EXPECT_EQ(cfg.broker().host, "localhost");
    EXPECT_EQ(cfg.broker().port, 5672);
    EXPECT_EQ(cfg.broker().vhost, "rabbitmq_qt");
    EXPECT_EQ(cfg.broker().username, "rabbitmq_qt_user");
    EXPECT_EQ(cfg.broker().password, "rabbitmqqt");
    EXPECT_EQ(cfg.broker().exchange, "amq.direct");
    EXPECT_EQ(cfg.broker().requestQueue, "serverQueue");
    EXPECT_EQ(cfg.logging().logLevel, LogLevel::Info);

    QTemporaryFile tempFile;
    ASSERT_TRUE(tempFile.open());
    const QString tempPath = tempFile.fileName();
    tempFile.close();

    cfg.broker().port = 5673;
    cfg.broker().username = "custom_user";
    cfg.logging().logLevel = LogLevel::Debug;
    ASSERT_TRUE(cfg.save(tempPath));

    Config loadedCfg;
    ASSERT_TRUE(loadedCfg.load(tempPath));
    EXPECT_EQ(loadedCfg.broker().port, 5673);
    EXPECT_EQ(loadedCfg.broker().username, "custom_user");
    EXPECT_EQ(loadedCfg.logging().logLevel, LogLevel::Debug);
}

TEST(ConfigTest, NonExistentFileHandling) {
    Config cfg;
    // Загрузка несуществующего пути должна вернуть false, сохранив значения по умолчанию
    EXPECT_FALSE(cfg.load("/non/existent/path/to/server.ini"));
    EXPECT_EQ(cfg.broker().host, "localhost");
    EXPECT_EQ(cfg.broker().port, 5672);
}

TEST(ConfigTest, SaveWithoutPathReturnsFalse) {
    Config cfg;
    // Попытка сохранить конфиг без указания пути
    EXPECT_FALSE(cfg.save(""));
}

TEST(ConfigTest, ConstructorWithFilePath) {
    QTemporaryFile tempFile;
    ASSERT_TRUE(tempFile.open());
    const QString tempPath = tempFile.fileName();
    tempFile.close();

    Config initialCfg;
    initialCfg.broker().host = "remote-broker";
    initialCfg.broker().port = 5675;
    ASSERT_TRUE(initialCfg.save(tempPath));

    Config loadedCfg(tempPath);
    EXPECT_EQ(loadedCfg.broker().host, "remote-broker");
    EXPECT_EQ(loadedCfg.broker().port, 5675);
    EXPECT_EQ(loadedCfg.filePath(), tempPath);
}

TEST(ConfigTest, PartialIniFileRetainsDefaults) {
    QTemporaryFile tempFile;
    ASSERT_TRUE(tempFile.open());
    const QString tempPath = tempFile.fileName();

    QTextStream out(&tempFile);
    out << "[Broker]\n";
    out << "port=9999\n";
    tempFile.close();

    Config cfg;
    ASSERT_TRUE(cfg.load(tempPath));
    EXPECT_EQ(cfg.broker().port, 9999);
    // Остальные поля должны сохранить свои дефолтные значения
    EXPECT_EQ(cfg.broker().host, "localhost");
    EXPECT_EQ(cfg.broker().vhost, "rabbitmq_qt");
    EXPECT_EQ(cfg.logging().logLevel, LogLevel::Info);
}
