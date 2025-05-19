### СТАДИЯ 1: «билд» зависимостей
FROM rust:1.82-slim AS chef-deps
RUN apt-get update && apt-get install -y libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Подготовка «рецепта» зависимостей
# Копируем только манифесты и создаём минимальный src для cargo metadata
COPY ./app/Cargo.toml ./
RUN mkdir -p src \
    && echo "fn main() { println!(\"hello\"); }" > src/main.rs

# Генерируем recipe.json
RUN cargo install cargo-chef \
    && cargo chef prepare --recipe-path recipe.json

### СТАДИЯ 2: установка зависимостей и подготовка sqlx
FROM rust:1.82-slim AS builder
RUN apt-get update && apt-get install -y libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Кэшируем зависимости по recipe
COPY --from=chef-deps /app/recipe.json recipe.json
RUN cargo install cargo-chef \
    && cargo chef cook --release --recipe-path recipe.json

# Копируем исходники приложения и манифесты
COPY ./app/src ./src
COPY ./app/Cargo.toml ./

# Генерируем sqlx-data.json автоматически в билде
ARG DATABASE_URL
ENV DATABASE_URL=${DATABASE_URL}
RUN cargo sqlx prepare -- --lib
ENV SQLX_OFFLINE=true

# Собираем релизный бинарник с нативной оптимизацией
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
