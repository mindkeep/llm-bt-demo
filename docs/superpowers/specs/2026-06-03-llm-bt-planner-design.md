# LLM-Directed Robot Task Planner — Design Spec

**Date:** 2026-06-03  
**Status:** Approved

---

## Overview

A simulated robot arm that receives a natural language goal, uses an LLM to decompose it into a behavior tree plan, executes it against a simple simulated environment, and visualizes the tree running in real time via Groot2.

The primary engineering challenge — and interview story — is the **prompt design and validation/repair loop**: getting an LLM to reliably emit valid BT.CPP XML, then enforcing correctness at runtime with structured feedback when it fails.

---

## Tech Stack

| Layer | Choice | Reason |
|---|---|---|
| BT core | BehaviorTree.CPP v4 | Exactly what MoveIt Pro uses |
| World sim | Plain C++ struct | Keep scope tight |
| LLM HTTP client | cpp-httplib + nlohmann/json (both header-only) | Lightweight, zero extra build deps, fully transparent |
| LLM API | OpenAI-compatible `/v1/chat/completions` | Works with Ollama, llama.cpp server, OpenAI, and any compatible endpoint |
| Visualization | Groot2 (free tier) via BT.CPP's built-in ZeroMQ publisher | Native BT.CPP tooling, zero extra code |
| Build | CMake 3.16+ with FetchContent | Matches PickNik's stack |
| Containerization | Docker | Matches PickNik's stack |
| Test framework | Google Test (gtest) via FetchContent | Widely known, clean CMake integration |

---

## Repository Layout

```
llm-bt-demo/
├── CMakeLists.txt               # root — FetchContent for BT.CPP, GTest, cpp-httplib, nlohmann/json
├── docker/
│   └── Dockerfile
├── libs/
│   ├── world_sim/               # WorldState struct + mutation helpers
│   │   ├── CMakeLists.txt
│   │   ├── include/world_sim/
│   │   └── src/
│   ├── bt_nodes/                # BT.CPP leaf node implementations
│   │   ├── CMakeLists.txt
│   │   ├── include/bt_nodes/
│   │   └── src/
│   └── llm_client/              # HTTP call + XML validation/repair
│       ├── CMakeLists.txt
│       ├── include/llm_client/
│       └── src/
├── src/
│   └── main.cpp                 # CLI entry point — wires all three libs
├── tests/
│   ├── test_world_sim.cpp
│   ├── test_bt_nodes.cpp
│   ├── test_llm_client.cpp
│   └── test_llm_integration.cpp # skipped if OPENAI_BASE_URL not set
└── docs/
    └── superpowers/specs/
```

---

## Runtime Flow

```
user input (goal string)
      │
      ▼
  BTXMLRepairAgent::get_valid_xml()
      ├─► LLMClient  ──POST /v1/chat/completions──►  Ollama / OpenAI / llama.cpp
      │       ◄── BT XML string ──────────────────
      ├─► BTXMLValidator  — checks node names against registered factory
      └─► on failure: re-prompt with error list, up to 3 retries
      │
      ▼
  BTTaskAgent::execute_goal()
      ├─► BT.CPP Factory  ──loads XML──►  BehaviorTree
      ├─► tick loop  ──reads/writes──►  WorldState
      ├─► Groot2 (ZeroMQ publisher, port 1667)
      └─► on std::exception: snapshot state, build recovery prompt, retry up to 3×
```

---

## Data Structures

### WorldState (`world_sim`)

```cpp
struct WorldState {
    enum class GripperState { Open, Closed };
    enum class Location { TableA, TableB, TableC, ArmReach, Unknown };

    struct Object {
        std::string name;
        Location location;
        bool held = false;
    };

    GripperState gripper = GripperState::Open;
    Location arm_position = Location::Unknown;
    std::array<float, 6> joint_angles = {};        // degrees, 6-DOF
    std::unordered_map<std::string, Object> objects;
};
```

The `world_sim` library owns mutation helpers (`move_arm_to`, `pick_object`, `place_object`, etc.) that enforce invariants: cannot hold two objects simultaneously, cannot place an object that is not currently held.

---

## BT Nodes (`bt_nodes`)

All nodes receive a `WorldState&` via the BT.CPP blackboard.

### Conditions

| Node | Succeeds when |
|---|---|
| `IsObjectAt(object, location)` | named object is at the given location |
| `IsGripperOpen()` | gripper state is Open |
| `IsArmNear(location)` | arm_position matches location |

### Actions

| Node | Effect |
|---|---|
| `MoveArmTo(location)` | sets arm_position; returns RUNNING on tick 1, SUCCESS on tick 2 |
| `OpenGripper()` | sets gripper = Open |
| `CloseGripper()` | sets gripper = Closed |
| `PickObject(object)` | sets object.held = true, object.location = ArmReach |
| `PlaceObject(object, location)` | sets object.held = false, object.location = given location |

All Action nodes return `RUNNING` on the first tick and `SUCCESS` on the second, simulating execution time and giving Groot2 meaningful state transitions to visualize.

### Decorators

`Retry(n)` and `Timeout(ms)` are provided by BT.CPP core — no custom implementation required.

---

## LLM Client (`llm_client`)

Three classes with distinct responsibilities:

**`LLMClient`**  
Owns the cpp-httplib connection. Sends a single POST to `/v1/chat/completions` with the system prompt and user goal. Returns the raw response string. Configuration via environment variables:

| Variable | Default | Purpose |
|---|---|---|
| `OPENAI_BASE_URL` | `http://localhost:11434/v1` | API endpoint (Ollama, llama.cpp, OpenAI, etc.) |
| `OPENAI_API_KEY` | *(empty)* | Bearer token — Ollama ignores it, OpenAI requires it |
| `LLM_MODEL` | `llama3.2` | Model name passed in the request body |

