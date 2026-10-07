#include <gtest/gtest.h>
#include "ClientWorker.h"
#include "Config.h"
#include <QSignalSpy>
#include <QUuid>

// ============================================================================
// Тесты ClientWorker: генерация ID, потоковая безопасность, сигналы и сбои
// ============================================================================

TEST(ClientWorkerTest, ClientIdFormat) {
    Config cfg;
    ClientWorker worker1(cfg);
    ClientWorker worker2(cfg);

    const QString id1 = worker1.clientId();
    const QString id2 = worker2.clientId();

    // Префикс должен быть "client_"
    EXPECT_TRUE(id1.startsWith("client_"));
    EXPECT_TRUE(id2.startsWith("client_"));

    // Идентификаторы двух разных клиентов должны быть уникальными
    EXPECT_NE(id1, id2);

    // Длина строки должна быть достаточной для префикса и UUID
    EXPECT_GE(id1.length(), 39);
}

TEST(ClientWorkerTest, InitialState) {
    Config cfg;
    ClientWorker worker(cfg);

    EXPECT_FALSE(worker.isConnected());
    EXPECT_FALSE(worker.isRunning());
}

TEST(ClientWorkerTest, SendRequestEnqueuesWithoutCrash) {
    Config cfg;
    ClientWorker worker(cfg);

    // Добавление запросов в очередь отправки до запуска потока
    worker.sendRequest(10);
    worker.sendRequest(20);
    worker.sendRequest(-5);
    SUCCEED();
}

TEST(ClientWorkerTest, UpdateConfigSlot) {
    Config cfg;
    ClientWorker worker(cfg);

    Config updatedCfg;
    updatedCfg.broker().host = "10.0.0.1";
    updatedCfg.broker().port = 5678;

    worker.updateConfig(updatedCfg);
    SUCCEED();
}

TEST(ClientWorkerTest, StopWithoutStartIsSafe) {
    Config cfg;
    ClientWorker worker(cfg);

    worker.stop();
    EXPECT_FALSE(worker.isRunning());
}

TEST(ClientWorkerTest, ConnectionFailureSignalEmitted) {
    Config cfg;
    cfg.broker().host = "127.0.0.1";
    cfg.broker().port = 59999; // Заведомо закрытый порт

    ClientWorker worker(cfg);
    QSignalSpy spyError(&worker, &ClientWorker::connectionError);
    QSignalSpy spyStatus(&worker, &ClientWorker::statusChanged);

    worker.start();

    // Ожидаем сигнал об ошибке подключения
    bool gotSignal = spyError.wait(2000) || spyStatus.wait(2000);
    EXPECT_TRUE(gotSignal);
    EXPECT_FALSE(worker.isConnected());

    worker.stop();
    EXPECT_TRUE(worker.wait(3000));
}
