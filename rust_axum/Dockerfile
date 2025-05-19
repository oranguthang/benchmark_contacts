### СТАДИЯ 1: «билд» зависимостей
FROM rust:1.82-slim AS chef-deps
RUN apt-get update && apt-get install -y libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Подготовка «рецепта» зависимостей
COPY ./app/Cargo.toml ./
RUN mkdir -p src \
    && echo "fn main() { println!(\"hello\"); }" > src/main.rs

# Генерируем recipe.json для кэширования зависимостей
RUN cargo install cargo-chef \
    && cargo chef prepare --recipe-path recipe.json

### СТАДИЯ 2: установка зависимостей и сборка приложения
FROM rust:1.82-slim AS builder
RUN apt-get update && apt-get install -y libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Кэшируем зависимости по recipe
COPY --from=chef-deps /app/recipe.json recipe.json
RUN cargo install cargo-chef \
    && cargo chef cook --release --recipe-path recipe.json

# Копируем весь исходный код и подготовленный sqlx-data.json
COPY ./app/src ./src
COPY ./app/.sqlx ./\.sqlx
COPY ./app/Cargo.toml ./

# Сборка релизного бинарника с нативной оптимизацией
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
ENV SQLX_OFFLINE=true

EXPOSE 8080
CMD ["./rust_axum"]
