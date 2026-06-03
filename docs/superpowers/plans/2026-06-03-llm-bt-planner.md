# LLM-Directed Robot Task Planner — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a C++20 CLI that takes a natural language robot goal, calls an OpenAI-compatible LLM to generate BT.CPP v4 XML, executes the behavior tree against a simulated world state, and streams live node status to Groot2.

**Architecture:** Three static C++ libraries (`world_sim`, `bt_nodes`, `llm_client`) linked into a single main executable. `llm_client` owns HTTP I/O, XML validation, and a retry-based repair loop. BT.CPP's built-in `Groot2Publisher` streams tree state over ZeroMQ with zero extra code.

**Tech Stack:** C++20, BehaviorTree.CPP v4, Google Test, cpp-httplib, nlohmann/json, CMake 3.16+ FetchContent, Docker, Groot2

---

## File Map

```
llm-bt-demo/
├── CMakeLists.txt
├── .gitignore
├── docker/
│   └── Dockerfile
├── libs/
│   ├── world_sim/
│   │   ├── CMakeLists.txt
│   │   ├── include/world_sim/world_state.hpp   # WorldState struct, enums
│   │   ├── include/world_sim/world_sim.hpp     # mutation helper declarations
│   │   └── src/world_sim.cpp                   # mutation helper implementations
│   ├── bt_nodes/
│   │   ├── CMakeLists.txt
│   │   ├── include/bt_nodes/conditions.hpp     # IsObjectAt, IsGripperOpen, IsArmNear
│   │   ├── include/bt_nodes/actions.hpp        # MoveArmTo, OpenGripper, CloseGripper, PickObject, PlaceObject
│   │   ├── include/bt_nodes/registry.hpp       # register_all_nodes() declaration
│   │   ├── src/conditions.cpp
│   │   ├── src/actions.cpp
│   │   └── src/registry.cpp
│   └── llm_client/
│       ├── CMakeLists.txt
│       ├── include/llm_client/errors.hpp           # LLMConnectionError, BTXMLParseError
│       ├── include/llm_client/llm_client.hpp       # LLMClient class
│       ├── include/llm_client/bt_xml_validator.hpp # BTXMLValidator class
│       ├── include/llm_client/bt_xml_repair_agent.hpp # BTXMLRepairAgent class
│       ├── src/llm_client.cpp
│       ├── src/bt_xml_validator.cpp
│       └── src/bt_xml_repair_agent.cpp
├── src/
│   └── main.cpp
└── tests/
    ├── CMakeLists.txt
    ├── test_world_sim.cpp
    ├── test_bt_nodes.cpp
    ├── test_llm_client.cpp
    └── test_llm_integration.cpp
```

---

## Task 1: CMake Scaffold and Build Verification

**Files:**
- Create: `CMakeLists.txt`
- Create: `libs/world_sim/CMakeLists.txt`
- Create: `libs/bt_nodes/CMakeLists.txt`
- Create: `libs/llm_client/CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `src/main.cpp` (stub)

- [ ] **Step 1: Create root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(llm_bt_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# BehaviorTree.CPP v4
FetchContent_Declare(
    behaviortree_cpp
    GIT_REPOSITORY https://github.com/BehaviorTree/BehaviorTree.CPP.git
    GIT_TAG        4.6
)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# cpp-httplib (header-only)
FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)

# nlohmann/json (header-only)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(behaviortree_cpp googletest cpp_httplib nlohmann_json)

add_subdirectory(libs/world_sim)
add_subdirectory(libs/bt_nodes)
add_subdirectory(libs/llm_client)

add_executable(llm_bt_demo src/main.cpp)
target_link_libraries(llm_bt_demo PRIVATE
    world_sim_lib
    bt_nodes_lib
    llm_client_lib
    behaviortree_cpp
)
target_include_directories(llm_bt_demo PRIVATE src)

enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 2: Create stub library CMakeLists.txt files**

`libs/world_sim/CMakeLists.txt`:
```cmake
add_library(world_sim_lib STATIC src/world_sim.cpp)
target_include_directories(world_sim_lib PUBLIC include)
```

`libs/bt_nodes/CMakeLists.txt`:
```cmake
add_library(bt_nodes_lib STATIC
    src/conditions.cpp
    src/actions.cpp
    src/registry.cpp
)
target_include_directories(bt_nodes_lib PUBLIC include)
target_link_libraries(bt_nodes_lib PUBLIC
    world_sim_lib
    behaviortree_cpp
)
```

`libs/llm_client/CMakeLists.txt`:
```cmake
add_library(llm_client_lib STATIC
    src/llm_client.cpp
    src/bt_xml_validator.cpp
    src/bt_xml_repair_agent.cpp
)
target_include_directories(llm_client_lib PUBLIC include)

