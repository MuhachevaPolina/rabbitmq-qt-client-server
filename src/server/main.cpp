#include <src/server/Server.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const* const* argv)
{
  Server server;
  char const* hostname;
  int port;
  char const* exchange;
  char const* bindingkey;

  amqp_bytes_t queuename;

  hostname = argv[1];
  port = atoi(argv[2]);
  exchange = "amq.direct";   /* argv[3]; */
  bindingkey = "test queue"; /* argv[4]; */

  amqp_connection_state_t conn = amqp_new_connection();
  amqp_socket_t* socket = amqp_tcp_socket_new(conn);

  if(!socket)
  {
    fprintf(stderr, "can't create socket\n");
    return 1;
  }

  int status = amqp_socket_open(socket, hostname, port);
  if(status < 0)
  {
    fprintf(stderr, "can't connect to %s:%d\n", hostname, port);
    return 1;
  }

  amqp_rpc_reply_t login_reply = amqp_login(conn, "rabbitmq_qt", 0, AMQP_DEFAULT_FRAME_SIZE, 0,
                 AMQP_SASL_METHOD_PLAIN, "rabbitmq_qt_user", "rabbitmqqt");

  if(login_reply.reply_type != AMQP_RESPONSE_NORMAL)
  {
    fprintf(stderr, "login error, answer type is %d\n", login_reply.reply_type);
    
    if (login_reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
        // broker rejected by itself
        fprintf(stderr, "server error, AMQP ID 0x%X\n", login_reply.reply.id);
    } else if (login_reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
        // network or lib error
        fprintf(stderr, "lib error: %s\n", amqp_error_string2(login_reply.library_error));
    }
    return 1;
  }

  // open chan after login
  amqp_channel_open_ok_t *ch_ok = amqp_channel_open(conn, 1);
  amqp_rpc_reply_t ch_reply = amqp_get_rpc_reply(conn);
  if (ch_reply.reply_type != AMQP_RESPONSE_NORMAL) {
      fprintf(stderr, "can't open channel\n");
      return 1;
  }

  amqp_queue_declare_ok_t* r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0,
                                                  0, 0, 1, amqp_empty_table);
  queuename = amqp_bytes_malloc_dup(r->queue);

  amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange),
                  amqp_cstring_bytes(bindingkey), amqp_empty_table);
  amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0,
                     amqp_empty_table);

  server.run(conn);

  amqp_bytes_free(queuename);

  return 0;
}