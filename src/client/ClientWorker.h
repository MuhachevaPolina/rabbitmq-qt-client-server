#pragma once

#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QString>
#include <QUuid>
#include <atomic>

#include "Config.h"

#include <amqp.h>
#include <amqp_tcp_socket.h>

class ClientWorker : public QThread {
    Q_OBJECT

public:
    explicit ClientWorker(const Config& config, QObject* parent = nullptr);
    ~ClientWorker() override;

    QString clientId() const { return m_clientId; }
    bool isConnected() const { return m_connected.load(); }

public slots:
    void sendRequest(int number);
    void updateConfig(const Config& newConfig);
    void stop();

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& errorMessage);
    void statusChanged(const QString& statusText, bool isError);
    void requestSent(int reqNumber);
    void responseReceived(const QString& clientId, int reqNumber, int resNumber);

protected:
    void run() override;

private:
    bool connectToBroker();
    void disconnectFromBroker();
    bool setupTopology();
    void processOutgoingQueue();
    void checkIncomingMessages();
    QString formatRpcReply(const amqp_rpc_reply_t& reply) const;

    Config m_config;
    QString m_clientId;
    QString m_replyQueue;

    amqp_connection_state_t m_conn = nullptr;
    amqp_socket_t* m_socket = nullptr;
    amqp_channel_t m_channel = 1;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_reconnectRequested{false};

    QMutex m_queueMutex;
    QQueue<int> m_outgoingQueue;
    QQueue<int> m_pendingRequests;

    QMutex m_configMutex;
};
