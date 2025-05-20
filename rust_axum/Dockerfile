### СТАДИЯ 1: «билд» зависимостей
FROM rust:1.82-slim AS chef-deps
RUN apt-get update \
    && apt-get install -y libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Копируем и Cargo.toml, и Cargo.lock — чтобы зафиксировать версию зависимостей
COPY ./app/Cargo.toml ./app/Cargo.lock ./

# Заглушка для исходников
RUN mkdir -p src \
    && echo "fn main() { println!(\"hello\"); }" > src/main.rs

# Генерируем recipe.json для кэширования зависимостей
RUN cargo install cargo-chef \
    && cargo chef prepare --recipe-path recipe.json

### СТАДИЯ 2: установка зависимостей и сборка приложения
FROM rust:1.82-slim AS builder
RUN apt-get update \
 && apt-get install -y libpq-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Кэшируем зависимости по recipe.json
COPY --from=chef-deps /app/recipe.json ./recipe.json
RUN cargo install cargo-chef \
    && cargo chef cook --release --recipe-path recipe.json

# Копируем манифесты и lock-файл перед сборкой — чтобы cargo build использовал Cargo.lock
COPY ./app/Cargo.toml ./app/Cargo.lock ./

# Копируем весь исходный код
COPY ./app/src ./src

# Копируем кэш sqlx (если используешь offline)
COPY ./app/.sqlx ./.sqlx

# Собираем релизный бинарник с нативной оптимизацией
RUN RUSTFLAGS="-C target-cpu=native" \
    cargo build --release \
    && strip target/release/rust_axum

### СТАДИЯ 3: минимальный рантайм на Bookworm
FROM debian:bookworm-slim AS runtime

RUN apt-get update \
    && apt-get install -y libpq5 curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/target/release/rust_axum ./

ENV RUST_LOG=info
ENV CPU_CORES=8
ENV SQLX_OFFLINE=true

EXPOSE 8080
CMD ["./rust_axum"]
