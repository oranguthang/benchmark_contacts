### STAGE 1: "build" dependencies
FROM rust:1.82-slim AS chef-deps
RUN apt-get update \
    && apt-get install -y libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy both Cargo.toml and Cargo.lock to lock dependency versions
COPY ./app/Cargo.toml ./app/Cargo.lock ./

# Stub for source files
RUN mkdir -p src \
    && echo "fn main() { println!(\"hello\"); }" > src/main.rs

# Generate recipe.json for dependency caching
RUN cargo install cargo-chef \
    && cargo chef prepare --recipe-path recipe.json

### STAGE 2: install dependencies and build application
FROM rust:1.82-slim AS builder
RUN apt-get update \
 && apt-get install -y libpq-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Cache dependencies using recipe.json
COPY --from=chef-deps /app/recipe.json ./recipe.json
RUN cargo install cargo-chef \
    && cargo chef cook --release --recipe-path recipe.json

# Copy manifests and lock file before build to use Cargo.lock
COPY ./app/Cargo.toml ./app/Cargo.lock ./

# Copy all source code
COPY ./app/src ./src

# Copy sqlx cache (if using offline)
COPY ./app/.sqlx ./.sqlx

# Build release binary with native optimization
RUN RUSTFLAGS="-C target-cpu=native" \
    cargo build --release \
    && strip target/release/rust_axum

### STAGE 3: minimal runtime on Bookworm
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
