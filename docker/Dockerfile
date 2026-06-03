FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    git \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc)

EXPOSE 1667

ENTRYPOINT ["./build/llm_bt_demo"]
