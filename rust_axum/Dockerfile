FROM rust:1.82

RUN apt-get update && \
    apt-get install -y \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY ./app/Cargo.toml ./app/
RUN mkdir -p ./app/src && \
    echo "fn main() {}" > ./app/src/main.rs && \
    cargo build --release && \
    rm -rf target/release/deps/rust_axum*

COPY ./app /app/app

RUN cd ./app && \
    RUSTFLAGS="-C target-cpu=native" cargo build --release && \
    strip target/release/rust_axum

ENV RUST_LOG=info
ENV TOKIO_WORKER_THREADS=8

CMD ["./app/target/release/rust_axum"]
