#include <gtest/gtest.h>
#include "Messages.pb.h"
#include <limits>

// ============================================================================
// Тесты Protobuf: Сериализация, десериализация и валидация сетевого протокола
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

TEST(ProtobufTest, NegativeAndBoundaryValues) {
    // Проверка сериализации отрицательных чисел и пределов int32
    TestTask::Messages::Request req;
    req.set_id("client-boundary");
    req.set_req(std::numeric_limits<int32_t>::min());

    std::string serialized;
    ASSERT_TRUE(req.SerializeToString(&serialized));

    TestTask::Messages::Request parsedReq;
    ASSERT_TRUE(parsedReq.ParseFromString(serialized));
    EXPECT_EQ(parsedReq.req(), std::numeric_limits<int32_t>::min());

    TestTask::Messages::Response resp;
    resp.set_id("client-boundary");
    resp.set_res(std::numeric_limits<int32_t>::max());

    ASSERT_TRUE(resp.SerializeToString(&serialized));
    TestTask::Messages::Response parsedResp;
    ASSERT_TRUE(parsedResp.ParseFromString(serialized));
    EXPECT_EQ(parsedResp.res(), std::numeric_limits<int32_t>::max());
}

TEST(ProtobufTest, EmptyClientIdAllowed) {
    TestTask::Messages::Request req;
    req.set_id("");
    req.set_req(100);

    EXPECT_TRUE(req.IsInitialized());
    std::string serialized;
    ASSERT_TRUE(req.SerializeToString(&serialized));

    TestTask::Messages::Request parsedReq;
    ASSERT_TRUE(parsedReq.ParseFromString(serialized));
    EXPECT_TRUE(parsedReq.id().empty());
    EXPECT_EQ(parsedReq.req(), 100);
}

TEST(ProtobufTest, CorruptedPayloadParsingFails) {
    const std::string garbage = "\xFF\xFE\xFD\xFC\x00\x01\x02";
    TestTask::Messages::Request req;
    EXPECT_FALSE(req.ParseFromString(garbage));

    TestTask::Messages::Response resp;
    EXPECT_FALSE(resp.ParseFromString(garbage));
}
