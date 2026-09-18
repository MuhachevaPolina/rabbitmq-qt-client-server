#include "Worker.h"

#include <stdio.h>

Worker::Worker(amqp_connection_state_t conn): QRunnable(), m_conn(conn) {}

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
