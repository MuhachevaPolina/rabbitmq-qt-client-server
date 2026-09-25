#pragma once

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <QRunnable>

#include <thread>
#include <memory>

class Worker: public QRunnable
{
public:
  Worker(amqp_connection_state_t conn, amqp_bytes_t queue_name, char const* requestbindingkey);
  bool doAccept();
  bool onAccept(amqp_connection_state_t conn, amqp_bytes_t queue_name);

  void run() override;
private:
  amqp_connection_state_t m_conn;
  amqp_bytes_t m_queue_name;
  char const* m_requestbindingkey;
  char const* m_reply_key;
};