# RabbitMQ Qt Client-Server

## Сборка и запуск

### 1. Зависимости
В системе должны быть установлены:
- Компилятор C++ с поддержкой C++17 (GCC / Clang)
- CMake 3.15+
- Qt 5 (модули `Qt5Core`, `Qt5Widgets`)
- Protocol Buffers компилятор и библиотека (`libprotobuf-dev`, `protobuf-compiler`)
- `librabbitmq-dev`
- `libgtest-dev`

### 2. Сборка проекта
```bash
cmake -B build -S .

cmake --build build -j$(nproc)
```

### 3. Запуск тестов
```bash
cd build
ctest --output-on-failure
```

### 4. Запуск приложений
```bash
./build/src/server/rabbitmq_server

./build/src/client/rabbitmq_client
```
