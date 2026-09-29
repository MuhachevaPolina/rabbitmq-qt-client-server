#include <gtest/gtest.h>
#include "Messages.pb.h"
#include "Config.h"
#include "Logger.h"
#include <QTemporaryFile>

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
