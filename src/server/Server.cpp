#include "Server.h"
#include "Logger.h"

#include <amqp_framing.h>

Server::Server(const Config& config)
    : m_config(config)
{
}

Server::~Server()
{
    cleanup();
}

QString Server::formatRpcReply(const amqp_rpc_reply_t& reply) const
{
    switch (reply.reply_type) {
    case AMQP_RESPONSE_NORMAL:
        return "Успешно";
    case AMQP_RESPONSE_NONE:
        return "Отсутствует ответ RPC";
    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        return QString("Системная/сетевая ошибка: %1").arg(amqp_error_string2(reply.library_error));
    case AMQP_RESPONSE_SERVER_EXCEPTION: {
        if (reply.reply.id == AMQP_CONNECTION_CLOSE_METHOD) {
            auto* method = reinterpret_cast<amqp_connection_close_t*>(reply.reply.decoded);
            return QString("Брокер закрыл соединение (%1): %2")
                .arg(method->reply_code)
                .arg(QString::fromUtf8(reinterpret_cast<const char*>(method->reply_text.bytes), method->reply_text.len));
        } else if (reply.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
            auto* method = reinterpret_cast<amqp_channel_close_t*>(reply.reply.decoded);
            return QString("Брокер закрыл канал (%1): %2")
                .arg(method->reply_code)
                .arg(QString::fromUtf8(reinterpret_cast<const char*>(method->reply_text.bytes), method->reply_text.len));
        }
        return QString("Исключение брокера AMQP (метод: 0x%1)").arg(reply.reply.id, 0, 16);
    }
    default:
        return "Неизвестная ошибка AMQP";
    }
}

bool Server::setupConnection()
{
    m_conn = amqp_new_connection();
    if (!m_conn) {
        LOG_ERROR("Не удалось создать контекст AMQP соединения");
        return false;
    }

    m_socket = amqp_tcp_socket_new(m_conn);
    if (!m_socket) {
        LOG_ERROR("Не удалось создать TCP-сокет для AMQP");
        return false;
    }

    const QString host = m_config.broker().host;
    const int port = m_config.broker().port;

    int status = amqp_socket_open(m_socket, host.toUtf8().constData(), port);
    if (status < 0) {
        LOG_ERROR(QString("Не удалось открыть TCP соединение с брокером %1:%2: %3")
            .arg(host).arg(port).arg(amqp_error_string2(status)));
        return false;
    }
    LOG_INFO(QString("Установлено TCP соединение с брокером %1:%2").arg(host).arg(port));

    // Аутентификация в виртуальном хосте
    amqp_rpc_reply_t loginReply = amqp_login(
        m_conn,
        m_config.broker().vhost.toUtf8().constData(),
        0,                        // channel_max
        AMQP_DEFAULT_FRAME_SIZE,  // frame_max
        0,                        // heartbeat
        AMQP_SASL_METHOD_PLAIN,
        m_config.broker().username.toUtf8().constData(),
        m_config.broker().password.toUtf8().constData()
    );

    if (loginReply.reply_type != AMQP_RESPONSE_NORMAL) {
        LOG_ERROR(QString("Ошибка авторизации пользователя '%1' в vhost '%2': %3")
            .arg(m_config.broker().username)
            .arg(m_config.broker().vhost)
            .arg(formatRpcReply(loginReply)));
        return false;
    }
    LOG_INFO(QString("Успешная авторизация пользователя '%1' в vhost '%2'")
        .arg(m_config.broker().username)
        .arg(m_config.broker().vhost));

    // Открытие AMQP канала
    amqp_channel_open(m_conn, m_channel);
    amqp_rpc_reply_t chReply = amqp_get_rpc_reply(m_conn);
    if (chReply.reply_type != AMQP_RESPONSE_NORMAL) {
        LOG_ERROR(QString("Не удалось открыть AMQP канал %1: %2")
            .arg(m_channel)
            .arg(formatRpcReply(chReply)));
        return false;
    }
    LOG_INFO(QString("AMQP канал %1 успешно открыт").arg(m_channel));

    return true;
}

