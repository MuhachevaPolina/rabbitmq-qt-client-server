#pragma once

#include <string>
#include <cstdint>
#include <amqp.h>
#include <QString>

class Worker {
public:
  Worker() = default;

  // Вычисление удвоенного числа с проверкой на переполнение 32-битного int
  static bool calculateDoubled(int32_t req, int32_t& res);

  // Разбор Protobuf Request, удвоение и сборка Protobuf Response
  static bool processRequest(const void* data, size_t size,
                             std::string& outClientId,
                             int32_t& outReq,
                             int32_t& outRes,
                             std::string& outResponseBytes,
                             QString& outError);

  // Обработка сообщения из очереди брокера
  bool handleMessage(amqp_connection_state_t conn,
                     amqp_channel_t channel,
                     const amqp_envelope_t& envelope,
                     const QString& exchange);
};