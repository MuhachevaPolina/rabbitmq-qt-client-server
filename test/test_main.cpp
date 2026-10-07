#include <gtest/gtest.h>
#include <QApplication>

int main(int argc, char** argv) {
    // Включаем offscreen платформу Qt для стабильного прогона тестов GUI без дисплея
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
