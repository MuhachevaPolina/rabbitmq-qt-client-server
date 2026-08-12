#include <src/server/Server.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <iostream>
#include <stdexcept>
#include <string>

void check_amqp_error(amqp_rpc_reply_t reply, const std::string& context)
{
  if(reply.reply_type != AMQP_RESPONSE_NORMAL)
  {
    throw std::runtime_error(context + " failed; reply: " + std::to_string(reply.reply_type));
  }
}

int main(int argc, char const* const* argv)
{
  if(argc < 3)
  {
    std::cerr << "error: waiting for <host> <port> arguments input\n";
    return 1;
  }

  std::string hostname = argv[1];
  int port = std::stoi(argv[2]);

  std::string exchange = "amq.direct";
  std::string bindingkey = "test queue";

  Server* server = new Server();
  amqp_connection_state_t conn = amqp_new_connection();

  try
  {
    amqp_socket_t* socket = amqp_tcp_socket_new(conn);
    if(!socket)
    {
      throw std::runtime_error("error: creating TCP socket");
    }

    if(amqp_socket_open(socket, hostname.c_str(), port) != 0)
    {
      throw std::runtime_error("error: opening socket on " + hostname + ":" + std::to_string(port));
    }

    check_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"), "login");

    amqp_channel_open(conn, 1);
    check_amqp_error(amqp_get_rpc_reply(conn), "open chan 1");

    amqp_queue_declare_ok_t* r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
    check_amqp_error(amqp_get_rpc_reply(conn), "create queue");

    amqp_bytes_t queuename = amqp_bytes_malloc_dup(r->queue);
    if(queuename.bytes == nullptr)
    {
      throw std::runtime_error("error: not enough free memory");
    }

    amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(bindingkey.c_str()), amqp_empty_table);
    check_amqp_error(amqp_get_rpc_reply(conn), "queue binding");

    amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    check_amqp_error(amqp_get_rpc_reply(conn), "listener start (Consume)");

    server->run(conn);

    amqp_bytes_free(queuename);
    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
  }
  catch(const std::exception& e)
  {
    std::cerr << "error: " << e.what() << std::endl;
    amqp_destroy_connection(conn);
    delete server;
    return 1;
  }

  delete server;
  return 0;
}