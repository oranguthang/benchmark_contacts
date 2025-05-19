FROM gcc:14-bookworm AS build

WORKDIR /app

# Установим зависимости
RUN apt-get update && \
    apt-get install -y cmake libpqxx-dev libssl-dev nlohmann-json3-dev

COPY . .

# Собираем проект
RUN cmake -Bbuild -H. && cmake --build build --target server

FROM debian:bookworm-slim

WORKDIR /app

# Устанавливаем libpq
RUN apt-get update && \
    apt-get install -y libpq5 && \
    rm -rf /var/lib/apt/lists/*

# Копируем бинарник из стадии сборки
COPY --from=build /app/build/server /app/server

EXPOSE 8080

CMD ["./server"]
