#include <QApplication>
#include "MainWindow.h"
#include "Logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("RabbitMQClient");

    Logger::instance().init("client.log", LogLevel::Info);

    MainWindow window;
    window.show();

    return app.exec();
}