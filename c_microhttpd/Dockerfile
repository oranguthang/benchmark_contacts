# Используем образ с gcc и нужными dev-библиотеками (debian-based)
FROM debian:bullseye-slim

# Устанавливаем зависимости: компилятор, библиотеки и заголовки
RUN apt-get update && apt-get install -y \
    build-essential \
    libmicrohttpd-dev \
    libpq-dev \
    libjansson-dev \
    && rm -rf /var/lib/apt/lists/*

# Создаём рабочую директорию
WORKDIR /app

# Копируем исходники
COPY app/main.c .

# Компилируем
RUN gcc main.c -o myservice \
    -lmicrohttpd -lpq -ljansson

# Указываем порт, который будет слушать сервис
EXPOSE 8080

# Запускаем сервис
CMD ["./myservice"]