bool Server::setupTopology()
{
    const QString exchange = m_config.broker().exchange;
    const QString requestQueue = m_config.broker().requestQueue;

    // 1. Декларация direct exchange (идемпотентно: passive = 0, durable = 1)
    amqp_exchange_declare(
        m_conn,
        m_channel,
        amqp_cstring_bytes(exchange.toUtf8().constData()),
        amqp_cstring_bytes("direct"),
        0, // passive
        1, // durable
        0, // auto_delete
        0, // internal
        amqp_empty_table
    );
    amqp_rpc_reply_t exReply = amqp_get_rpc_reply(m_conn);
    if (exReply.reply_type != AMQP_RESPONSE_NORMAL) {
        LOG_ERROR(QString("Не удалось объявить exchange '%1': %2")
            .arg(exchange).arg(formatRpcReply(exReply)));
        return false;
    }
    LOG_INFO(QString("Exchange '%1' (direct, durable) успешно проверен/объявлен").arg(exchange));

    // 2. Декларация очереди запросов serverQueue (идемпотентно: passive = 0, durable = 1)
    amqp_queue_declare(
        m_conn,
        m_channel,
        amqp_cstring_bytes(requestQueue.toUtf8().constData()),
        0, // passive
        1, // durable
        0, // exclusive
        0, // auto_delete
        amqp_empty_table
    );
    amqp_rpc_reply_t qReply = amqp_get_rpc_reply(m_conn);
    if (qReply.reply_type != AMQP_RESPONSE_NORMAL) {
        LOG_ERROR(QString("Не удалось объявить очередь '%1': %2")
            .arg(requestQueue).arg(formatRpcReply(qReply)));
        return false;
    }
    LOG_INFO(QString("Очередь '%1' (durable) успешно проверена/объявлена").arg(requestQueue));

    // 3. Привязка очереди к exchange по ключу routing key = имени очереди
    amqp_queue_bind(
        m_conn,
        m_channel,
        amqp_cstring_bytes(requestQueue.toUtf8().constData()),
        amqp_cstring_bytes(exchange.toUtf8().constData()),
        amqp_cstring_bytes(requestQueue.toUtf8().constData()),
        amqp_empty_table
    );
    amqp_rpc_reply_t bindReply = amqp_get_rpc_reply(m_conn);
    if (bindReply.reply_type != AMQP_RESPONSE_NORMAL) {
        LOG_ERROR(QString("Не удалось привязать очередь '%1' к exchange '%2' по ключу '%3': %4")
            .arg(requestQueue).arg(exchange).arg(requestQueue).arg(formatRpcReply(bindReply)));
        return false;
    }
    LOG_INFO(QString("Очередь '%1' привязана к exchange '%2' с routing key '%3'")
        .arg(requestQueue).arg(exchange).arg(requestQueue));

    // 4. Подписка на сообщения очереди (no_ack = 0 для явного ручного подтверждения)
    amqp_basic_consume(
        m_conn,
        m_channel,
        amqp_cstring_bytes(requestQueue.toUtf8().constData()),
        amqp_empty_bytes, // consumer tag от брокера
        0, // no_local
        0, // no_ack = 0 (требуется ack)
        0, // exclusive
        amqp_empty_table
    );
    amqp_rpc_reply_t consReply = amqp_get_rpc_reply(m_conn);
    if (consReply.reply_type != AMQP_RESPONSE_NORMAL) {
        LOG_ERROR(QString("Не удалось подписаться на очередь '%1': %2")
            .arg(requestQueue).arg(formatRpcReply(consReply)));
        return false;
    }
    LOG_INFO(QString("Подписка на сообщения из очереди '%1' активирована").arg(requestQueue));

    return true;
}

bool Server::init()
{
    cleanup();

    if (!setupConnection()) {
        cleanup();
        return false;
    }

    if (!setupTopology()) {
        cleanup();
        return false;
    }

    m_initialized = true;
    return true;
}

void Server::run()
{
    if (!m_initialized || !m_conn) {
        LOG_ERROR("Попытка запуска Server::run() без успешной инициализации init()");
        return;
    }

    m_running.store(true);
    LOG_INFO("Основной рабочий цикл сервера запущен. Ожидание запросов...");

    while (m_running.load()) {
        amqp_maybe_release_buffers(m_conn);

        // Таймаут 250 мс для периодической проверки флага остановки
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 250000;

        amqp_envelope_t envelope;
        amqp_rpc_reply_t ret = amqp_consume_message(m_conn, &envelope, &timeout, 0);

        if (!m_running.load()) {
            if (ret.reply_type == AMQP_RESPONSE_NORMAL) {
                amqp_destroy_envelope(&envelope);
            }
            break;
        }

        if (ret.reply_type == AMQP_RESPONSE_NORMAL) {
            m_worker.handleMessage(m_conn, m_channel, envelope, m_config.broker().exchange);
            amqp_destroy_envelope(&envelope);
        } else if (ret.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
            if (ret.library_error == AMQP_STATUS_TIMEOUT) {
                // Очередь пуста в данный интервал таймаута - нормальная работа
                continue;
            }

            if (ret.library_error == AMQP_STATUS_UNEXPECTED_STATE) {
                amqp_frame_t frame;
                if (amqp_simple_wait_frame(m_conn, &frame) == AMQP_STATUS_OK) {
                    if (frame.frame_type == AMQP_FRAME_METHOD) {
                        if (frame.payload.method.id == AMQP_CHANNEL_CLOSE_METHOD ||
                            frame.payload.method.id == AMQP_CONNECTION_CLOSE_METHOD) {
                            LOG_ERROR("Канал или соединение закрыто со стороны брокера RabbitMQ");
                            break;
                        }
                    }
                }
            } else {
                LOG_ERROR(QString("Сетевая ошибка при чтении сообщений: %1").arg(amqp_error_string2(ret.library_error)));
                break;
            }
        } else if (ret.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
            LOG_ERROR(QString("Исключение сервера AMQP: %1").arg(formatRpcReply(ret)));
            break;
        }
    }

    LOG_INFO("Рабочий цикл сервера завершен");
}

void Server::stop()
{
    m_running.store(false);
}

void Server::cleanup()
{
    if (m_conn) {
        amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
        amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(m_conn);
        m_conn = nullptr;
        m_socket = nullptr;
        LOG_INFO("Ресурсы AMQP соединения корректно освобождены");
    }
    m_initialized = false;
    m_running.store(false);
}