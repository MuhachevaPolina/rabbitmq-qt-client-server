#include <gtest/gtest.h>
#include "Server.h"
#include "Config.h"

// ============================================================================
// Тесты Server: жизненный цикл, инициализация, обработка сбоев подключения
// ============================================================================

TEST(ServerTest, InitialState) {
    Config cfg;
    Server server(cfg);
    EXPECT_FALSE(server.isRunning());
}

TEST(ServerTest, InitFailsOnInvalidBrokerHost) {
    Config cfg;
    cfg.broker().host = "127.0.0.1";
    cfg.broker().port = 59999; // Заведомо закрытый / неиспользуемый порт

    Server server(cfg);
    // При недоступности брокера метод init() должен вернуть false и корректно освободить ресурсы
    EXPECT_FALSE(server.init());
    EXPECT_FALSE(server.isRunning());
}

TEST(ServerTest, RunWithoutInitReturnsImmediately) {
    Config cfg;
    Server server(cfg);
    // Запуск run() без предварительного init() должен завершиться немедленно
    server.run();
    EXPECT_FALSE(server.isRunning());
}

TEST(ServerTest, StopAndCleanupIdempotent) {
    Config cfg;
    Server server(cfg);

    // Многократный вызов stop() и cleanup() должен быть идемпотентным и безопасным
    server.stop();
    EXPECT_FALSE(server.isRunning());

    server.cleanup();
    server.cleanup();
    EXPECT_FALSE(server.isRunning());
}