find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_compile_definitions(llm_client_lib PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(llm_client_lib PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

target_link_libraries(llm_client_lib PUBLIC
    behaviortree_cpp
    httplib::httplib
    nlohmann_json::nlohmann_json
)
```

`tests/CMakeLists.txt`:
```cmake
include(GoogleTest)

add_executable(test_world_sim test_world_sim.cpp)
target_link_libraries(test_world_sim PRIVATE world_sim_lib GTest::gtest_main)
gtest_discover_tests(test_world_sim)

add_executable(test_bt_nodes test_bt_nodes.cpp)
target_link_libraries(test_bt_nodes PRIVATE bt_nodes_lib GTest::gtest_main)
gtest_discover_tests(test_bt_nodes)

add_executable(test_llm_client test_llm_client.cpp)
target_link_libraries(test_llm_client PRIVATE llm_client_lib bt_nodes_lib GTest::gtest_main)
gtest_discover_tests(test_llm_client)

add_executable(test_llm_integration test_llm_integration.cpp)
target_link_libraries(test_llm_integration PRIVATE llm_client_lib bt_nodes_lib GTest::gtest_main)
gtest_discover_tests(test_llm_integration)
```

- [ ] **Step 3: Create all stub source files so CMake can compile**

`src/main.cpp`:
```cpp
#include <iostream>
int main() { std::cout << "llm-bt-demo\n"; return 0; }
```

Create empty stub `.cpp` files (they will be filled in subsequent tasks):
```bash
mkdir -p libs/world_sim/src libs/world_sim/include/world_sim
mkdir -p libs/bt_nodes/src libs/bt_nodes/include/bt_nodes
mkdir -p libs/llm_client/src libs/llm_client/include/llm_client
mkdir -p tests
touch libs/world_sim/src/world_sim.cpp
touch libs/bt_nodes/src/conditions.cpp libs/bt_nodes/src/actions.cpp libs/bt_nodes/src/registry.cpp
touch libs/llm_client/src/llm_client.cpp libs/llm_client/src/bt_xml_validator.cpp libs/llm_client/src/bt_xml_repair_agent.cpp
touch tests/test_world_sim.cpp tests/test_bt_nodes.cpp tests/test_llm_client.cpp tests/test_llm_integration.cpp
```

- [ ] **Step 4: Build and verify it compiles**

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Expected: build succeeds, `build/llm_bt_demo` exists. FetchContent will clone all four deps on first run (~2 min).

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt libs/ src/ tests/
git commit -m "feat: cmake scaffold with all dependencies wired"
```

---

## Task 2: WorldState Data Structure

**Files:**
- Create: `libs/world_sim/include/world_sim/world_state.hpp`
- Modify: `tests/test_world_sim.cpp`

- [ ] **Step 1: Write the failing test**

`tests/test_world_sim.cpp`:
```cpp
#include <gtest/gtest.h>
#include "world_sim/world_state.hpp"

TEST(WorldState, DefaultGripperIsOpen) {
    WorldState ws;
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Open);
}

TEST(WorldState, DefaultArmPositionIsUnknown) {
    WorldState ws;
    EXPECT_EQ(ws.arm_position, WorldState::Location::Unknown);
}

TEST(WorldState, DefaultJointAnglesAreZero) {
    WorldState ws;
    for (float angle : ws.joint_angles) {
        EXPECT_FLOAT_EQ(angle, 0.0f);
    }
}

TEST(WorldState, ObjectsMapIsEmptyByDefault) {
    WorldState ws;
    EXPECT_TRUE(ws.objects.empty());
}

TEST(WorldState, ObjectCanBeAddedAndRetrieved) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    ASSERT_TRUE(ws.objects.count("ObjectA"));
    EXPECT_EQ(ws.objects.at("ObjectA").location, WorldState::Location::TableA);
    EXPECT_FALSE(ws.objects.at("ObjectA").held);
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_world_sim -V
```

Expected: compilation error — `world_state.hpp` not found.

- [ ] **Step 3: Implement WorldState**

`libs/world_sim/include/world_sim/world_state.hpp`:
```cpp
#pragma once
#include <array>
#include <string>
#include <unordered_map>

struct WorldState {
    enum class GripperState { Open, Closed };
    enum class Location { TableA, TableB, TableC, ArmReach, Unknown };

    struct Object {
        std::string name;
        Location location = Location::Unknown;
        bool held = false;
    };

    GripperState gripper = GripperState::Open;
    Location arm_position = Location::Unknown;
    std::array<float, 6> joint_angles = {};
    std::unordered_map<std::string, Object> objects;
};
```

- [ ] **Step 4: Run tests to verify they pass**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_world_sim -V
```

Expected: 5 tests pass.

- [ ] **Step 5: Commit**

```bash
git add libs/world_sim/include/world_sim/world_state.hpp tests/test_world_sim.cpp
git commit -m "feat: WorldState data structure with tests"
```

---

## Task 3: WorldSim Mutation Helpers

**Files:**
- Create: `libs/world_sim/include/world_sim/world_sim.hpp`
- Modify: `libs/world_sim/src/world_sim.cpp`
- Modify: `tests/test_world_sim.cpp`

- [ ] **Step 1: Write the failing tests (append to test_world_sim.cpp)**

```cpp
#include "world_sim/world_sim.hpp"

TEST(WorldSim, MoveArmUpdatesPosition) {
    WorldState ws;
    WorldSim::move_arm_to(ws, WorldState::Location::TableA);
    EXPECT_EQ(ws.arm_position, WorldState::Location::TableA);
}

TEST(WorldSim, OpenGripperSetsState) {
    WorldState ws;
    ws.gripper = WorldState::GripperState::Closed;
    WorldSim::open_gripper(ws);
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Open);
}

TEST(WorldSim, CloseGripperSetsState) {
    WorldState ws;
    WorldSim::close_gripper(ws);
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Closed);
}

TEST(WorldSim, PickObjectSetsHeldAndLocation) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    ws.arm_position = WorldState::Location::TableA;
    ws.gripper = WorldState::GripperState::Open;

    WorldSim::pick_object(ws, "ObjectA");

    EXPECT_TRUE(ws.objects.at("ObjectA").held);
    EXPECT_EQ(ws.objects.at("ObjectA").location, WorldState::Location::ArmReach);
}

TEST(WorldSim, PickObjectThrowsIfAlreadyHoldingOne) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, true};
    ws.objects["ObjectB"] = {"ObjectB", WorldState::Location::TableA, false};

    EXPECT_THROW(WorldSim::pick_object(ws, "ObjectB"), std::logic_error);
}

TEST(WorldSim, PlaceObjectUpdatesLocationAndClearsHeld) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::ArmReach, true};

    WorldSim::place_object(ws, "ObjectA", WorldState::Location::TableC);

    EXPECT_FALSE(ws.objects.at("ObjectA").held);
    EXPECT_EQ(ws.objects.at("ObjectA").location, WorldState::Location::TableC);
}

TEST(WorldSim, PlaceObjectThrowsIfNotHeld) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};

    EXPECT_THROW(WorldSim::place_object(ws, "ObjectA", WorldState::Location::TableC), std::logic_error);
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_world_sim -V
```

Expected: compilation error — `world_sim.hpp` not found.

- [ ] **Step 3: Create world_sim.hpp**

`libs/world_sim/include/world_sim/world_sim.hpp`:
```cpp
#pragma once
#include <stdexcept>
#include <string>
#include "world_sim/world_state.hpp"

namespace WorldSim {
    void move_arm_to(WorldState& ws, WorldState::Location location);
    void open_gripper(WorldState& ws);
    void close_gripper(WorldState& ws);
    void pick_object(WorldState& ws, const std::string& object_name);
    void place_object(WorldState& ws, const std::string& object_name, WorldState::Location location);
}
```

- [ ] **Step 4: Implement world_sim.cpp**

`libs/world_sim/src/world_sim.cpp`:
```cpp
#include "world_sim/world_sim.hpp"
#include <algorithm>

namespace WorldSim {

void move_arm_to(WorldState& ws, WorldState::Location location) {
    ws.arm_position = location;
}

void open_gripper(WorldState& ws) {
    ws.gripper = WorldState::GripperState::Open;
}

void close_gripper(WorldState& ws) {
    ws.gripper = WorldState::GripperState::Closed;
}

void pick_object(WorldState& ws, const std::string& name) {
    bool already_holding = std::any_of(ws.objects.begin(), ws.objects.end(),
        [](const auto& kv) { return kv.second.held; });
    if (already_holding) {
        throw std::logic_error("Cannot pick object: already holding one");
    }
    ws.objects.at(name).held = true;
    ws.objects.at(name).location = WorldState::Location::ArmReach;
}

void place_object(WorldState& ws, const std::string& name, WorldState::Location location) {
    if (!ws.objects.at(name).held) {
        throw std::logic_error("Cannot place object: not currently held");
    }
    ws.objects.at(name).held = false;
    ws.objects.at(name).location = location;
}

} // namespace WorldSim
```

- [ ] **Step 5: Run tests to verify they pass**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_world_sim -V
```

Expected: all 11 world_sim tests pass.

- [ ] **Step 6: Commit**

```bash
git add libs/world_sim/ tests/test_world_sim.cpp
git commit -m "feat: WorldSim mutation helpers with invariant enforcement"
```

