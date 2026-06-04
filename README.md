# LLM-Directed Robot Task Planner

A simulated robot arm that receives a natural language goal, uses an LLM to decompose it into a behavior tree plan, executes the plan against a simulated environment, and visualizes the running tree in real time via [Groot2](https://www.behaviortree.dev/groot/).

```
$ ./build/llm_bt_demo
LLM Robot Task Planner  (type /exit or Ctrl-D to quit)

--- World State ---
Gripper:  Open
Arm:      Unknown
Objects:
  ObjectA  @ TableA
  ObjectB  @ TableB
  ObjectC  @ TableC
-------------------

Goal> Pick up object A and move it to location C
Sending goal to LLM: "Pick up object A and move it to location C"
Generated BT XML: ...

Executing tree... (connect Groot2 to localhost:1667)
Tree finished: SUCCESS

--- World State ---
Gripper:  Open
Arm:      TableC
Objects:
  ObjectA  @ TableC
  ObjectB  @ TableB
  ObjectC  @ TableC
-------------------

Goal> /exit
Goodbye.
```

---

## How It Works

1. You type a goal in plain English.
2. An LLM generates a BT.CPP v4 XML behavior tree.
3. A validator checks that every node name exists in the registered factory.
4. If the XML is invalid, the error is fed back to the LLM as a repair prompt (up to 3 retries).
5. The validated tree runs against a simulated world state.
6. [Groot2](https://www.behaviortree.dev/groot/) connects over ZeroMQ to visualize node states as they tick.

The LLM-to-BT validation/repair loop is the core engineering challenge — see `libs/llm_client/` for the implementation.

---

## Dependencies

All C++ dependencies are fetched automatically via CMake FetchContent. No manual installs required beyond the build tools.

| Dependency | Version | Purpose |
|---|---|---|
| [BehaviorTree.CPP](https://github.com/BehaviorTree/BehaviorTree.CPP) | 4.6.2 | Behavior tree runtime |
| [libzmq](https://github.com/zeromq/libzmq) | 4.3.5 | ZeroMQ for Groot2 publisher |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | 0.15.3 | HTTP client for LLM API |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | JSON serialization |
| [Google Test](https://github.com/google/googletest) | 1.14.0 | Unit testing |

**Build tools required:**
- CMake 3.16+
- C++20-capable compiler (GCC 12+, Clang 14+)
- Git (for FetchContent)
- libssl-dev (optional — enables HTTPS for OpenAI and other remote APIs)

---

## Build

```bash
git clone <repo-url>
cd llm-bt-demo

cmake -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j$(nproc)
```

First build fetches all dependencies (~3–5 min depending on network speed). Subsequent builds are fast.

The binary is at `build/llm_bt_demo`.

---

## Running

### Prerequisites

You need an OpenAI-compatible LLM endpoint. See **[docs/local-model-setup.md](docs/local-model-setup.md)** for Ollama and llama.cpp setup instructions.

### Environment variables

| Variable | Default | Description |
|---|---|---|
| `OPENAI_BASE_URL` | `http://localhost:11434/v1` | API endpoint (Ollama, llama.cpp, OpenAI, etc.) |
| `OPENAI_API_KEY` | *(empty)* | Bearer token — Ollama ignores it; required for OpenAI |
| `LLM_MODEL` | `granite4.1:3b-q8_0` | Model name passed in the request body |

### Interactive session (default)

```bash
export OPENAI_BASE_URL=http://localhost:11434/v1
export LLM_MODEL=granite4.1:3b-q8_0

./build/llm_bt_demo
```

Type goals at the `Goal>` prompt. The world state persists between goals, so each command builds on the previous one. Type `/exit` or press Ctrl-D to quit. Ctrl-C also exits cleanly.

### Single-shot mode

Pass the goal as an argument to run once and exit — useful for scripts and containers:

```bash
./build/llm_bt_demo "Pick up object A and move it to location C"
```

Exits with code `0` on success, `1` on error.

### Visualizing with Groot2

While a tree is executing, open [Groot2](https://www.behaviortree.dev/groot/) and connect to `localhost:1667`. Node states are highlighted in real time as the tree ticks.

---

## Container (Podman / Docker)

Build:
```bash
podman build -f docker/Dockerfile -t llm-bt-demo .
```

Run in single-shot mode (with Ollama on the host):
```bash
podman run --rm \
  -e OPENAI_BASE_URL=http://host.docker.internal:11434/v1 \
  -e LLM_MODEL=granite4.1:3b-q8_0 \
  -p 1667:1667 \
  llm-bt-demo "Pick up object A and move it to location C"
```

On Linux, `host.docker.internal` requires `--add-host=host.docker.internal:host-gateway`.

---

## Testing

```bash
make -C build -j$(nproc)
ctest --test-dir build -V
```

**35 tests** in four suites:

| Suite | Count | Requires LLM |
|---|---|---|
| `WorldState` / `WorldSim` | 13 | No |
| `Conditions` / `Actions` | 11 | No |
| `LLMClient` / `BTXMLValidator` / `BTXMLRepairAgent` | 8 | No |
| `LLMIntegration` | 3 | Yes — skipped if `OPENAI_BASE_URL` unset |

To run integration tests with a live model:
```bash
export OPENAI_BASE_URL=http://localhost:11434/v1
export LLM_MODEL=granite4.1:3b-q8_0
ctest --test-dir build -R "LLMIntegration" -V
```

See **[docs/local-model-setup.md](docs/local-model-setup.md)** for local model options.

---

## Project Structure

```
libs/
  world_sim/       WorldState struct + mutation helpers (pick, place, move, gripper)
  bt_nodes/        BT.CPP v4 condition and action nodes + node registry
  llm_client/      LLMClient, BTXMLValidator, BTXMLRepairAgent, error types
src/
  main.cpp         CLI entry point
tests/
  test_world_sim.cpp
  test_bt_nodes.cpp
  test_llm_client.cpp
  test_llm_integration.cpp
docker/
  Dockerfile
docs/
  local-model-setup.md   Ollama and llama.cpp setup
  superpowers/           Design spec and implementation plan
```

---

## Simulated World

The simulation has no physics — just state transitions:

- **Objects:** `ObjectA`, `ObjectB`, `ObjectC` start at `TableA`, `TableB`, `TableC`
- **Locations:** `TableA`, `TableB`, `TableC`, `ArmReach`
- **Gripper:** `Open` or `Closed`
- **Arm:** 6-DOF joint angles + named position

**Available BT nodes:**

| Type | Nodes |
|---|---|
| Conditions | `IsObjectAt`, `IsGripperOpen`, `IsArmNear` |
| Actions | `MoveArmTo`, `OpenGripper`, `CloseGripper`, `PickObject`, `PlaceObject` |
| Decorators | `Retry`, `Timeout` (BT.CPP built-ins) |
