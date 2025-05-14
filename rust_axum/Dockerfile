FROM rust:1.82 AS chef
RUN apt-get update && apt-get install -y libpq-dev
WORKDIR /app
RUN cargo install cargo-chef
COPY ./app .
RUN cargo chef prepare --recipe-path recipe.json

FROM rust:1.82 AS builder
RUN apt-get update && apt-get install -y libpq-dev
WORKDIR /app
COPY --from=chef /app/recipe.json recipe.json
RUN cargo chef cook --release --recipe-path recipe.json
COPY ./app .
RUN RUSTFLAGS="-C target-cpu=native" cargo build --release && \
    strip target/release/rust_axum
CMD ["target/release/rust_axum"]
