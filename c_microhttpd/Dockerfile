FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    libmicrohttpd-dev \
    libpq-dev \
    libjansson-dev \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY app/main.c .

RUN gcc main.c -o myservice \
    -I/usr/include/postgresql \
    -lmicrohttpd -lpq -ljansson

EXPOSE 8080

CMD ["./myservice"]
