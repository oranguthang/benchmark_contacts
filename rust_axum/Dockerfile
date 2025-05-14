FROM rust:1.82

RUN apt-get update && \
    apt-get install -y \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY ./app/Cargo.toml ./

RUN mkdir -p src && \
    echo "fn main() {}" > src/main.rs && \
    cargo build --release && \
    rm -rf target/release/deps/rust_axum*

COPY ./app/src ./src

RUN RUSTFLAGS="-C target-cpu=native" cargo build --release && \
    strip target/release/rust_axum

ENV RUST_LOG=info
ENV CPU_CORES=8

CMD ["target/release/rust_axum"]
