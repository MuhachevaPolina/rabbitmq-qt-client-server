#include "Client.h"

#include <iostream>

#ifndef amqp_literal_bytes
#define amqp_literal_bytes(str) \
  (amqp_bytes_t) { sizeof(str) - 1, (void*)(str) }
#endif

Client::Client() {}

void Client::connect(int argc, char const* const* argv)
{
  char const* hostname;
  int port, status;
  int message_count;
  amqp_socket_t* socket = NULL;
  amqp_connection_state_t conn;

  char const* exchange;
  char const* bindingkey;

  amqp_bytes_t queuename;

  hostname = argv[1];
  port = atoi(argv[2]);
  message_count = atoi(argv[3]);

  exchange = "amq.direct";   /* argv[3]; */
  bindingkey = "test queue"; /* argv[4]; */

  conn = amqp_new_connection();

  socket = amqp_tcp_socket_new(conn);
  if(!socket)
  {
    // die("creating TCP socket");
  }

  status = amqp_socket_open(socket, hostname, port);
  if(status)
  {
    // die("opening TCP socket");
  }

  amqp_rpc_reply_t login_reply =
      amqp_login(conn, "rabbitmq_qt", 0, AMQP_DEFAULT_FRAME_SIZE, 0,
                 AMQP_SASL_METHOD_PLAIN, "rabbitmq_qt_user", "rabbitmqqt");

  if(login_reply.reply_type != AMQP_RESPONSE_NORMAL)
  {
    fprintf(stderr, "login error, answer type is %d\n", login_reply.reply_type);

    if(login_reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION)
    {
      // broker rejected by itself
      fprintf(stderr, "server error, AMQP ID 0x%X\n", login_reply.reply.id);
    }
    else if(login_reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION)
    {
      // network or lib error
      fprintf(stderr, "lib error: %s\n",
              amqp_error_string2(login_reply.library_error));
    }
  }

  amqp_channel_open(conn, 1);

  amqp_queue_declare_ok_t* r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0,
                                                  0, 0, 1, amqp_empty_table);
  queuename = amqp_bytes_malloc_dup(r->queue);

  amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange),
                  amqp_cstring_bytes(bindingkey), amqp_empty_table);

  send_batch(conn, amqp_literal_bytes("test queue"), message_count);
}

void Client::send_batch(amqp_connection_state_t conn, amqp_bytes_t queue_name,
                        int message_count)
{
  int i;

  amqp_basic_properties_t props;
  props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
  props.content_type = amqp_cstring_bytes("text/plain");
  props.delivery_mode = 2;  // Persistent

  amqp_bytes_t message_bytes = amqp_cstring_bytes("Hello, Server!");

  for(i = 0; i < message_count; i++)
  {
    int res = amqp_basic_publish(conn, 1, amqp_literal_bytes("amq.direct"),
                                 queue_name, 0, 0, NULL, message_bytes);
    if(res == 0)
    {
      std::cout << "sent 'Hello, Server!' message" << std::endl;
    }
    else
    {

    }
  }
}

bool Client::getAnswer()
{
  int received = 0;

  amqp_frame_t frame;

  for(;;)
  {
    amqp_rpc_reply_t ret;
    amqp_envelope_t envelope;

    amqp_maybe_release_buffers(m_conn);
    ret = amqp_consume_message(m_conn, &envelope, NULL, 0);

    if(AMQP_RESPONSE_NORMAL != ret.reply_type)
    {
      if(AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type &&
         AMQP_STATUS_UNEXPECTED_STATE == ret.library_error)
      {
        if(AMQP_STATUS_OK != amqp_simple_wait_frame(m_conn, &frame))
        {
          return false;
        }

        if(AMQP_FRAME_METHOD == frame.frame_type)
        {
          switch(frame.payload.method.id)
          {
            case AMQP_BASIC_ACK_METHOD:
              /* if we've turned publisher confirms on, and we've published a
               * message here is a message being confirmed.
               */
              break;
            case AMQP_BASIC_RETURN_METHOD:
              /* if a published message couldn't be routed and the mandatory
               * flag was set this is what would be returned. The message then
               * needs to be read.
               */
              {
                amqp_message_t message;
                ret = amqp_read_message(m_conn, frame.channel, &message, 0);
                if(AMQP_RESPONSE_NORMAL != ret.reply_type)
                {
                  return false;
                }

                amqp_destroy_message(&message);
              }

              break;

            case AMQP_CHANNEL_CLOSE_METHOD:
              /* a channel.close method happens when a channel exception occurs,
               * this can happen by publishing to an exchange that doesn't exist
               * for example.
               *
               * In this case you would need to open another channel redeclare
               * any queues that were declared auto-delete, and restart any
               * consumers that were attached to the previous channel.
               */
              return false;

            case AMQP_CONNECTION_CLOSE_METHOD:
              /* a m_connection.close method happens when a m_connection exception
               * occurs, this can happen by trying to use a channel that isn't
               * open for example.
               *
               * In this case the whole m_connection must be restarted.
               */
              return false;

            default:
              fprintf(stderr, "An unexpected method was received %u\n",
                      frame.payload.method.id);
              return false;
          }
        }
      }
    }
    else
    {
      printf("messsage (%.*s): ", (int)envelope.message.body.len,
             (char*)envelope.message.body.bytes);
      printf("%.*s\n", (int)envelope.message.body.len,
             (char*)envelope.message.body.bytes);

      amqp_destroy_envelope(&envelope);
    }
    received++;
  }
}