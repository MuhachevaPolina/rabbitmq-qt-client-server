#pragma once

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <QRunnable>

#include <thread>
#include <memory>

class Worker: public QRunnable
{
public:
  Worker(amqp_connection_state_t conn);
  bool doAccept();

  void run() override;
  // bool onAccept();
private:
  amqp_connection_state_t m_conn;
};