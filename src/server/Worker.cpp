#include "Worker.h"
#include "Messages.pb.h"
#include "Logger.h"

#include <limits>
#include <cstring>

bool Worker::calculateDoubled(int32_t req, int32_t& res) {
    int64_t doubled = static_cast<int64_t>(req) * 2;
    if (doubled > std::numeric_limits<int32_t>::max()) {
        res = std::numeric_limits<int32_t>::max();
        return false;
    }
    if (doubled < std::numeric_limits<int32_t>::min()) {
        res = std::numeric_limits<int32_t>::min();
        return false;
    }
    res = static_cast<int32_t>(doubled);
    return true;
}

bool Worker::processRequest(const void* data, size_t size,
                            std::string& outClientId,
                            int32_t& outReq,
                            int32_t& outRes,
                            std::string& outResponseBytes,
                            QString& outError) {
    if (!data || size == 0) {
        outError = "Пустое тело сообщения запроса";
        return false;
    }

    TestTask::Messages::Request request;
    if (!request.ParseFromArray(data, static_cast<int>(size))) {
        outError = "Ошибка десериализации Protobuf Request";
        return false;
    }

    outClientId = request.id();
    outReq = request.req();

    bool noOverflow = calculateDoubled(outReq, outRes);
    if (!noOverflow) {
        LOG_WARN(QString("Обнаружено переполнение для req=%1, результат ограничен: %2").arg(outReq).arg(outRes));
    }

    TestTask::Messages::Response response;
    response.set_id(outClientId);
    response.set_res(outRes);

    if (!response.SerializeToString(&outResponseBytes)) {
        outError = "Ошибка сериализации Protobuf Response";
        return false;
    }

    return true;
}

bool Worker::handleMessage(amqp_connection_state_t conn,
                           amqp_channel_t channel,
                           const amqp_envelope_t& envelope,
                           const QString& exchange) {
    std::string clientId;
    int32_t req = 0;
    int32_t res = 0;
    std::string responseBytes;
    QString error;

    bool ok = processRequest(envelope.message.body.bytes,
                             envelope.message.body.len,
                             clientId,
                             req,
                             res,
                             responseBytes,
                             error);

    if (!ok) {
        LOG_ERROR(QString("Ошибка обработки запроса: %1").arg(error));
        // Подтверждаем сообщение, чтобы поврежденные данные не зацикливали очередь
        amqp_basic_ack(conn, channel, envelope.delivery_tag, 0);
        return false;
    }

    // Свойства для отправляемого сообщения (Persistent)
    amqp_basic_properties_t props;
    std::memset(&props, 0, sizeof(props));
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/x-protobuf");
    props.delivery_mode = 2; // Persistent

    amqp_bytes_t exBytes = amqp_cstring_bytes(exchange.toUtf8().constData());
    amqp_bytes_t routingKeyBytes = amqp_cstring_bytes(clientId.c_str());
    amqp_bytes_t bodyBytes;
    bodyBytes.len = responseBytes.size();
    bodyBytes.bytes = const_cast<char*>(responseBytes.data());

    int pubStatus = amqp_basic_publish(
        conn,
        channel,
        exBytes,
        routingKeyBytes,
        0, // mandatory
        0, // immediate
        &props,
        bodyBytes
    );

    if (pubStatus != AMQP_STATUS_OK) {
        LOG_ERROR(QString("Не удалось опубликовать ответ в routing key '%1': %2")
            .arg(QString::fromStdString(clientId))
            .arg(amqp_error_string2(pubStatus)));
        amqp_basic_ack(conn, channel, envelope.delivery_tag, 0);
        return false;
    }

    // Отправляем подтверждение обработки сообщения брокеру
    int ackStatus = amqp_basic_ack(conn, channel, envelope.delivery_tag, 0);
    if (ackStatus != AMQP_STATUS_OK) {
        LOG_ERROR(QString("Не удалось отправить ACK для delivery_tag %1: %2")
            .arg(envelope.delivery_tag)
            .arg(amqp_error_string2(ackStatus)));
    }

    LOG_INFO(QString("Обработан запрос id: %1, req: %2 -> doubled: %3 (отправлен в routing key: '%1')")
        .arg(QString::fromStdString(clientId))
        .arg(req)
        .arg(res));

    return true;
}
