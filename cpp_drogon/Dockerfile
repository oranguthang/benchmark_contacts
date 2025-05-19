FROM drogonframework/drogon:latest

# Устанавливаем зависимости
RUN apt-get update && \
    apt-get install -y libjsoncpp-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . /build

RUN mkdir -p /build/build && \
    cd /build/build && \
    cmake .. && \
    make -j$(nproc)

CMD ["/build/build/server"]