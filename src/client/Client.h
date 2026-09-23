#pragma once

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <string>


class Client
{
public:
  Client();
  void connect(int argc, char const* const* argv);
  void send_batch(amqp_connection_state_t conn, amqp_bytes_t queue_name,
                      int message_count);
  bool getAnswer();
private:

};