---

## Task 4: BT Condition Nodes

**Files:**
- Create: `libs/bt_nodes/include/bt_nodes/conditions.hpp`
- Modify: `libs/bt_nodes/src/conditions.cpp`
- Modify: `tests/test_bt_nodes.cpp`

Each condition node receives a `WorldState*` from the blackboard key `"world_state"`.

- [ ] **Step 1: Write the failing tests**

`tests/test_bt_nodes.cpp`:
```cpp
#include <gtest/gtest.h>
#include "behaviortree_cpp/bt_factory.h"
#include "bt_nodes/conditions.hpp"
#include "bt_nodes/registry.hpp"
#include "world_sim/world_state.hpp"

// Helper: build a single-node tree and tick it once
static BT::NodeStatus tick_tree(const std::string& xml, WorldState& ws) {
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);
    return tree.tickOnce();
}

static const char* XML_IS_GRIPPER_OPEN = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T"><IsGripperOpen/></BehaviorTree>
</root>)";

TEST(Conditions, IsGripperOpenSucceedsWhenOpen) {
    WorldState ws;
    ws.gripper = WorldState::GripperState::Open;
    EXPECT_EQ(tick_tree(XML_IS_GRIPPER_OPEN, ws), BT::NodeStatus::SUCCESS);
}

TEST(Conditions, IsGripperOpenFailsWhenClosed) {
    WorldState ws;
    ws.gripper = WorldState::GripperState::Closed;
    EXPECT_EQ(tick_tree(XML_IS_GRIPPER_OPEN, ws), BT::NodeStatus::FAILURE);
}

TEST(Conditions, IsObjectAtSucceeds) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <IsObjectAt object="ObjectA" location="TableA"/>
  </BehaviorTree>
</root>)";
    EXPECT_EQ(tick_tree(xml, ws), BT::NodeStatus::SUCCESS);
}

TEST(Conditions, IsObjectAtFailsWrongLocation) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableB, false};
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <IsObjectAt object="ObjectA" location="TableA"/>
  </BehaviorTree>
</root>)";
    EXPECT_EQ(tick_tree(xml, ws), BT::NodeStatus::FAILURE);
}

TEST(Conditions, IsArmNearSucceeds) {
    WorldState ws;
    ws.arm_position = WorldState::Location::TableC;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <IsArmNear location="TableC"/>
  </BehaviorTree>
</root>)";
    EXPECT_EQ(tick_tree(xml, ws), BT::NodeStatus::SUCCESS);
}

TEST(Conditions, IsArmNearFails) {
    WorldState ws;
    ws.arm_position = WorldState::Location::TableA;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <IsArmNear location="TableC"/>
  </BehaviorTree>
</root>)";
    EXPECT_EQ(tick_tree(xml, ws), BT::NodeStatus::FAILURE);
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_bt_nodes -V
```

Expected: compilation error — `conditions.hpp` and `registry.hpp` not found.

- [ ] **Step 3: Create conditions.hpp**

`libs/bt_nodes/include/bt_nodes/conditions.hpp`:
```cpp
#pragma once
#include "behaviortree_cpp/condition_node.h"
#include "world_sim/world_state.hpp"

class IsGripperOpen : public BT::ConditionNode {
public:
    IsGripperOpen(const std::string& name, const BT::NodeConfig& config)
        : BT::ConditionNode(name, config) {}
    BT::NodeStatus tick() override;
    static BT::PortsList providedPorts() { return {}; }
};

class IsObjectAt : public BT::ConditionNode {
public:
    IsObjectAt(const std::string& name, const BT::NodeConfig& config)
        : BT::ConditionNode(name, config) {}
    BT::NodeStatus tick() override;
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("object"),
                 BT::InputPort<std::string>("location") };
    }
};

class IsArmNear : public BT::ConditionNode {
public:
    IsArmNear(const std::string& name, const BT::NodeConfig& config)
        : BT::ConditionNode(name, config) {}
    BT::NodeStatus tick() override;
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("location") };
    }
};
```

- [ ] **Step 4: Create a stub registry.hpp** (actions are added in Task 5)

`libs/bt_nodes/include/bt_nodes/registry.hpp`:
```cpp
#pragma once
#include "behaviortree_cpp/bt_factory.h"

void register_all_nodes(BT::BehaviorTreeFactory& factory);
```

- [ ] **Step 5: Implement conditions.cpp**

`libs/bt_nodes/src/conditions.cpp`:
```cpp
#include "bt_nodes/conditions.hpp"

static WorldState* get_world(const BT::TreeNode& node) {
    return node.config().blackboard->get<WorldState*>("world_state");
}

static WorldState::Location parse_location(const std::string& s) {
    if (s == "TableA")   return WorldState::Location::TableA;
    if (s == "TableB")   return WorldState::Location::TableB;
    if (s == "TableC")   return WorldState::Location::TableC;
    if (s == "ArmReach") return WorldState::Location::ArmReach;
    return WorldState::Location::Unknown;
}

BT::NodeStatus IsGripperOpen::tick() {
    auto* ws = get_world(*this);
    return ws->gripper == WorldState::GripperState::Open
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

BT::NodeStatus IsObjectAt::tick() {
    auto object_name = getInput<std::string>("object").value();
    auto location_str = getInput<std::string>("location").value();
    auto* ws = get_world(*this);
    auto it = ws->objects.find(object_name);
    if (it == ws->objects.end()) return BT::NodeStatus::FAILURE;
    return it->second.location == parse_location(location_str)
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

BT::NodeStatus IsArmNear::tick() {
    auto location_str = getInput<std::string>("location").value();
    auto* ws = get_world(*this);
    return ws->arm_position == parse_location(location_str)
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}
```

- [ ] **Step 6: Implement a stub registry.cpp** (full registration in Task 5)

`libs/bt_nodes/src/registry.cpp`:
```cpp
#include "bt_nodes/registry.hpp"
#include "bt_nodes/conditions.hpp"
#include "bt_nodes/actions.hpp"

void register_all_nodes(BT::BehaviorTreeFactory& factory) {
    factory.registerNodeType<IsGripperOpen>("IsGripperOpen");
    factory.registerNodeType<IsObjectAt>("IsObjectAt");
    factory.registerNodeType<IsArmNear>("IsArmNear");
    // Actions registered in Task 5
}
```

- [ ] **Step 7: Create a stub actions.hpp** so the registry compiles

`libs/bt_nodes/include/bt_nodes/actions.hpp`:
```cpp
#pragma once
// Action nodes declared in Task 5
```

