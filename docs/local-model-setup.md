# Local Model Testing Setup

This project talks to any OpenAI-compatible `/v1/chat/completions` endpoint. Two good local options are Ollama and llama.cpp server.

---

## Ollama

### Install

```bash
curl -fsSL https://ollama.com/install.sh | sh
```

Or see [ollama.com](https://ollama.com) for platform packages.

### Pull a model

Small models (3–8B) work well for structured XML output. Granite and Llama are good choices:

```bash
ollama pull granite3.1:3b
ollama pull llama3.2
```

### Run

Ollama starts automatically on install and listens on `localhost:11434`. To start it manually:

```bash
ollama serve
```

### Point the demo at it

```bash
export OPENAI_BASE_URL=http://localhost:11434/v1
export LLM_MODEL=granite3.1:3b   # or whatever you pulled
# OPENAI_API_KEY not needed — Ollama ignores it

./build/llm_bt_demo "Pick up object A and move it to location C"
```

If Ollama is on another host (e.g. a dev server named `tonfa`):

```bash
export OPENAI_BASE_URL=http://tonfa:11434/v1
```

---

## llama.cpp Server

### Build

```bash
git clone https://github.com/ggml-org/llama.cpp
cd llama.cpp
cmake -B build -DLLAMA_CURL=ON
make -C build -j$(nproc) llama-server
```

Add `-DGGML_CUDA=ON` or `-DGGML_METAL=ON` if you have a GPU.

### Get a model

Download a GGUF model from Hugging Face. Small quantised models work well:

```bash
# Example: Llama-3.2-3B-Instruct Q8
huggingface-cli download \
  bartowski/Llama-3.2-3B-Instruct-GGUF \
  Llama-3.2-3B-Instruct-Q8_0.gguf \
  --local-dir ./models
```

### Run the server

```bash
./build/bin/llama-server \
  --model ./models/Llama-3.2-3B-Instruct-Q8_0.gguf \
  --port 8080 \
  --ctx-size 4096
```

The server exposes an OpenAI-compatible endpoint at `http://localhost:8080/v1`.

### Point the demo at it

```bash
export OPENAI_BASE_URL=http://localhost:8080/v1
export LLM_MODEL=llama-3.2-3b   # name is arbitrary for llama-server
# OPENAI_API_KEY not needed

./build/llm_bt_demo "Pick up object A and move it to location C"
```

---

## Running Integration Tests

The live LLM integration tests skip automatically when `OPENAI_BASE_URL` is not set. With a local model running:

```bash
export OPENAI_BASE_URL=http://localhost:11434/v1
export LLM_MODEL=granite3.1:3b

ctest --test-dir build -R "LLMIntegration" -V
```

Each test makes 1–3 LLM calls. Expect 10–30 seconds per test depending on model size and hardware.

---

## Tips for Better XML Output

Smaller models sometimes need a nudge. If the repair loop exhausts its 3 retries:

- Try a larger or instruction-tuned model (`llama3.1:8b`, `qwen2.5:7b`)
- Lower `temperature` is already set to `0.0` in the code — this is correct for deterministic planning
- The repair prompt shows the model its exact errors and asks it to correct them — most models get it right on the second attempt
