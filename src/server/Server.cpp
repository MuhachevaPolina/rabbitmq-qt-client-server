#include "Server.h"

Server::Server(amqp_bytes_t queue_name, char const* requestbindingkey)
  : m_queue_name(queue_name), m_reply_key(requestbindingkey) {} // Сохраняем "answer queue"

void Server::runThread(amqp_connection_state_t conn)
{
  // Передаем сохраненный ключ в Worker
  Worker* worker = new Worker(conn, m_queue_name, m_reply_key);
  m_pool.start(worker);
}