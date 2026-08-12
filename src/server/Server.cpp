#include <src/server/Server.h>

Server::Server() {}

void run(amqp_connection_state_t conn)
{
  amqp_frame_t frame;

  for(;;)
  {
    amqp_rpc_reply_t ret;
    amqp_envelope_t envelope;

    amqp_maybe_release_buffers(conn);
    ret = amqp_consume_message(conn, &envelope, NULL, 0);

    if(AMQP_RESPONSE_NORMAL != ret.reply_type)
    {
      if(AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type &&
         AMQP_STATUS_UNEXPECTED_STATE == ret.library_error)
      {
        if(AMQP_STATUS_OK != amqp_simple_wait_frame(conn, &frame))
        {
          return;
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
                ret = amqp_read_message(conn, frame.channel, &message, 0);
                if(AMQP_RESPONSE_NORMAL != ret.reply_type)
                {
                  return;
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
              return;

            case AMQP_CONNECTION_CLOSE_METHOD:
              /* a connection.close method happens when a connection exception
               * occurs, this can happen by trying to use a channel that isn't
               * open for example.
               *
               * In this case the whole connection must be restarted.
               */
              return;

            default:
              /* fprintf(stderr, "An unexpected method was received %u\n",
                      frame.payload.method.id); */
              return;
          }
        }
      }
    }
    else
    {
      amqp_destroy_envelope(&envelope);
    }
  }
}