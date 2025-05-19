FROM gcc:14 AS build

WORKDIR /build

RUN apt-get update && apt-get install -y \
    cmake \
    libpqxx-dev \
    nlohmann-json3-dev \
    pkg-config \
    git \
    && rm -rf /var/lib/apt/lists/*

COPY app /build/app

RUN cmake -Bbuild -S/build/app && cmake --build build --target server
