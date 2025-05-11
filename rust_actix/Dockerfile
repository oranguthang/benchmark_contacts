FROM rust:1.69 as builder

WORKDIR /app

COPY ./app /app

RUN cargo build --release

CMD ["cargo run"]
