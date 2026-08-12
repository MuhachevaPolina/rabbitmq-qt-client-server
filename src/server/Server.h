#pragma once

#include <amqp.h>
#include <amqp_tcp_socket.h>

class Server
{
public:
  Server();
  void run(amqp_connection_state_t conn);

private:
  bool processClientMessage();
  bool getMessageFromRabbitmq();
  bool responseToClient(int clientNumber);

  int m_curClientNum;
};