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

# FetchContent pulls all deps (BT.CPP, ZeroMQ, GTest, httplib, nlohmann/json)
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc)

EXPOSE 1667

# Usage: podman run --rm -e OPENAI_BASE_URL=... -e LLM_MODEL=... -p 1667:1667 llm-bt-demo "<goal>"
ENTRYPOINT ["./build/llm_bt_demo"]
