add_executable(rabbitmq_server
    main.cpp
    Server.h
    Server.cpp
    Worker.h
    Worker.cpp
)

target_link_libraries(rabbitmq_server PRIVATE
    common
    rabbitmq::rabbitmq
    Qt5::Core
)
