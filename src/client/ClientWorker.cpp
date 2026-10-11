#include "ClientWorker.h"
#include "Messages.pb.h"
#include "Logger.h"

#include <amqp_framing.h>
#include <cstring>

ClientWorker::ClientWorker(const Config& config, QObject* parent)
  : QThread(parent)
  , m_config(config)
{
  m_clientId = "client_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  m_replyQueue = "reply_" + m_clientId;
}

ClientWorker::~ClientWorker()
{
  stop();
  wait();
  disconnectFromBroker();
}

void ClientWorker::sendRequest(int number)
{
  QMutexLocker locker(&m_queueMutex);
  m_outgoingQueue.enqueue(number);
  LOG_INFO(QString("Запрос добавлен в очередь: req=%1").arg(number));
}

void ClientWorker::updateConfig(const Config& newConfig)
{
  {
    QMutexLocker locker(&m_configMutex);
    m_config = newConfig;
  }
  m_reconnectRequested.store(true);
  LOG_INFO("Запрошено переподключение с новыми настройками");
}

void ClientWorker::stop()
{
  m_running.store(false);
}

QString ClientWorker::formatRpcReply(const amqp_rpc_reply_t& reply) const
{
  switch (reply.reply_type) {
  case AMQP_RESPONSE_NORMAL:
    return "OK";
  case AMQP_RESPONSE_NONE:
    return "Missing RPC reply";
  case AMQP_RESPONSE_LIBRARY_EXCEPTION:
    return QString("Library error: %1").arg(amqp_error_string2(reply.library_error));
  case AMQP_RESPONSE_SERVER_EXCEPTION:
    return QString("Server exception: 0x%1").arg(reply.reply.id, 0, 16);
  default:
    return "AMQP error";
  }
}

bool ClientWorker::connectToBroker()
{
  BrokerConfig b;
  {
    QMutexLocker locker(&m_configMutex);
    b = m_config.broker();
  }

  m_conn = amqp_new_connection();
  if (!m_conn) {
    emit connectionError("Не удалось создать AMQP контекст");
    return false;
  }

  m_socket = amqp_tcp_socket_new(m_conn);
  if (!m_socket) {
    emit connectionError("Не удалось создать TCP сокет");
    return false;
  }

  int status = amqp_socket_open(m_socket, b.host.toUtf8().constData(), b.port);
  if (status < 0) {
    emit connectionError(QString("Не удалось подключиться к %1:%2 (%3)")
      .arg(b.host).arg(b.port).arg(amqp_error_string2(status)));
    return false;
  }

  amqp_rpc_reply_t loginReply = amqp_login(
    m_conn,
    b.vhost.toUtf8().constData(),
    0,
    AMQP_DEFAULT_FRAME_SIZE,
    0,
    AMQP_SASL_METHOD_PLAIN,
    b.username.toUtf8().constData(),
    b.password.toUtf8().constData()
  );

  if (loginReply.reply_type != AMQP_RESPONSE_NORMAL) {
    emit connectionError(QString("Ошибка авторизации: %1").arg(formatRpcReply(loginReply)));
    return false;
  }

  amqp_channel_open(m_conn, m_channel);
  amqp_rpc_reply_t chReply = amqp_get_rpc_reply(m_conn);
  if (chReply.reply_type != AMQP_RESPONSE_NORMAL) {
    emit connectionError(QString("Не удалось открыть канал: %1").arg(formatRpcReply(chReply)));
    return false;
  }

  return true;
}

void ClientWorker::disconnectFromBroker()
{
  if (m_conn) {
    amqp_channel_close(m_conn, m_channel, AMQP_REPLY_SUCCESS);
    amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(m_conn);
    m_conn = nullptr;
    m_socket = nullptr;
  }
  if (m_connected.exchange(false)) {
    emit disconnected();
  }
}

