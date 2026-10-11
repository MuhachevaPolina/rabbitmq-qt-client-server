#pragma once

#include "Config.h"
#include "Worker.h"

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <atomic>
#include <QString>

class Server {
public:
  explicit Server(const Config& config);
  ~Server();

  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  bool init();
  void run();
  void stop();
  bool isRunning() const { return m_running.load(); }
  void cleanup();

private:
  bool setupConnection();
  bool setupTopology();
  QString formatRpcReply(const amqp_rpc_reply_t& reply) const;

  Config m_config;
  amqp_connection_state_t m_conn = nullptr;
  amqp_socket_t* m_socket = nullptr;
  amqp_channel_t m_channel = 1;
  Worker m_worker;
  std::atomic<bool> m_running{false};
  bool m_initialized{false};
};