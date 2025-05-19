FROM drogonframework/drogon:latest

WORKDIR /build
COPY . /build

RUN mkdir -p /build/build && cd /build/build && \
    cmake .. && \
    make -j$(nproc)

CMD ["/build/build/app/server"]
