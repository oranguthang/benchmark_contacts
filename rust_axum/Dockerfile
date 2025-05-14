FROM rust:1.82 AS builder

WORKDIR /app

RUN cargo install sqlx-cli --no-default-features --features postgres

COPY app/Cargo.toml app/Cargo.lock ./
COPY app/src ./src

COPY app/.sqlx ./.sqlx

RUN RUSTFLAGS="-C target-cpu=native" cargo build --release && \
    strip target/release/rust_axum

FROM debian:bullseye-slim

RUN apt-get update && apt-get install -y libpq5 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/target/release/rust_axum .

COPY --from=builder /app/.sqlx ./.sqlx

EXPOSE 8080

CMD ["./rust_axum"]
