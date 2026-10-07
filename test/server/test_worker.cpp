#include <gtest/gtest.h>
#include "Worker.h"
#include "Messages.pb.h"
#include <limits>

// ============================================================================
// Тесты Worker: вычисления, граничные значения, переполнение и устойчивость
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

TEST(WorkerTest, ProcessRequestWithOverflow) {
    TestTask::Messages::Request req;
    req.set_id("overflow-client");
    req.set_req(std::numeric_limits<int32_t>::max());

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
    EXPECT_EQ(outReq, std::numeric_limits<int32_t>::max());
    EXPECT_EQ(outRes, std::numeric_limits<int32_t>::max());

    TestTask::Messages::Response resp;
    ASSERT_TRUE(resp.ParseFromString(responseBytes));
    EXPECT_EQ(resp.id(), "overflow-client");
    EXPECT_EQ(resp.res(), std::numeric_limits<int32_t>::max());
}
