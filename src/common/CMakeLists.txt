find_package(Protobuf REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS Messages.proto)

add_library(common STATIC
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    Config.h
    Config.cpp
    Logger.h
    Logger.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(common PUBLIC
    protobuf::libprotobuf
    Qt5::Core
)
