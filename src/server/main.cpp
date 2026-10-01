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

    // 1. Определение пути к конфигурационному файлу (аргумент argv[1] или configs/server.ini)
    QString configPath = "configs/server.ini";
    if (argc > 1) {
        configPath = QString::fromUtf8(argv[1]);
    }

    // Если файл не найден по указанному пути, проверяем относительно папки приложения
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

    // 2. Чтение конфигурации
    Config config;
    if (!config.load(configPath)) {
        LOG_WARN(QString("Конфигурационный файл '%1' не найден. Применяются параметры по умолчанию.")
            .arg(configPath));
    }

    // 3. Инициализация глобального логгера
    Logger::instance().init(config.logging().logPath, config.logging().logLevel);

    LOG_INFO("==================================================");
    LOG_INFO("Запуск RabbitMQ Qt Server (rabbitmq_server v1.0)");
    LOG_INFO(QString("Конфиг:          %1").arg(configPath));
    LOG_INFO(QString("RabbitMQ Хост:   %1:%2").arg(config.broker().host).arg(config.broker().port));
    LOG_INFO(QString("Virtual Host:    %1").arg(config.broker().vhost));
    LOG_INFO(QString("Пользователь:    %1").arg(config.broker().username));
    LOG_INFO(QString("Exchange:        %1 (direct)").arg(config.broker().exchange));
    LOG_INFO(QString("Очередь:         %1").arg(config.broker().requestQueue));
    LOG_INFO(QString("Файл логов:      %1 (уровень: %2)")
        .arg(config.logging().logPath)
        .arg(Logger::levelToString(config.logging().logLevel)));
    LOG_INFO("==================================================");

    // 4. Регистрация обработчиков сигналов завершения (SIGINT, SIGTERM)
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    // 5. Инициализация и запуск сервера
    Server server(config);
    g_serverInstance = &server;

    if (!server.init()) {
        LOG_ERROR("Ошибка инициализации сервера и подключения к RabbitMQ. Завершение работы.");
        g_serverInstance = nullptr;
        return 1;
    }

    LOG_INFO("Сервер готов к приему запросов от клиентов.");
    server.run();

    g_serverInstance = nullptr;
    LOG_INFO("Сервер корректно остановлен.");
    return 0;
}