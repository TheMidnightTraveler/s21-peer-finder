FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    git \
    libssl-dev \
    libboost-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    ca-certificates \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget -q -O /usr/local/include/httplib.h \
    "https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h"

WORKDIR /app
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j$(nproc)


FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    libssl3 \
    libcurl4 \
    zlib1g \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/s21_bot .

CMD ["./s21_bot"]
