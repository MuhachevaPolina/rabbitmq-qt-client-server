#pragma once

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <string>


class Client
{
public:
  Client();
  void connect(int argc, char const* const* argv);
  void send_batch(amqp_connection_state_t conn,
                      int message_count);
  bool getAnswer(amqp_connection_state_t conn, int message_count);
private:
    amqp_bytes_t m_gotQueueName;
    amqp_bytes_t m_sentQueueName;
};