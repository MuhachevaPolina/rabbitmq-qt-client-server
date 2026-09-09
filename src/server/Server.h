#pragma once

#include "Worker.h"

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <vector>
#include <memory>

class Server
{
public:
  Server();
  void run(amqp_connection_state_t conn);

private:

};
 // 1. 1 worker with 1 thread and server with rabbitmq without client and serialization