#include "Server.h"

Server::Server() {}

void Server::runThread(amqp_connection_state_t conn)
{
  Worker* worker = new Worker(conn);
  m_pool.start(worker);
}
