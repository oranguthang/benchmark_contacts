FROM gcc:14-bookworm AS build

WORKDIR /build

# Установим зависимости для сборки
RUN apt-get update && \
    apt-get install -y cmake libpqxx-dev libssl-dev nlohmann-json3-dev

# Копируем исходники в контейнер
COPY app /build/app

# Собираем проект
RUN cmake -Bbuild -H/build/app && cmake --build build --target server

FROM debian:bookworm-slim

WORKDIR /app

# Устанавливаем runtime зависимости
RUN apt-get update && \
    apt-get install -y libpq5 && \
    rm -rf /var/lib/apt/lists/*

# Копируем готовый бинарник
COPY --from=build /build/build/server /app/server

EXPOSE 8080

CMD ["./server"]
