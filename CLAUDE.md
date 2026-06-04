# CLAUDE.md — LLM-Directed Robot Task Planner

Project guidelines for AI assistants (Claude Code, etc.) working in this codebase.

---

## Project Overview

A simulated robot arm demo that accepts natural language goals, uses an LLM to generate
BehaviorTree.CPP v4 XML, validates/repairs it, executes it against a simulated world,
and recovers from runtime failures by feeding error context back to the LLM.

Target audience: PickNik / MoveIt Pro portfolio. Reflects real MoveIt Pro BT.CPP patterns.

---

## Library Structure

Dependencies flow strictly downward — never introduce circular dependencies.

```
llm_bt_demo (executable)
    └── bt_nodes_lib
    └── bt_task_agent_lib
            ├── world_sim_lib       (WorldState, WorldSim mutations)
            ├── llm_client_lib      (LLMClient, BTXMLValidator, BTXMLRepairAgent)
            └── behaviortree_cpp    (BT.CPP v4 — FetchContent dep)
    bt_nodes_lib
            ├── world_sim_lib
            └── behaviortree_cpp
    llm_client_lib
            └── behaviortree_cpp
```

`main.cpp` is pure orchestration: setup, REPL loop, display. Logic belongs in libs.

---

## C++20 Practices

**Use these:**
- Structured bindings: `for (const auto& [name, obj] : ws.objects)`
- `std::string_view` for read-only string parameters
- `[[nodiscard]]` on functions whose return value must not be silently dropped
- `constexpr` for compile-time constants (`static constexpr int MAX_RETRIES = 3`)
- Range-based for loops over index loops
- `std::unique_ptr` / `std::make_unique` — no raw `new`/`delete`
- Designated initialisers where they improve clarity
- `if constexpr` for compile-time branching

**Avoid:**
- Raw owning pointers
- `reinterpret_cast` / `const_cast` without a documented reason
- `std::endl` (flushes unnecessarily — use `"\n"`)
- Unsigned arithmetic where signed is correct
- `assert()` for recoverable errors — throw or return error status instead

---

## Code Style

- **No comments on obvious code.** Only comment WHY something is non-obvious: a
  hidden constraint, a workaround, an invariant that would surprise a reader.
- **Short functions.** If a function needs a comment explaining what it does,
  consider splitting it.
- **RAII everywhere.** Resources (ZMQ sockets, BT publishers) live in RAII wrappers.
  Never manually close/release if a destructor can do it.
- **Single lookup pattern.** `auto& obj = ws.objects.at(name)` — don't look up the
  same key twice.
- **Error handling at boundaries.** Validate at system entry points (user input, LLM
  responses). Trust internal invariants. Don't add defensive checks inside well-tested
  helpers.

---

## Testing

- Unit tests live in `tests/` using Google Test.
- Integration tests (require `OPENAI_BASE_URL`) use `GTEST_SKIP()` in `SetUp()` so
  they skip cleanly without being marked as failures.
- Use `FakeLLMClient` (subclass of `LLMClient` with a `responses` vector) to unit-test
  anything that touches the LLM.
- Pass `groot2_port=0` to `BTTaskAgent` in tests to disable the ZMQ publisher.
- All `gtest_discover_tests` calls must include `DISCOVERY_MODE PRE_TEST` (cmake 4.x
  compat — avoids a build-time race condition with parallel `make -j`).

Run tests:
```bash
ctest --test-dir build -V
```

---

## BT.CPP v4 Patterns

- Actions extend `BT::StatefulActionNode`: `onStart()` returns RUNNING, `onRunning()`
  applies the mutation and returns SUCCESS.
- Conditions extend `BT::ConditionNode`: `tick()` reads world state, no side effects.
- Always use `getInput<T>("port_name")` and check the result before using the value.
- Blackboard access: `node.config().blackboard->get<WorldState*>("world_state")`.
- Nodes are registered via `register_all_nodes(factory)` before any tree is created.

---

## LLM / XML Repair Flow

```
User goal
  → BTXMLRepairAgent::get_valid_xml()   [up to 3 XML-parse retries]
      → LLMClient::complete()
      → BTXMLValidator::validate()
  → BTTaskAgent::execute_goal()         [up to 3 runtime-error retries]
      → BT::Tree::tickOnce() loop
      → on std::exception: build_recovery_prompt() → retry
```

`BTXMLRepairAgent` handles bad XML (parse/validate failures).
`BTTaskAgent` handles bad execution (runtime exceptions, FAILURE status).

---

## Build Notes

- All C++ deps fetched via CMake FetchContent — no system installs needed beyond
  `cmake`, `build-essential`, `git`, optionally `gnutls-devel` (WSS support in ZMQ).
- BT.CPP 4.6.2 requires two patches (applied via `PATCH_COMMAND`):
  - `patches/fix_lexy_typo.cmake` — typo + cmake_minimum_required bump
  - `patches/fix_conan_zmq.cmake` — prefer FetchContent ZMQ over conan
- libzmq requires `patches/fix_zmq_cmake_min.cmake` for the same cmake_minimum_required reason.
- `target_compile_options(behaviortree_cpp PRIVATE -w)` suppresses GCC 16 false
  positives in lexy template instantiations — do not remove.

---

## Ollama / Local LLM

Configure via environment variables only. Never hardcode endpoints or model names.

```bash
export OPENAI_BASE_URL=http://tonfa:11434/v1
export LLM_MODEL=granite4.1:3b-q8_0
```

See `docs/local-model-setup.md` for Ollama and llama.cpp instructions.
