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

    // Запрет копирования
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // Подключение к брокеру, открытие канала, идемпотентная декларация обменника и очереди
    bool init();

    // Основной цикл приема и обработки запросов
    void run();

    // Остановка сервера (потокобезопасно)
    void stop();

    // Проверка статуса работы сервера
    bool isRunning() const { return m_running.load(); }

    // Корректное закрытие ресурсов AMQP
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