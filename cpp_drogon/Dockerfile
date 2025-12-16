FROM drogonframework/drogon:latest

RUN apt-get update && \
    apt-get install -y libpq-dev libjsoncpp-dev libfmt-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j$(nproc)

# Environment variables
ENV DB_HOST=db \
    DB_PORT=5432 \
    DB_NAME=contacts_db \
    DB_USER=user \
    DB_PASSWORD=password \
    APP_PORT=8080

CMD ["./build/server"]
