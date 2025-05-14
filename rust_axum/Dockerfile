FROM rust:1.82 as chef
WORKDIR /app

RUN cargo install cargo-chef

COPY ./app .
RUN cargo chef prepare --recipe-path recipe.json

FROM rust:1.82 as builder
WORKDIR /app

RUN cargo install cargo-chef

COPY --from=chef /app/recipe.json recipe.json
RUN cargo chef cook --release --recipe-path recipe.json

COPY ./app .

RUN RUSTFLAGS="-C target-cpu=native" cargo build --release && \
    strip target/release/rust_axum

FROM debian:bullseye-slim

RUN apt-get update && apt-get install -y libpq5 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/target/release/rust_axum /app/rust_axum

ENV RUST_LOG=info
ENV TOKIO_WORKER_THREADS=8

CMD ["/app/rust_axum"]