- [ ] **Step 8: Run tests to verify they pass**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_bt_nodes -V
```

Expected: 6 condition tests pass.

- [ ] **Step 9: Commit**

```bash
git add libs/bt_nodes/ tests/test_bt_nodes.cpp
git commit -m "feat: BT condition nodes with tests"
```

---

## Task 5: BT Action Nodes

**Files:**
- Modify: `libs/bt_nodes/include/bt_nodes/actions.hpp`
- Create: `libs/bt_nodes/src/actions.cpp`
- Modify: `libs/bt_nodes/src/registry.cpp`
- Modify: `tests/test_bt_nodes.cpp`

Action nodes use `BT::StatefulActionNode`: `onStart()` returns `RUNNING`, `onRunning()` returns `SUCCESS`. This simulates execution time and gives Groot2 visible state transitions.

- [ ] **Step 1: Write the failing tests (append to test_bt_nodes.cpp)**

```cpp
TEST(Actions, MoveArmToReturnsRunningThenSuccess) {
    WorldState ws;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <MoveArmTo location="TableB"/>
  </BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    EXPECT_EQ(tree.tickOnce(), BT::NodeStatus::RUNNING);
    EXPECT_EQ(tree.tickOnce(), BT::NodeStatus::SUCCESS);
    EXPECT_EQ(ws.arm_position, WorldState::Location::TableB);
}

TEST(Actions, OpenGripperSucceeds) {
    WorldState ws;
    ws.gripper = WorldState::GripperState::Closed;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T"><OpenGripper/></BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    tree.tickOnce(); tree.tickOnce();
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Open);
}

TEST(Actions, CloseGripperSucceeds) {
    WorldState ws;
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T"><CloseGripper/></BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    tree.tickOnce(); tree.tickOnce();
    EXPECT_EQ(ws.gripper, WorldState::GripperState::Closed);
}

TEST(Actions, PickObjectSetsHeld) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <PickObject object="ObjectA"/>
  </BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    tree.tickOnce(); tree.tickOnce();
    EXPECT_TRUE(ws.objects.at("ObjectA").held);
}

TEST(Actions, PlaceObjectUpdatesLocation) {
    WorldState ws;
    ws.objects["ObjectA"] = {"ObjectA", WorldState::Location::ArmReach, true};
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="T">
    <PlaceObject object="ObjectA" location="TableC"/>
  </BehaviorTree>
</root>)";
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);
    auto bb = BT::Blackboard::create();
    bb->set("world_state", &ws);
    auto tree = factory.createTreeFromText(xml, bb);

    tree.tickOnce(); tree.tickOnce();
    EXPECT_EQ(ws.objects.at("ObjectA").location, WorldState::Location::TableC);
    EXPECT_FALSE(ws.objects.at("ObjectA").held);
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_bt_nodes -V
```

Expected: compilation error — action classes not yet declared in actions.hpp.

- [ ] **Step 3: Implement actions.hpp**

`libs/bt_nodes/include/bt_nodes/actions.hpp`:
```cpp
#pragma once
#include "behaviortree_cpp/action_node.h"
#include "world_sim/world_state.hpp"

class MoveArmTo : public BT::StatefulActionNode {
public:
    MoveArmTo(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("location") };
    }
};

class OpenGripper : public BT::StatefulActionNode {
public:
    OpenGripper(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() { return {}; }
};

class CloseGripper : public BT::StatefulActionNode {
public:
    CloseGripper(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() { return {}; }
};

class PickObject : public BT::StatefulActionNode {
public:
    PickObject(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("object") };
    }
};

class PlaceObject : public BT::StatefulActionNode {
public:
    PlaceObject(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override {}
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("object"),
                 BT::InputPort<std::string>("location") };
    }
};
```

- [ ] **Step 4: Implement actions.cpp**

`libs/bt_nodes/src/actions.cpp`:
```cpp
#include "bt_nodes/actions.hpp"
#include "world_sim/world_sim.hpp"

static WorldState* get_world(const BT::TreeNode& node) {
    return node.config().blackboard->get<WorldState*>("world_state");
}

static WorldState::Location parse_location(const std::string& s) {
    if (s == "TableA")   return WorldState::Location::TableA;
    if (s == "TableB")   return WorldState::Location::TableB;
    if (s == "TableC")   return WorldState::Location::TableC;
    if (s == "ArmReach") return WorldState::Location::ArmReach;
    return WorldState::Location::Unknown;
}

// MoveArmTo
BT::NodeStatus MoveArmTo::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus MoveArmTo::onRunning() {
    auto loc = parse_location(getInput<std::string>("location").value());
    WorldSim::move_arm_to(*get_world(*this), loc);
    return BT::NodeStatus::SUCCESS;
}

// OpenGripper
BT::NodeStatus OpenGripper::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus OpenGripper::onRunning() {
    WorldSim::open_gripper(*get_world(*this));
    return BT::NodeStatus::SUCCESS;
}

// CloseGripper
BT::NodeStatus CloseGripper::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus CloseGripper::onRunning() {
    WorldSim::close_gripper(*get_world(*this));
    return BT::NodeStatus::SUCCESS;
}

// PickObject
BT::NodeStatus PickObject::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus PickObject::onRunning() {
    WorldSim::pick_object(*get_world(*this), getInput<std::string>("object").value());
    return BT::NodeStatus::SUCCESS;
}

// PlaceObject
BT::NodeStatus PlaceObject::onStart() { return BT::NodeStatus::RUNNING; }
BT::NodeStatus PlaceObject::onRunning() {
    auto loc = parse_location(getInput<std::string>("location").value());
    WorldSim::place_object(*get_world(*this), getInput<std::string>("object").value(), loc);
    return BT::NodeStatus::SUCCESS;
}
```

- [ ] **Step 5: Complete registry.cpp**

`libs/bt_nodes/src/registry.cpp`:
```cpp
#include "bt_nodes/registry.hpp"
#include "bt_nodes/conditions.hpp"
#include "bt_nodes/actions.hpp"

void register_all_nodes(BT::BehaviorTreeFactory& factory) {
    factory.registerNodeType<IsGripperOpen>("IsGripperOpen");
    factory.registerNodeType<IsObjectAt>("IsObjectAt");
    factory.registerNodeType<IsArmNear>("IsArmNear");
    factory.registerNodeType<MoveArmTo>("MoveArmTo");
    factory.registerNodeType<OpenGripper>("OpenGripper");
    factory.registerNodeType<CloseGripper>("CloseGripper");
    factory.registerNodeType<PickObject>("PickObject");
    factory.registerNodeType<PlaceObject>("PlaceObject");
}
```

- [ ] **Step 6: Run tests to verify they pass**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_bt_nodes -V
```

Expected: all 11 bt_node tests pass.

- [ ] **Step 7: Commit**

```bash
git add libs/bt_nodes/ tests/test_bt_nodes.cpp
git commit -m "feat: BT action nodes with RUNNING/SUCCESS two-tick pattern"
```

---

## Task 6: Hardcoded XML Tree Running End-to-End

Wire up `main.cpp` with a hardcoded tree to verify the full BT execution pipeline before introducing the LLM.

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace stub main.cpp with hardcoded tree**

