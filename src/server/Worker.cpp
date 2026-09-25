#include "Worker.h"

#include <stdio.h>
#include <iostream>

#ifndef amqp_literal_bytes
#define amqp_literal_bytes(str) \
  (amqp_bytes_t) { sizeof(str) - 1, (void*)(str) }
#endif

// Конструктор принимает reply_key (который равен "answer queue")
Worker::Worker(amqp_connection_state_t conn, amqp_bytes_t queue_name, char const* reply_key)
  : QRunnable(), m_conn(conn), m_queue_name(queue_name), m_reply_key(reply_key) {}

void Worker::run()
{
  doAccept();
}

bool Worker::doAccept()
{
  int received = 0;

  amqp_frame_t frame;

  for(;;)
  {
    amqp_rpc_reply_t ret;
    amqp_envelope_t envelope;

    amqp_maybe_release_buffers(m_conn);
    ret = amqp_consume_message(m_conn, &envelope, NULL, 0);
    // amqp_bytes_t queue_name;

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

      int ret = onAccept(m_conn, m_queue_name);

      amqp_destroy_envelope(&envelope);
    }
    received++;
  }
}

bool Worker::onAccept(amqp_connection_state_t conn, amqp_bytes_t queue_name)
{
  // int i;

  amqp_basic_properties_t props;
  props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
  props.content_type = amqp_cstring_bytes("text/plain");
  props.delivery_mode = 2;  // Persistent

  amqp_bytes_t message_bytes = amqp_cstring_bytes("Hello, Client!");


    // 🔴 ИСПРАВЛЕНИЕ: Используем m_reply_key вместо m_requestbindingkey
  // И передаем &props вместо NULL, чтобы применились свойства фрейма
  
  int res = amqp_basic_publish(conn, 1, amqp_literal_bytes("amq.direct"),
                               amqp_cstring_bytes(m_reply_key), 0, 0, &props, message_bytes);
    if(res == 0)
    {
      std::cout << "sent 'Hello, Client!' message" << std::endl;
      return true;
    }
    else
    {
      return false;
    }
}
