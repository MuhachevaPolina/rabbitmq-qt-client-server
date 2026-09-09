#pragma once

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <thread>
#include <memory>

class Worker
{
public:
  Worker();
  bool doAccept(amqp_connection_state_t conn);
  bool onAccept();
private:
  // std::shared_ptr<std::thread> m_workerThread;
};