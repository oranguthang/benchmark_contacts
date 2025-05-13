FROM rust:1.82

WORKDIR /app

COPY ./app/Cargo.toml ./app/

RUN mkdir -p ./app/src && echo "fn main() {}" > ./app/src/main.rs

RUN cd ./app && cargo fetch

COPY ./app /app/app

RUN cd ./app && RUSTFLAGS="-C target-cpu=native" cargo build --release

CMD ["./app/target/release/rust_axum"]
