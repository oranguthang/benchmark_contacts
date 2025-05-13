FROM rust:1.82

WORKDIR /app

COPY ./app /app

RUN cargo build --release

CMD ["cargo run"]
