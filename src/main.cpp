#include <iostream>
#include <csignal>
#include <string>

#include "behaviortree_cpp/bt_factory.h"
#include "bt_nodes/registry.hpp"
#include "world_sim/world_state.hpp"
#include "world_sim/world_sim.hpp"
#include "llm_client/llm_client.hpp"
#include "llm_client/bt_xml_validator.hpp"
#include "llm_client/bt_xml_repair_agent.hpp"
#include "bt_task_agent/bt_task_agent.hpp"

static void print_usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  llm_bt_demo \"<goal>\"   run a single goal and exit\n";
    std::cerr << "  llm_bt_demo             interactive session (/exit or Ctrl-D to quit)\n\n";
    std::cerr << "Environment variables:\n";
    std::cerr << "  OPENAI_BASE_URL  OpenAI-compatible API base URL"
                 " (default: http://localhost:11434/v1)\n";
    std::cerr << "  OPENAI_API_KEY   bearer token, empty for Ollama\n";
    std::cerr << "  LLM_MODEL        model name (default: granite4.1:3b-q8_0)\n";
}

static void print_world_state(const WorldState& ws) {
    std::cout << "\n--- World State ---\n";
    std::cout << "Gripper:  "
              << (ws.gripper == WorldState::GripperState::Open ? "Open" : "Closed") << "\n";
    std::cout << "Arm:      " << location_to_string(ws.arm_position) << "\n";
    std::cout << "Objects:\n";
    for (const auto& [name, obj] : ws.objects) {
        std::cout << "  " << name << "  @ " << location_to_string(obj.location);
        if (obj.held) std::cout << "  (held)";
        std::cout << "\n";
    }
    std::cout << "-------------------\n";
}

static WorldState make_initial_world() {
    WorldState world;
    world.objects["ObjectA"] = {.name = "ObjectA", .location = WorldState::Location::TableA};
    world.objects["ObjectB"] = {.name = "ObjectB", .location = WorldState::Location::TableB};
    world.objects["ObjectC"] = {.name = "ObjectC", .location = WorldState::Location::TableC};
    return world;
}

int main(int argc, char* argv[]) {
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);

    LLMClient llm;
    BTXMLValidator validator;
    BTXMLRepairAgent repair_agent(llm, validator, factory);
    BTTaskAgent agent(repair_agent, factory);

    if (argc >= 2) {
        WorldState world = make_initial_world();
        try {
            agent.execute_goal(argv[1], world);
            print_world_state(world);
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    std::signal(SIGINT, [](int) {
        std::cout << "\nGoodbye.\n";
        std::exit(0);
    });

    WorldState world = make_initial_world();
    std::cout << "LLM Robot Task Planner  (type /exit or Ctrl-D to quit)\n";

    while (true) {
        print_world_state(world);
        std::cout << "\nGoal> " << std::flush;

        std::string goal;
        if (!std::getline(std::cin, goal)) {
            std::cout << "\nGoodbye.\n";
            break;
        }
        if (goal == "/exit") { std::cout << "Goodbye.\n"; break; }
        if (goal.empty()) continue;

        try {
            agent.execute_goal(goal, world);
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n";
        }
    }

    return 0;
}