bool ClientWorker::setupTopology()
{
  BrokerConfig b;
  {
    QMutexLocker locker(&m_configMutex);
    b = m_config.broker();
  }

  // 1. Декларация direct exchange
  amqp_exchange_declare(
    m_conn,
    m_channel,
    amqp_cstring_bytes(b.exchange.toUtf8().constData()),
    amqp_cstring_bytes("direct"),
    0, 1, 0, 0, amqp_empty_table
  );
  amqp_rpc_reply_t exReply = amqp_get_rpc_reply(m_conn);
  if (exReply.reply_type != AMQP_RESPONSE_NORMAL) {
    emit connectionError(QString("Ошибка декларации exchange: %1").arg(formatRpcReply(exReply)));
    return false;
  }

  // 2. Декларация очереди запросов serverQueue (для независимости запуска)
  amqp_queue_declare(
    m_conn,
    m_channel,
    amqp_cstring_bytes(b.requestQueue.toUtf8().constData()),
    0, 1, 0, 0, amqp_empty_table
  );
  amqp_rpc_reply_t qReply = amqp_get_rpc_reply(m_conn);
  if (qReply.reply_type != AMQP_RESPONSE_NORMAL) {
    emit connectionError(QString("Ошибка декларации очереди запросов: %1").arg(formatRpcReply(qReply)));
    return false;
  }

  amqp_queue_bind(
    m_conn,
    m_channel,
    amqp_cstring_bytes(b.requestQueue.toUtf8().constData()),
    amqp_cstring_bytes(b.exchange.toUtf8().constData()),
    amqp_cstring_bytes(b.requestQueue.toUtf8().constData()),
    amqp_empty_table
  );
  amqp_rpc_reply_t bindReply = amqp_get_rpc_reply(m_conn);
  if (bindReply.reply_type != AMQP_RESPONSE_NORMAL) {
    emit connectionError(QString("Ошибка привязки очереди запросов: %1").arg(formatRpcReply(bindReply)));
    return false;
  }

  // 3. Создание временной очереди ответов для клиента
  amqp_queue_declare(
    m_conn,
    m_channel,
    amqp_cstring_bytes(m_replyQueue.toUtf8().constData()),
    0, 0, 1, 1, amqp_empty_table
  );
  amqp_rpc_reply_t replyQDecl = amqp_get_rpc_reply(m_conn);
  if (replyQDecl.reply_type != AMQP_RESPONSE_NORMAL) {
    emit connectionError(QString("Ошибка создания очереди ответов: %1").arg(formatRpcReply(replyQDecl)));
    return false;
  }

  // 4. Привязка очереди ответов по ключу client_id
  amqp_queue_bind(
    m_conn,
    m_channel,
    amqp_cstring_bytes(m_replyQueue.toUtf8().constData()),
    amqp_cstring_bytes(b.exchange.toUtf8().constData()),
    amqp_cstring_bytes(m_clientId.toUtf8().constData()),
    amqp_empty_table
  );
  amqp_rpc_reply_t replyBind = amqp_get_rpc_reply(m_conn);
  if (replyBind.reply_type != AMQP_RESPONSE_NORMAL) {
    emit connectionError(QString("Ошибка привязки очереди ответов: %1").arg(formatRpcReply(replyBind)));
    return false;
  }

  // 5. Подписка на очередь ответов
  amqp_basic_consume(
    m_conn,
    m_channel,
    amqp_cstring_bytes(m_replyQueue.toUtf8().constData()),
    amqp_empty_bytes,
    0, 1, 0, amqp_empty_table
  );
  amqp_rpc_reply_t consReply = amqp_get_rpc_reply(m_conn);
  if (consReply.reply_type != AMQP_RESPONSE_NORMAL) {
    emit connectionError(QString("Ошибка подписки на ответы: %1").arg(formatRpcReply(consReply)));
    return false;
  }

  LOG_INFO(QString("Клиент настроен: очередь ответов '%1'").arg(m_replyQueue));
  return true;
}

