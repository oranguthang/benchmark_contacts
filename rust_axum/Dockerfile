### СТАДИЯ 1: «билд» зависимостей
FROM rust:1.82-slim AS chef-deps
RUN apt-get update && apt-get install -y libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Подготовка рецепта для кэша зависимостей
COPY ./app/Cargo.toml ./
RUN apt-get update \
    && cargo install cargo-chef \
    && cargo chef prepare --recipe-path recipe.json

### СТАДИЯ 2: установка зависимостей и подготовка sqlx
FROM rust:1.82-slim AS builder
RUN apt-get update && apt-get install -y libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Копируем рецепт и готовим зависимости
COPY --from=chef-deps /app/recipe.json recipe.json
RUN cargo install cargo-chef \
    && cargo chef cook --release --recipe-path recipe.json

# Копируем исходники
COPY ./app/src ./src
COPY ./app/Cargo.toml ./

# Генерируем sqlx-data.json автоматически в билде
# Перед этим нужно передать DATABASE_URL как build-arg:
ARG DATABASE_URL
ENV DATABASE_URL=${DATABASE_URL}
RUN cargo sqlx prepare -- --lib
ENV SQLX_OFFLINE=true

# Сборка релизного бинара с нативной оптимизацией
RUN RUSTFLAGS="-C target-cpu=native" \
    cargo build --release \
    && strip target/release/rust_axum

### СТАДИЯ 3: минимальный рантайм
FROM debian:bullseye-slim AS runtime
RUN apt-get update && apt-get install -y libpq5 && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/target/release/rust_axum ./

ENV RUST_LOG=info
ENV CPU_CORES=8

EXPOSE 8080
CMD ["./rust_axum"]
