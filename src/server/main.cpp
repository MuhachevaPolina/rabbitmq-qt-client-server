#include "Server.h"
#include "Config.h"
#include "Logger.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <csignal>

static Server* g_serverInstance = nullptr;

void handleSignal(int signum)
{
  LOG_INFO(QString("Получен сигнал завершения (%1). Остановка сервера...").arg(signum));
  if (g_serverInstance) {
    g_serverInstance->stop();
  }
}

int main(int argc, char* argv[])
{
  QCoreApplication app(argc, argv);

  QString configPath = "configs/server.ini";
  if (argc > 1) {
    configPath = QString::fromUtf8(argv[1]);
  }

  if (!QFileInfo::exists(configPath)) {
    QString altPath1 = QDir::current().filePath("../configs/server.ini");
    QString altPath2 = QCoreApplication::applicationDirPath() + "/../../../configs/server.ini";
    QString altPath3 = QCoreApplication::applicationDirPath() + "/../../configs/server.ini";
    if (QFileInfo::exists(altPath1)) {
      configPath = altPath1;
    } else if (QFileInfo::exists(altPath2)) {
      configPath = altPath2;
    } else if (QFileInfo::exists(altPath3)) {
      configPath = altPath3;
    }
  }

  Config config;
  if (!config.load(configPath)) {
    LOG_WARN(QString("Файл конфигурации '%1' не найден, используются значения по умолчанию").arg(configPath));
  }

  Logger::instance().init(config.logging().logPath, config.logging().logLevel);

  LOG_INFO("Запуск сервера RabbitMQ");
  LOG_INFO(QString("Конфиг: %1").arg(configPath));
  LOG_INFO(QString("Хост: %1:%2, Очередь: %3").arg(config.broker().host).arg(config.broker().port).arg(config.broker().requestQueue));

  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);

  Server server(config);
  g_serverInstance = &server;

  if (!server.init()) {
    LOG_ERROR("Ошибка инициализации сервера. Завершение работы.");
    g_serverInstance = nullptr;
    return 1;
  }

  LOG_INFO("Сервер готов к обработке запросов");
  server.run();

  g_serverInstance = nullptr;
  LOG_INFO("Сервер успешно остановлен");
  return 0;
}