void ClientWorker::processOutgoingQueue()
{
  while (true) {
    int reqVal = 0;
    {
      QMutexLocker locker(&m_queueMutex);
      if (m_outgoingQueue.isEmpty()) {
        break;
      }
      reqVal = m_outgoingQueue.dequeue();
      m_pendingRequests.enqueue(reqVal);
    }

    TestTask::Messages::Request request;
    request.set_id(m_clientId.toStdString());
    request.set_req(reqVal);

    std::string payload;
    if (!request.SerializeToString(&payload)) {
      LOG_ERROR("Ошибка сериализации Protobuf Request");
      continue;
    }

    BrokerConfig b;
    {
      QMutexLocker locker(&m_configMutex);
      b = m_config.broker();
    }

    amqp_basic_properties_t props;
    std::memset(&props, 0, sizeof(props));
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/x-protobuf");
    props.delivery_mode = 2; // Persistent

    amqp_bytes_t exBytes = amqp_cstring_bytes(b.exchange.toUtf8().constData());
    amqp_bytes_t rkBytes = amqp_cstring_bytes(b.requestQueue.toUtf8().constData());
    amqp_bytes_t bodyBytes;
    bodyBytes.len = payload.size();
    bodyBytes.bytes = const_cast<char*>(payload.data());

    int status = amqp_basic_publish(
      m_conn,
      m_channel,
      exBytes,
      rkBytes,
      0,
      0,
      &props,
      bodyBytes
    );

    if (status != AMQP_STATUS_OK) {
      LOG_ERROR(QString("Ошибка отправки запроса в RabbitMQ: %1").arg(amqp_error_string2(status)));
      disconnectFromBroker();
      break;
    }

    emit requestSent(reqVal);
    LOG_INFO(QString("Запрос отправлен: req=%1").arg(reqVal));
  }
}

void ClientWorker::checkIncomingMessages()
{
  if (!m_conn || !m_connected.load()) {
    return;
  }

  amqp_maybe_release_buffers(m_conn);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 100000; // 100 мс

  amqp_envelope_t envelope;
  amqp_rpc_reply_t ret = amqp_consume_message(m_conn, &envelope, &timeout, 0);

  if (ret.reply_type == AMQP_RESPONSE_NORMAL) {
    TestTask::Messages::Response resp;
    if (resp.ParseFromArray(envelope.message.body.bytes, static_cast<int>(envelope.message.body.len))) {
      QString replyId = QString::fromStdString(resp.id());
      int resVal = resp.res();

      int originalReq = 0;
      {
        QMutexLocker locker(&m_queueMutex);
        if (!m_pendingRequests.isEmpty()) {
          originalReq = m_pendingRequests.dequeue();
        }
      }

      LOG_INFO(QString("Получен ответ: id=%1, req=%2, res=%3").arg(replyId).arg(originalReq).arg(resVal));
      emit responseReceived(replyId, originalReq, resVal);
    } else {
      LOG_ERROR("Ошибка разбора Protobuf Response");
    }
    amqp_destroy_envelope(&envelope);
  } else if (ret.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
    if (ret.library_error != AMQP_STATUS_TIMEOUT) {
      LOG_ERROR(QString("Сетевая ошибка при чтении ответов: %1").arg(amqp_error_string2(ret.library_error)));
      disconnectFromBroker();
    }
  } else if (ret.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
    LOG_ERROR(QString("Исключение сервера AMQP: %1").arg(formatRpcReply(ret)));
    disconnectFromBroker();
  }
}

void ClientWorker::run()
{
  m_running.store(true);
  LOG_INFO(QString("ClientWorker запущен. ID: %1").arg(m_clientId));

  while (m_running.load()) {
    if (m_reconnectRequested.exchange(false)) {
      disconnectFromBroker();
    }

    if (!m_connected.load()) {
      emit statusChanged("Подключение к RabbitMQ...", false);
      if (!connectToBroker() || !setupTopology()) {
        disconnectFromBroker();
        emit statusChanged("Ошибка подключения к брокеру", true);

        for (int i = 0; i < 30 && m_running.load() && !m_reconnectRequested.load(); ++i) {
          QThread::msleep(100);
        }
        continue;
      }

      m_connected.store(true);
      emit connected();
      emit statusChanged(QString("Подключен к RabbitMQ (%1)").arg(m_clientId), false);
    }

    processOutgoingQueue();
    checkIncomingMessages();

    QThread::msleep(10);
  }

  disconnectFromBroker();
  LOG_INFO("ClientWorker остановлен");
}
