#pragma once

#include <string>
#include <cstdint>
#include <amqp.h>
#include <QString>

class Worker {
public:
    Worker() = default;

    // Вычисление удвоенного числа с проверкой на переполнение 32-битного int.
    // Возвращает true, если расчет выполнен без переполнения.
    // При переполнении/андерфлоу возвращает false, а результат ограничивается INT32_MAX / INT32_MIN.
    static bool calculateDoubled(int32_t req, int32_t& res);

    // Десериализация сообщения Request Protobuf, выполнение удвоения и сериализация Response Protobuf.
    // Возвращает true при успешном разборе и формировании ответа.
    static bool processRequest(const void* data, size_t size,
                               std::string& outClientId,
                               int32_t& outReq,
                               int32_t& outRes,
                               std::string& outResponseBytes,
                               QString& outError);

    // Обработка входящего конверта AMQP:
    // 1. Десериализация через processRequest
    // 2. Отправка ответа в exchange с routing key = client_id
    // 3. Отправка подтверждения amqp_basic_ack
    bool handleMessage(amqp_connection_state_t conn,
                       amqp_channel_t channel,
                       const amqp_envelope_t& envelope,
                       const QString& exchange);
};