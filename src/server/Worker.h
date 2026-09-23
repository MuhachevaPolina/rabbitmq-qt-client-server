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
  bool onAccept(amqp_connection_state_t conn, amqp_bytes_t queue_name,
                        int message_count);

  void run() override;
private:
  amqp_connection_state_t m_conn;
};