`src/main.cpp`:
```cpp
#include <iostream>
#include <thread>
#include <chrono>

#include "behaviortree_cpp/bt_factory.h"
#include "bt_nodes/registry.hpp"
#include "world_sim/world_state.hpp"

static const char* HARDCODED_XML = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Sequence>
      <MoveArmTo location="TableA"/>
      <OpenGripper/>
      <PickObject object="ObjectA"/>
      <CloseGripper/>
      <MoveArmTo location="TableC"/>
      <OpenGripper/>
      <PlaceObject object="ObjectA" location="TableC"/>
    </Sequence>
  </BehaviorTree>
</root>)";

int main() {
    WorldState world;
    world.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    world.objects["ObjectB"] = {"ObjectB", WorldState::Location::TableB, false};
    world.objects["ObjectC"] = {"ObjectC", WorldState::Location::TableC, false};

    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);

    auto blackboard = BT::Blackboard::create();
    blackboard->set("world_state", &world);

    auto tree = factory.createTreeFromText(HARDCODED_XML, blackboard);

    std::cout << "Executing hardcoded pick-and-place tree...\n";

    BT::NodeStatus status = BT::NodeStatus::RUNNING;
    while (status == BT::NodeStatus::RUNNING) {
        status = tree.tickOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Tree finished with status: "
              << (status == BT::NodeStatus::SUCCESS ? "SUCCESS" : "FAILURE") << "\n";
    std::cout << "ObjectA is now at: "
              << (world.objects.at("ObjectA").location == WorldState::Location::TableC
                  ? "TableC" : "elsewhere") << "\n";
    return 0;
}
```

- [ ] **Step 2: Build and run**

```bash
cmake --build build -j$(nproc) && ./build/llm_bt_demo
```

Expected output:
```
Executing hardcoded pick-and-place tree...
Tree finished with status: SUCCESS
ObjectA is now at: TableC
```

- [ ] **Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "feat: hardcoded XML tree executing end-to-end"
```

---

## Task 7: Groot2 Publisher Integration

**Files:**
- Modify: `src/main.cpp`

Groot2 connects over ZeroMQ to port 1667. BT.CPP's `Groot2Publisher` is available automatically when BT.CPP detects ZeroMQ at build time. If ZeroMQ is not installed, this task is skipped (tree still runs, just without visualization).

- [ ] **Step 1: Install ZeroMQ (if not present)**

```bash
# Ubuntu/Debian:
sudo apt-get install -y libzmq3-dev
# Then re-run cmake to pick it up:
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

- [ ] **Step 2: Add Groot2Publisher to main.cpp**

In `src/main.cpp`, add the include and publisher after the tree is created:

```cpp
// Add near other includes:
#include "behaviortree_cpp/groot2_publisher.h"

// Add after factory.createTreeFromText(...):
BT::Groot2Publisher publisher(tree, 1667);
std::cout << "Groot2 publisher active on port 1667.\n";
std::cout << "Open Groot2 and connect to localhost:1667 to visualize.\n";
```

- [ ] **Step 3: Build and manually verify Groot2 connection**

```bash
cmake --build build -j$(nproc) && ./build/llm_bt_demo
```

Open Groot2 → Connect → `localhost:1667`. You should see the tree with nodes highlighted as they execute.

If `Groot2Publisher` is not found (ZeroMQ missing), BT.CPP will omit it silently — the binary still runs. Check BT.CPP CMake output for `BTCPP_GROOT2_PUBLISHER` to confirm it was enabled.

- [ ] **Step 4: Commit**

```bash
git add src/main.cpp
git commit -m "feat: Groot2Publisher on port 1667 for live tree visualization"
```

---

## Task 8: LLMClient HTTP Layer

**Files:**
- Modify: `libs/llm_client/include/llm_client/errors.hpp`
- Modify: `libs/llm_client/include/llm_client/llm_client.hpp`
- Modify: `libs/llm_client/src/llm_client.cpp`
- Modify: `tests/test_llm_client.cpp`

- [ ] **Step 1: Write the failing test**

`tests/test_llm_client.cpp`:
```cpp
#include <gtest/gtest.h>
#include "llm_client/llm_client.hpp"
#include "llm_client/errors.hpp"

TEST(LLMClient, ThrowsOnUnreachableHost) {
    LLMClient client("http://localhost:19999/v1", "", "test-model");
    EXPECT_THROW(client.complete("pick up object A"), LLMConnectionError);
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_llm_client -V
```

Expected: compilation error — headers not found.

- [ ] **Step 3: Implement errors.hpp**

`libs/llm_client/include/llm_client/errors.hpp`:
```cpp
#pragma once
#include <stdexcept>
#include <string>

class LLMConnectionError : public std::runtime_error {
public:
    explicit LLMConnectionError(const std::string& msg)
        : std::runtime_error("LLM connection error: " + msg) {}
};

class BTXMLParseError : public std::runtime_error {
public:
    std::string raw_xml;
    explicit BTXMLParseError(const std::string& msg, std::string xml = "")
        : std::runtime_error("BT XML parse error: " + msg), raw_xml(std::move(xml)) {}
};
```

- [ ] **Step 4: Implement llm_client.hpp**

`libs/llm_client/include/llm_client/llm_client.hpp`:
```cpp
#pragma once
#include <string>

class LLMClient {
public:
    // Reads OPENAI_BASE_URL (default: http://localhost:11434/v1),
    // OPENAI_API_KEY (default: empty), LLM_MODEL (default: llama3.2)
    LLMClient();

    // For testing: explicit configuration
    LLMClient(std::string base_url, std::string api_key, std::string model);

    // Sends a chat completion request with user_message as the user turn.
    // Returns the assistant message content.
    // Throws LLMConnectionError on network failure or non-200 response.
    virtual std::string complete(const std::string& user_message);

    virtual ~LLMClient() = default;

private:
    std::string base_url_;
    std::string api_key_;
    std::string model_;

    static std::string system_prompt();
    std::string origin() const;   // e.g. "http://localhost:11434"
    std::string base_path() const; // e.g. "/v1"
};
```

- [ ] **Step 5: Implement llm_client.cpp**

