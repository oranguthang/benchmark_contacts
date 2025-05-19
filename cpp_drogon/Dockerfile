FROM drogonframework/drogon:latest

# Устанавливаем зависимости для сборки
RUN apt-get update && \
    apt-get install -y libjsoncpp-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . /build

RUN mkdir -p /build/build && \
    cd /build/build && \
    cmake .. && \
    make -j$(nproc)

# Переменные окружения для конфигурации
ENV DB_HOST=db \
    DB_PORT=5432 \
    DB_NAME=contacts_db \
    DB_USER=user \
    DB_PASSWORD=password \
    APP_PORT=8080

CMD ["/build/build/server"]
