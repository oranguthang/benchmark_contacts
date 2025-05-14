FROM rust:1.82 AS chef
RUN cargo install cargo-chef

WORKDIR /app
COPY ./app/Cargo.toml ./app/Cargo.lock ./
RUN cargo chef prepare --recipe-path recipe.json

FROM rust:1.82 AS builder
RUN apt-get update && apt-get install -y libpq-dev && rm -rf /var/lib/apt/lists/*
RUN cargo install cargo-chef

WORKDIR /app
COPY --from=chef /app/recipe.json recipe.json
RUN cargo chef cook --release --recipe-path recipe.json

COPY ./app ./
COPY sqlx-data.json ./

RUN RUSTFLAGS="-C target-cpu=native" cargo build --release && \
    strip target/release/rust_axum

FROM debian:bullseye-slim
RUN apt-get update && apt-get install -y libpq5 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/target/release/rust_axum .

ENV RUST_LOG=info
ENV TOKIO_WORKER_THREADS=8
CMD ["./rust_axum"]