`libs/llm_client/src/llm_client.cpp`:
```cpp
#include "llm_client/llm_client.hpp"
#include "llm_client/errors.hpp"

#include <cstdlib>
#include <httplib.h>
#include <nlohmann/json.hpp>

static std::string env_or(const char* name, const char* fallback) {
    const char* v = std::getenv(name);
    return v ? v : fallback;
}

LLMClient::LLMClient()
    : base_url_(env_or("OPENAI_BASE_URL", "http://localhost:11434/v1"))
    , api_key_(env_or("OPENAI_API_KEY", ""))
    , model_(env_or("LLM_MODEL", "llama3.2"))
{}

LLMClient::LLMClient(std::string base_url, std::string api_key, std::string model)
    : base_url_(std::move(base_url))
    , api_key_(std::move(api_key))
    , model_(std::move(model))
{}

std::string LLMClient::origin() const {
    auto scheme_end = base_url_.find("://");
    if (scheme_end == std::string::npos) return base_url_;
    auto path_start = base_url_.find('/', scheme_end + 3);
    return path_start == std::string::npos ? base_url_ : base_url_.substr(0, path_start);
}

std::string LLMClient::base_path() const {
    auto scheme_end = base_url_.find("://");
    if (scheme_end == std::string::npos) return "";
    auto path_start = base_url_.find('/', scheme_end + 3);
    return path_start == std::string::npos ? "" : base_url_.substr(path_start);
}

std::string LLMClient::system_prompt() {
    return R"(You are a behavior tree planner for a robot arm. Output ONLY valid BT.CPP v4 XML.
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
- No markdown, no explanation, no code fences. XML only.)";
}

std::string LLMClient::complete(const std::string& user_message) {
    nlohmann::json body = {
        {"model", model_},
        {"messages", {
            {{"role", "system"}, {"content", system_prompt()}},
            {{"role", "user"}, {"content", user_message}}
        }},
        {"temperature", 0.0}
    };

    httplib::Client cli(origin());
    cli.set_connection_timeout(10);
    cli.set_read_timeout(60);

    httplib::Headers headers = {{"Content-Type", "application/json"}};
    if (!api_key_.empty()) {
        headers.insert({"Authorization", "Bearer " + api_key_});
    }

    auto res = cli.Post(base_path() + "/chat/completions",
                        headers, body.dump(), "application/json");

    if (!res) {
        throw LLMConnectionError("no response from " + origin());
    }
    if (res->status != 200) {
        throw LLMConnectionError("HTTP " + std::to_string(res->status) + ": " + res->body);
    }

    auto resp = nlohmann::json::parse(res->body);
    return resp["choices"][0]["message"]["content"].get<std::string>();
}
```

- [ ] **Step 6: Run test to verify it passes**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_llm_client -V
```

Expected: `LLMClient.ThrowsOnUnreachableHost` passes (connection refused → `LLMConnectionError`).

- [ ] **Step 7: Commit**

```bash
git add libs/llm_client/ tests/test_llm_client.cpp
git commit -m "feat: LLMClient HTTP layer with OpenAI-compatible API"
```

---

## Task 9: BTXMLValidator

**Files:**
- Modify: `libs/llm_client/include/llm_client/bt_xml_validator.hpp`
- Modify: `libs/llm_client/src/bt_xml_validator.cpp`
- Modify: `tests/test_llm_client.cpp`

`BTXMLValidator` uses BT.CPP's XML parser to check that all node names exist in a registered factory. It returns a list of error strings.

- [ ] **Step 1: Write the failing tests (append to test_llm_client.cpp)**

```cpp
#include "llm_client/bt_xml_validator.hpp"
#include "bt_nodes/registry.hpp"

static BT::BehaviorTreeFactory make_factory() {
    BT::BehaviorTreeFactory f;
    register_all_nodes(f);
    return f;
}

TEST(BTXMLValidator, ValidXmlReturnsNoErrors) {
    auto factory = make_factory();
    BTXMLValidator validator;
    const std::string valid_xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Sequence>
      <MoveArmTo location="TableA"/>
      <PickObject object="ObjectA"/>
    </Sequence>
  </BehaviorTree>
</root>)";
    auto errors = validator.validate(valid_xml, factory);
    EXPECT_TRUE(errors.empty()) << errors.front();
}

TEST(BTXMLValidator, UnknownNodeNameReturnsError) {
    auto factory = make_factory();
    BTXMLValidator validator;
    const std::string bad_xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <GrabObject object="ObjectA"/>
  </BehaviorTree>
</root>)";
    auto errors = validator.validate(bad_xml, factory);
    EXPECT_FALSE(errors.empty());
    EXPECT_NE(errors.front().find("GrabObject"), std::string::npos);
}

TEST(BTXMLValidator, MalformedXmlReturnsError) {
    auto factory = make_factory();
    BTXMLValidator validator;
    const std::string not_xml = "this is not xml at all";
    auto errors = validator.validate(not_xml, factory);
    EXPECT_FALSE(errors.empty());
}

