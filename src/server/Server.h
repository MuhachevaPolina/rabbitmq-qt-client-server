#pragma once

#include "Worker.h"

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <QThreadPool>

#include <boost/asio.hpp>

class Server
{
public:
  Server();
  void start();
  void addConnection();
  void runThread(amqp_connection_state_t conn);

private:
  QThreadPool m_pool;
  boost::asio::io_context m_context;
};






// 0. server and client exchange with rabbitmq without threads -- DONE
// 1. 1 worker in threadpool on server -- DONE
// 2. multiple workers acync with boost