**`BTXMLValidator`**  
Parses the LLM response string using BT.CPP's own XML parser. Checks all node names against `BehaviorTreeFactory::registeredNodes()` to ensure the tree can actually be loaded. Returns a list of human-readable error strings.

**`BTXMLRepairAgent`**  
If validation fails, sends a follow-up LLM call with the original goal plus the error list injected into the prompt. Retries up to 3 times. After 3 failures throws `BTXMLParseError` with the last error list and raw XML attached.

## BT Task Agent (`bt_task_agent`)

**`BTTaskAgent`**  
Orchestrates the full execution cycle. Given a goal string and a mutable `WorldState`, it calls `BTXMLRepairAgent::get_valid_xml()`, loads the tree, ticks it to completion, and handles runtime failures:

1. Snapshots `WorldState` before each execution attempt.
2. Wraps the tick loop in a `try/catch(std::exception)`.
3. On failure (exception or FAILURE status), builds a structured recovery prompt containing: original goal, world state before the attempt, the attempted XML, the error message, and the current (partially-modified) world state.
4. Passes the recovery prompt back through `BTXMLRepairAgent` and retries.
5. After `max_retries` (default 3) exhausted, returns `false`.

`groot2_port=0` disables the ZMQ publisher, used in unit tests to avoid port binding.

---

## Prompt Design

### System Prompt (fixed, sent on every call)

```
You are a behavior tree planner for a robot arm. Output ONLY valid BT.CPP v4 XML.
Available nodes:

CONDITIONS (return SUCCESS/FAILURE, no side effects):
  <IsObjectAt object="ObjectA|ObjectB|ObjectC" location="TableA|TableB|TableC|ArmReach"/>
  <IsGripperOpen/>
  <IsArmNear location="TableA|TableB|TableC"/>

ACTIONS (return RUNNING then SUCCESS):
  <MoveArmTo location="TableA|TableB|TableC"/>
  <OpenGripper/>
  <CloseGripper/>
  <PickObject object="ObjectA|ObjectB|ObjectC"/>
  <PlaceObject object="ObjectA|ObjectB|ObjectC" location="TableA|TableB|TableC"/>

COMPOSITES (BT.CPP built-ins):
  <Sequence> <Fallback> <Parallel>

DECORATORS (BT.CPP built-ins):
  <Retry num_attempts="N"> <Timeout msec="N">

Rules:
- Output a single <root> element with one <BehaviorTree ID="Main"> child.
- No markdown, no explanation, no code fences. XML only.
```

### User Message (per request)

```
Goal: "<user-supplied goal string>"
```

### Repair Prompt (on validation failure)

```
Your previous output was invalid. Errors:
<error list from BTXMLValidator>

Corrected XML only:
```

---

## Error Handling

| Zone | Failure | Handling |
|---|---|---|
| LLM call | Network error / timeout | Throw `LLMConnectionError`; caught in `main`, print message, exit cleanly |
| XML validation | Invalid after 3 repair attempts | Throw `BTXMLParseError` with error list and raw XML; `main` prints verbatim |
| BT execution | Root node returns FAILURE | BT.CPP propagates naturally; `main` prints final tree status and exits |

No silent failures. Every error surface identifies which layer broke and what it produced.

---

## Groot2 Integration

BT.CPP v4's built-in publisher requires two lines in `main.cpp`:

```cpp
BT::Groot2Publisher publisher(tree, 1667);  // port 1667 is Groot2's default
// tick the tree normally — publisher handles everything
```

Groot2 connects to `localhost:1667` and renders the live tree with node states highlighted as it ticks. Port 1667 must be exposed in Docker.

---

## Testing Strategy

| Test file | Scope | LLM required |
|---|---|---|
| `test_world_sim.cpp` | State transitions, invariant enforcement | No |
| `test_bt_nodes.cpp` | Each node in isolation against known `WorldState` | No |
| `test_llm_client.cpp` | `BTXMLValidator` on valid/malformed XML; `BTXMLRepairAgent` with mock HTTP server | No |
| `test_llm_integration.cpp` | Full round-trip: goal string → LLM → validated XML → loaded tree | **Yes — skipped if `OPENAI_BASE_URL` unset** |

Integration tests use GTest's `GTEST_SKIP()`:

```cpp
TEST(LLMIntegration, PickAndPlace) {
    if (std::getenv("OPENAI_BASE_URL") == nullptr) {
        GTEST_SKIP() << "OPENAI_BASE_URL not set — skipping live LLM tests";
    }
    // ...
}
```

A CMake option `-DENABLE_LLM_INTEGRATION_TESTS=ON` can make the integration test suite mandatory in environments where live LLM access is guaranteed.

---

## Build Order (Suggested)

**Week 1:** BT.CPP building, all leaf nodes implemented, hardcoded XML tree running, Groot2 connected and visualizing.

**Week 2:** LLM call wired in — prompt → XML → dynamic tree load. Repair loop working.

**Week 3:** Hardening — Retry/Timeout decorators exercised, malformed LLM output handled gracefully, Docker working end-to-end.

---

## Other Considerations

**ClickHouse ai-sdk-cpp:** A modern C++20 SDK from ClickHouse engineers that natively supports OpenAI and Anthropic APIs with tool calling, streaming, and async support. Uses CMake + bundled nlohmann/json. Worth evaluating if the LLM layer grows more complex (streaming responses, tool use, multi-turn agent loops). Repository: https://github.com/ClickHouse/ai-sdk-cpp