TEST(BTXMLValidator, MissingRootElementReturnsError) {
    auto factory = make_factory();
    BTXMLValidator validator;
    const std::string no_root = R"(<MoveArmTo location="TableA"/>)";
    auto errors = validator.validate(no_root, factory);
    EXPECT_FALSE(errors.empty());
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_llm_client -V
```

Expected: compilation error — `bt_xml_validator.hpp` not found.

- [ ] **Step 3: Implement bt_xml_validator.hpp**

`libs/llm_client/include/llm_client/bt_xml_validator.hpp`:
```cpp
#pragma once
#include <string>
#include <vector>
#include "behaviortree_cpp/bt_factory.h"

class BTXMLValidator {
public:
    // Returns a list of human-readable error strings.
    // Empty list means the XML is valid and loadable.
    std::vector<std::string> validate(const std::string& xml,
                                      const BT::BehaviorTreeFactory& factory) const;
};
```

- [ ] **Step 4: Implement bt_xml_validator.cpp**

`libs/llm_client/src/bt_xml_validator.cpp`:
```cpp
#include "llm_client/bt_xml_validator.hpp"
#include <stdexcept>

std::vector<std::string> BTXMLValidator::validate(
    const std::string& xml,
    const BT::BehaviorTreeFactory& factory) const
{
    std::vector<std::string> errors;
    try {
        // createTreeFromText throws if XML is malformed or node names are unknown
        factory.createTreeFromText(xml);
    } catch (const std::exception& e) {
        errors.push_back(e.what());
    }
    return errors;
}
```

Note: BT.CPP's `createTreeFromText` throws a `std::exception` (typically `std::runtime_error`) when it encounters unknown node names or malformed XML. We catch it and return the message as the error list. This keeps the validator thin and means its error messages are always in sync with what BT.CPP actually checks.

- [ ] **Step 5: Run tests to verify they pass**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_llm_client -V
```

Expected: all 5 llm_client tests pass.

- [ ] **Step 6: Commit**

```bash
git add libs/llm_client/ tests/test_llm_client.cpp
git commit -m "feat: BTXMLValidator using BT.CPP factory for node name checking"
```

---

## Task 10: BTXMLRepairAgent

**Files:**
- Modify: `libs/llm_client/include/llm_client/bt_xml_repair_agent.hpp`
- Modify: `libs/llm_client/src/bt_xml_repair_agent.cpp`
- Modify: `tests/test_llm_client.cpp`

`BTXMLRepairAgent` calls `LLMClient::complete()` up to 3 times, injecting validator errors into the prompt on each retry. It takes `LLMClient&` and `BTXMLValidator&` by reference so they can be substituted with test fakes.

- [ ] **Step 1: Write the failing tests (append to test_llm_client.cpp)**

```cpp
#include "llm_client/bt_xml_repair_agent.hpp"

static const std::string VALID_XML = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <MoveArmTo location="TableA"/>
  </BehaviorTree>
</root>)";

static const std::string INVALID_XML = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <GrabObject object="ObjectA"/>
  </BehaviorTree>
</root>)";

class FakeLLMClient : public LLMClient {
public:
    std::vector<std::string> responses;
    int call_count = 0;
    std::string complete(const std::string&) override {
        return responses.at(call_count++);
    }
};

TEST(BTXMLRepairAgent, ReturnsValidXmlOnFirstAttempt) {
    FakeLLMClient fake;
    fake.responses = {VALID_XML};
    BTXMLValidator validator;
    auto factory = make_factory();

    BTXMLRepairAgent agent(fake, validator, factory);
    auto xml = agent.get_valid_xml("pick up A");
    EXPECT_EQ(fake.call_count, 1);
    EXPECT_FALSE(xml.empty());
}

TEST(BTXMLRepairAgent, RetriesOnInvalidXmlAndSucceeds) {
    FakeLLMClient fake;
    fake.responses = {INVALID_XML, VALID_XML};
    BTXMLValidator validator;
    auto factory = make_factory();

    BTXMLRepairAgent agent(fake, validator, factory);
    auto xml = agent.get_valid_xml("pick up A");
    EXPECT_EQ(fake.call_count, 2);
    EXPECT_FALSE(xml.empty());
}

TEST(BTXMLRepairAgent, ThrowsAfterMaxRetries) {
    FakeLLMClient fake;
    fake.responses = {INVALID_XML, INVALID_XML, INVALID_XML};
    BTXMLValidator validator;
    auto factory = make_factory();

    BTXMLRepairAgent agent(fake, validator, factory, /*max_retries=*/3);
    EXPECT_THROW(agent.get_valid_xml("pick up A"), BTXMLParseError);
    EXPECT_EQ(fake.call_count, 3);
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_llm_client -V
```

Expected: compilation error — `bt_xml_repair_agent.hpp` not found.

- [ ] **Step 3: Implement bt_xml_repair_agent.hpp**

`libs/llm_client/include/llm_client/bt_xml_repair_agent.hpp`:
```cpp
#pragma once
#include <string>
#include "behaviortree_cpp/bt_factory.h"
#include "llm_client/llm_client.hpp"
#include "llm_client/bt_xml_validator.hpp"
#include "llm_client/errors.hpp"

class BTXMLRepairAgent {
public:
    BTXMLRepairAgent(LLMClient& client,
                     BTXMLValidator& validator,
                     const BT::BehaviorTreeFactory& factory,
                     int max_retries = 3);

    // Returns valid BT XML or throws BTXMLParseError after max_retries.
    std::string get_valid_xml(const std::string& goal);

private:
    LLMClient& client_;
    BTXMLValidator& validator_;
    const BT::BehaviorTreeFactory& factory_;
    int max_retries_;

    std::string repair_prompt(const std::string& goal,
                              const std::string& bad_xml,
                              const std::vector<std::string>& errors) const;
};
```

- [ ] **Step 4: Implement bt_xml_repair_agent.cpp**

`libs/llm_client/src/bt_xml_repair_agent.cpp`:
```cpp
#include "llm_client/bt_xml_repair_agent.hpp"

BTXMLRepairAgent::BTXMLRepairAgent(LLMClient& client,
                                   BTXMLValidator& validator,
                                   const BT::BehaviorTreeFactory& factory,
                                   int max_retries)
    : client_(client), validator_(validator), factory_(factory), max_retries_(max_retries)
{}

std::string BTXMLRepairAgent::repair_prompt(
    const std::string& goal,
    const std::string& bad_xml,
    const std::vector<std::string>& errors) const
{
    std::string prompt = "Goal: " + goal + "\n\n";
    prompt += "Your previous output was invalid. Errors:\n";
    for (const auto& e : errors) {
        prompt += "- " + e + "\n";
    }
    prompt += "\nPrevious invalid XML:\n" + bad_xml + "\n\nCorrected XML only:";
    return prompt;
}

std::string BTXMLRepairAgent::get_valid_xml(const std::string& goal) {
    std::string last_xml;
    std::vector<std::string> last_errors;

    for (int attempt = 0; attempt < max_retries_; ++attempt) {
        const std::string user_msg = (attempt == 0)
            ? "Goal: " + goal
            : repair_prompt(goal, last_xml, last_errors);

        last_xml = client_.complete(user_msg);
        last_errors = validator_.validate(last_xml, factory_);

        if (last_errors.empty()) {
            return last_xml;
        }
    }

    std::string error_summary;
    for (const auto& e : last_errors) error_summary += e + "; ";
    throw BTXMLParseError(error_summary, last_xml);
}
```

- [ ] **Step 5: Run tests to verify they pass**

```bash
cmake --build build -j$(nproc) && cd build && ctest -R test_llm_client -V
```

Expected: all 8 llm_client tests pass.

- [ ] **Step 6: Commit**

```bash
git add libs/llm_client/ tests/test_llm_client.cpp
git commit -m "feat: BTXMLRepairAgent with retry loop and structured error feedback"
```

---

## Task 11: Wire LLM Layer into main.cpp

Replace the hardcoded XML with a live LLM call. Goal string comes from `argv[1]`.

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace main.cpp**

`src/main.cpp`:
```cpp
#include <iostream>
#include <thread>
#include <chrono>

#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_cpp/groot2_publisher.h"
#include "bt_nodes/registry.hpp"
#include "world_sim/world_state.hpp"
#include "llm_client/llm_client.hpp"
#include "llm_client/bt_xml_validator.hpp"
#include "llm_client/bt_xml_repair_agent.hpp"
#include "llm_client/errors.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: llm_bt_demo \"<goal string>\"\n\n";
        std::cerr << "Environment variables:\n";
        std::cerr << "  OPENAI_BASE_URL  base URL for OpenAI-compatible API"
                     " (default: http://localhost:11434/v1)\n";
        std::cerr << "  OPENAI_API_KEY   bearer token, empty for Ollama\n";
        std::cerr << "  LLM_MODEL        model name (default: llama3.2)\n";
        return 1;
    }

    const std::string goal = argv[1];

    // Initial world: ObjectA at TableA, ObjectB at TableB, ObjectC at TableC
    WorldState world;
    world.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    world.objects["ObjectB"] = {"ObjectB", WorldState::Location::TableB, false};
    world.objects["ObjectC"] = {"ObjectC", WorldState::Location::TableC, false};

    // Build the factory first — validator needs it to check node names
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);

    LLMClient llm;
    BTXMLValidator validator;
    BTXMLRepairAgent repair_agent(llm, validator, factory);

    std::string xml;
    try {
        std::cout << "Sending goal to LLM: \"" << goal << "\"\n";
        xml = repair_agent.get_valid_xml(goal);
    } catch (const BTXMLParseError& e) {
        std::cerr << "Failed to get valid BT XML after retries.\n" << e.what() << "\n";
        if (!e.raw_xml.empty()) std::cerr << "Last XML:\n" << e.raw_xml << "\n";
        return 1;
    } catch (const LLMConnectionError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    std::cout << "Generated BT XML:\n" << xml << "\n\n";

    auto blackboard = BT::Blackboard::create();
    blackboard->set("world_state", &world);

    auto tree = factory.createTreeFromText(xml, blackboard);

    BT::Groot2Publisher publisher(tree, 1667);
    std::cout << "Executing tree... (connect Groot2 to localhost:1667)\n";

    BT::NodeStatus status = BT::NodeStatus::RUNNING;
    while (status == BT::NodeStatus::RUNNING) {
        status = tree.tickOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Tree finished: "
              << (status == BT::NodeStatus::SUCCESS ? "SUCCESS" : "FAILURE") << "\n";
    return status == BT::NodeStatus::SUCCESS ? 0 : 1;
}
```

- [ ] **Step 2: Build**

```bash
cmake --build build -j$(nproc)
```

- [ ] **Step 3: Manual test with Ollama running**

```bash
# In a separate terminal, ensure Ollama is running with a model:
# ollama run llama3.2

export OPENAI_BASE_URL=http://localhost:11434/v1
export LLM_MODEL=llama3.2

./build/llm_bt_demo "Pick up object A and move it to location C"
```

Expected: LLM returns XML, tree executes, status SUCCESS printed.

- [ ] **Step 4: Commit**

```bash
git add src/main.cpp
git commit -m "feat: LLM-driven BT execution replacing hardcoded XML"
```

---

## Task 12: LLM Integration Tests

**Files:**
- Modify: `tests/test_llm_integration.cpp`

These tests require a running LLM endpoint. They are skipped automatically when `OPENAI_BASE_URL` is not set.

- [ ] **Step 1: Implement test_llm_integration.cpp**

`tests/test_llm_integration.cpp`:
```cpp
#include <gtest/gtest.h>
#include <cstdlib>
#include "behaviortree_cpp/bt_factory.h"
#include "bt_nodes/registry.hpp"
#include "llm_client/llm_client.hpp"
#include "llm_client/bt_xml_validator.hpp"
#include "llm_client/bt_xml_repair_agent.hpp"
#include "llm_client/errors.hpp"

class LLMIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (std::getenv("OPENAI_BASE_URL") == nullptr) {
            GTEST_SKIP() << "OPENAI_BASE_URL not set — skipping live LLM tests";
        }
        register_all_nodes(factory);
    }
    BT::BehaviorTreeFactory factory;
    LLMClient llm;
    BTXMLValidator validator;
};

TEST_F(LLMIntegrationTest, SimplePickAndPlaceProducesValidXML) {
    BTXMLRepairAgent agent(llm, validator, factory);
    auto xml = agent.get_valid_xml("Pick up object A and move it to location C");
    auto errors = validator.validate(xml, factory);
    EXPECT_TRUE(errors.empty()) << "Validation errors: " << (errors.empty() ? "" : errors.front());
}

TEST_F(LLMIntegrationTest, StackingGoalProducesValidXML) {
    BTXMLRepairAgent agent(llm, validator, factory);
    auto xml = agent.get_valid_xml("Move object B to TableA then move object C to TableB");
    auto errors = validator.validate(xml, factory);
    EXPECT_TRUE(errors.empty()) << "Validation errors: " << (errors.empty() ? "" : errors.front());
}

TEST_F(LLMIntegrationTest, GeneratedXMLIsLoadableIntoFactory) {
    BTXMLRepairAgent agent(llm, validator, factory);
    auto xml = agent.get_valid_xml("Pick up object A and place it at TableB");
    // createTreeFromText throws if the tree can't be loaded
    EXPECT_NO_THROW(factory.createTreeFromText(xml));
}
```

- [ ] **Step 2: Run without OPENAI_BASE_URL set — verify tests skip**

```bash
unset OPENAI_BASE_URL
cmake --build build -j$(nproc) && cd build && ctest -R test_llm_integration -V
```

Expected: all 3 tests show `SKIPPED`.

- [ ] **Step 3: Run with Ollama — verify tests pass**

```bash
export OPENAI_BASE_URL=http://localhost:11434/v1
export LLM_MODEL=llama3.2
cd build && ctest -R test_llm_integration -V
```

Expected: all 3 tests pass (may take 10–30 seconds per test).

- [ ] **Step 4: Commit**

```bash
git add tests/test_llm_integration.cpp
git commit -m "feat: LLM integration tests with GTEST_SKIP when OPENAI_BASE_URL unset"
```

---

## Task 13: Error Handling Hardening

Verify the three error zones behave correctly end-to-end with real input.

**Files:**
- No code changes — manual verification only.

- [ ] **Step 1: Test network failure**

```bash
export OPENAI_BASE_URL=http://localhost:19999/v1   # nothing listening here
./build/llm_bt_demo "Pick up A"
```

Expected: `LLM connection error: no response from http://localhost:19999` printed, exit code 1.

- [ ] **Step 2: Test repair loop with a bad model**

Set `LLM_MODEL` to a model name that doesn't exist or consistently generates garbage. The repair agent should exhaust its 3 retries and print the error list.

```bash
export OPENAI_BASE_URL=http://localhost:11434/v1
export LLM_MODEL=nonexistent-model
./build/llm_bt_demo "Pick up A"
```

Expected: connection error or parse error printed, exit code 1.

- [ ] **Step 3: Run full test suite and confirm all pass**

```bash
cd build && ctest -V
```

Expected: all unit tests pass, integration tests skip if `OPENAI_BASE_URL` is unset.

- [ ] **Step 4: Commit**

```bash
git commit --allow-empty -m "test: verify error handling end-to-end"
```

---

## Task 14: Docker and .gitignore

**Files:**
- Create: `docker/Dockerfile`
- Create: `.gitignore`

- [ ] **Step 1: Create .gitignore**

`.gitignore`:
```
build/
.superpowers/
*.o
*.a
.DS_Store
```

- [ ] **Step 2: Create Dockerfile**

`docker/Dockerfile`:
```dockerfile
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    git \
    libzmq3-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc)

EXPOSE 1667

ENTRYPOINT ["./build/llm_bt_demo"]
CMD ["--help"]
```

- [ ] **Step 3: Build Docker image**

```bash
docker build -f docker/Dockerfile -t llm-bt-demo .
```

Expected: image builds successfully.

- [ ] **Step 4: Run the demo in Docker**

```bash
docker run --rm \
  -e OPENAI_BASE_URL=http://host.docker.internal:11434/v1 \
  -e LLM_MODEL=llama3.2 \
  -p 1667:1667 \
  llm-bt-demo "Pick up object A and move it to location C"
```

Note: `host.docker.internal` routes to the host machine where Ollama is running. On Linux, use `--add-host=host.docker.internal:host-gateway` if that hostname isn't available.

- [ ] **Step 5: Commit**

```bash
git add .gitignore docker/
git commit -m "feat: Dockerfile and .gitignore"
```

---

## Done

All three libraries are built, tested, and wired together. The demo:
1. Accepts a natural language goal from the CLI
2. Calls an OpenAI-compatible LLM to generate BT.CPP v4 XML
3. Validates the XML against the registered node factory
4. Repairs it with structured error feedback if needed (up to 3 retries)
5. Executes the tree against the simulated WorldState
6. Streams live node status to Groot2 on port 1667
