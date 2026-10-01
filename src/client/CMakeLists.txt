set(CMAKE_AUTOMOC ON)

add_executable(rabbitmq_client
    main.cpp
    MainWindow.h
    MainWindow.cpp
    SettingsDialog.h
    SettingsDialog.cpp
    ClientWorker.h
    ClientWorker.cpp
)

target_link_libraries(rabbitmq_client PRIVATE
    common
    rabbitmq::rabbitmq
    Qt5::Widgets
)
