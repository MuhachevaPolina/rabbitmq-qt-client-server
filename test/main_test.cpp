#include <gtest/gtest.h>
#include "Messages.pb.h"
#include "Config.h"
#include "Logger.h"
#include "Worker.h"
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>
#include <limits>

// ============================================================================
// 1. Тесты Protobuf: Сериализация, десериализация и валидация обязательных полей
// ============================================================================

TEST(ProtobufTest, RequestSerialization) {
    TestTask::Messages::Request req;
    req.set_id("client-uuid-12345");
    req.set_req(42);

    std::string serialized;
    ASSERT_TRUE(req.SerializeToString(&serialized));
    ASSERT_FALSE(serialized.empty());

    TestTask::Messages::Request parsedReq;
    ASSERT_TRUE(parsedReq.ParseFromString(serialized));
    EXPECT_EQ(parsedReq.id(), "client-uuid-12345");
    EXPECT_EQ(parsedReq.req(), 42);
}

TEST(ProtobufTest, ResponseSerialization) {
    TestTask::Messages::Response resp;
    resp.set_id("client-uuid-12345");
    resp.set_res(84); // 42 * 2

    std::string serialized;
    ASSERT_TRUE(resp.SerializeToString(&serialized));
    ASSERT_FALSE(serialized.empty());

    TestTask::Messages::Response parsedResp;
    ASSERT_TRUE(parsedResp.ParseFromString(serialized));
    EXPECT_EQ(parsedResp.id(), "client-uuid-12345");
    EXPECT_EQ(parsedResp.res(), 84);
}

TEST(ProtobufTest, MissingRequiredFields) {
    // В proto2 при отсутствии required полей IsInitialized() возвращает false
    TestTask::Messages::Request reqWithoutNumber;
    reqWithoutNumber.set_id("client-no-req");
    EXPECT_FALSE(reqWithoutNumber.IsInitialized());

    TestTask::Messages::Response respWithoutNumber;
    respWithoutNumber.set_id("client-no-res");
    EXPECT_FALSE(respWithoutNumber.IsInitialized());
}

// ============================================================================
// 2. Тесты Config: значения по умолчанию, сохранение, загрузка и обработка ошибок
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

// ============================================================================
// 3. Тесты Logger: конвертация уровней и фильтрация сообщений
// ============================================================================

TEST(LoggerTest, LevelConversions) {
    EXPECT_EQ(Logger::levelToString(LogLevel::Debug), "DEBUG");
    EXPECT_EQ(Logger::levelToString(LogLevel::Info), "INFO");
    EXPECT_EQ(Logger::levelToString(LogLevel::Warning), "WARN");
    EXPECT_EQ(Logger::levelToString(LogLevel::Error), "ERROR");

    EXPECT_EQ(Logger::stringToLevel("DEBUG"), LogLevel::Debug);
    EXPECT_EQ(Logger::stringToLevel("info"), LogLevel::Info);
    EXPECT_EQ(Logger::stringToLevel("WARN"), LogLevel::Warning);
    EXPECT_EQ(Logger::stringToLevel("WARNING"), LogLevel::Warning);
    EXPECT_EQ(Logger::stringToLevel("ERROR"), LogLevel::Error);
    EXPECT_EQ(Logger::stringToLevel("UNKNOWN_VALUE"), LogLevel::Info);
}

TEST(LoggerTest, FileOutputAndLevelFiltering) {
    QTemporaryFile tempFile;
    ASSERT_TRUE(tempFile.open());
    const QString logPath = tempFile.fileName();
    tempFile.close();

    // Инициализируем логгер с порогом WARNING
    Logger::instance().init(logPath, LogLevel::Warning);

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

// ============================================================================
// 4. Тесты Worker: вычисления, граничные значения, переполнение и устойчивость
// ============================================================================

TEST(WorkerTest, CalculateDoubledNormalAndBoundaries) {
    int32_t res = 0;

    // Ноль
    EXPECT_TRUE(Worker::calculateDoubled(0, res));
    EXPECT_EQ(res, 0);

    // Положительные числа
    EXPECT_TRUE(Worker::calculateDoubled(21, res));
    EXPECT_EQ(res, 42);

    // Отрицательные числа
    EXPECT_TRUE(Worker::calculateDoubled(-50, res));
    EXPECT_EQ(res, -100);

    // Максимально допустимое положительное число без переполнения: (INT32_MAX / 2) = 1073741823
    EXPECT_TRUE(Worker::calculateDoubled(1073741823, res));
    EXPECT_EQ(res, 2147483646);

    // Минимально допустимое отрицательное число без андерфлоу: (INT32_MIN / 2) = -1073741824
    EXPECT_TRUE(Worker::calculateDoubled(-1073741824, res));
    EXPECT_EQ(res, -2147483648);
}

TEST(WorkerTest, CalculateDoubledOverflowUnderflow) {
    int32_t res = 0;

    // Переполнение: INT32_MAX * 2 превышает предел 32 бит -> безопасное ограничение до INT32_MAX
    EXPECT_FALSE(Worker::calculateDoubled(std::numeric_limits<int32_t>::max(), res));
    EXPECT_EQ(res, std::numeric_limits<int32_t>::max());

    // Андерфлоу: INT32_MIN * 2 -> безопасное ограничение до INT32_MIN
    EXPECT_FALSE(Worker::calculateDoubled(std::numeric_limits<int32_t>::min(), res));
    EXPECT_EQ(res, std::numeric_limits<int32_t>::min());
}

TEST(WorkerTest, ProcessRequestSuccess) {
    TestTask::Messages::Request req;
    req.set_id("client-test-42");
    req.set_req(55);

    std::string serialized;
    ASSERT_TRUE(req.SerializeToString(&serialized));

    std::string clientId;
    int32_t outReq = 0;
    int32_t outRes = 0;
    std::string responseBytes;
    QString error;

    bool success = Worker::processRequest(serialized.data(),
                                         serialized.size(),
                                         clientId,
                                         outReq,
                                         outRes,
                                         responseBytes,
                                         error);

    ASSERT_TRUE(success);
    EXPECT_EQ(clientId, "client-test-42");
    EXPECT_EQ(outReq, 55);
    EXPECT_EQ(outRes, 110);
    EXPECT_TRUE(error.isEmpty());

    TestTask::Messages::Response resp;
    ASSERT_TRUE(resp.ParseFromString(responseBytes));
    EXPECT_EQ(resp.id(), "client-test-42");
    EXPECT_EQ(resp.res(), 110);
}

TEST(WorkerTest, ProcessRequestInvalidData) {
    std::string clientId;
    int32_t outReq = 0;
    int32_t outRes = 0;
    std::string responseBytes;
    QString error;

    // Пустой буфер
    EXPECT_FALSE(Worker::processRequest(nullptr, 0, clientId, outReq, outRes, responseBytes, error));
    EXPECT_FALSE(error.isEmpty());

    // Мусорные / некорректные бинарные данные
    const char garbage[] = "\xFF\xFE\x00\x01\x02\x03\x04\x05";
    EXPECT_FALSE(Worker::processRequest(garbage, sizeof(garbage), clientId, outReq, outRes, responseBytes, error));
    EXPECT_FALSE(error.isEmpty());
}
