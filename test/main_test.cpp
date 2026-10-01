#include <gtest/gtest.h>
#include "Messages.pb.h"
#include "Config.h"
#include "Logger.h"
#include "Worker.h"
#include <QTemporaryFile>
#include <limits>

// Test Protobuf Message serialization and deserialization
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

// Test Config defaults and saving/loading
TEST(ConfigTest, DefaultsAndSaveLoad) {
    Config cfg;
    EXPECT_EQ(cfg.broker().host, "localhost");
    EXPECT_EQ(cfg.broker().port, 5672);
    EXPECT_EQ(cfg.broker().vhost, "rabbitmq_qt");
    EXPECT_EQ(cfg.broker().username, "rabbitmq_qt_user");
    EXPECT_EQ(cfg.broker().password, "rabbitmqqt");
    EXPECT_EQ(cfg.logging().logLevel, LogLevel::Info);

    // Test saving to temp file and loading
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

// Test Logger level conversions
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
}

// Step 2 & 4: Test Worker business calculation logic and overflow protection
TEST(WorkerTest, CalculateDoubled) {
    int32_t res = 0;

    // Normal positive values
    EXPECT_TRUE(Worker::calculateDoubled(0, res));
    EXPECT_EQ(res, 0);

    EXPECT_TRUE(Worker::calculateDoubled(21, res));
    EXPECT_EQ(res, 42);

    // Normal negative values
    EXPECT_TRUE(Worker::calculateDoubled(-50, res));
    EXPECT_EQ(res, -100);

    // Boundary value within valid range (max int32 / 2)
    EXPECT_TRUE(Worker::calculateDoubled(1073741823, res));
    EXPECT_EQ(res, 2147483646);

    EXPECT_TRUE(Worker::calculateDoubled(-1073741824, res));
    EXPECT_EQ(res, -2147483648);

    // Overflow check
    EXPECT_FALSE(Worker::calculateDoubled(std::numeric_limits<int32_t>::max(), res));
    EXPECT_EQ(res, std::numeric_limits<int32_t>::max());

    // Underflow check
    EXPECT_FALSE(Worker::calculateDoubled(std::numeric_limits<int32_t>::min(), res));
    EXPECT_EQ(res, std::numeric_limits<int32_t>::min());
}

// Step 2 & 4: Test Worker processRequest with Protobuf
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

// Step 2 & 4: Test Worker processRequest with invalid or corrupt data
TEST(WorkerTest, ProcessRequestInvalidData) {
    std::string clientId;
    int32_t outReq = 0;
    int32_t outRes = 0;
    std::string responseBytes;
    QString error;

    // Test with null / empty buffer
    EXPECT_FALSE(Worker::processRequest(nullptr, 0, clientId, outReq, outRes, responseBytes, error));
    EXPECT_FALSE(error.isEmpty());

    // Test with invalid garbage data
    const char garbage[] = "this is definitely not a protobuf payload";
    EXPECT_FALSE(Worker::processRequest(garbage, sizeof(garbage), clientId, outReq, outRes, responseBytes, error));
    EXPECT_FALSE(error.isEmpty());
}
