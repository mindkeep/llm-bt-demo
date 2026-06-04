#include <iostream>
#include <csignal>
#include <string>
#include <thread>
#include <chrono>

#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_cpp/loggers/groot2_publisher.h"
#include "bt_nodes/registry.hpp"
#include "world_sim/world_state.hpp"
#include "world_sim/world_sim.hpp"
#include "llm_client/llm_client.hpp"
#include "llm_client/bt_xml_validator.hpp"
#include "llm_client/bt_xml_repair_agent.hpp"
#include "llm_client/errors.hpp"

static void print_usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  llm_bt_demo \"<goal>\"   run a single goal and exit\n";
    std::cerr << "  llm_bt_demo             interactive session (/exit or Ctrl-D to quit)\n\n";
    std::cerr << "Environment variables:\n";
    std::cerr << "  OPENAI_BASE_URL  OpenAI-compatible API base URL"
                 " (default: http://localhost:11434/v1)\n";
    std::cerr << "  OPENAI_API_KEY   bearer token, empty for Ollama\n";
    std::cerr << "  LLM_MODEL        model name (default: llama3.2)\n";
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

// Returns true on success, false on error. Errors are printed to stderr.
static bool run_goal(const std::string& goal,
                     WorldState& world,
                     BTXMLRepairAgent& repair_agent,
                     BT::BehaviorTreeFactory& factory) {
    std::string xml;
    try {
        std::cout << "Sending goal to LLM: \"" << goal << "\"\n";
        xml = repair_agent.get_valid_xml(goal);
    } catch (const BTXMLParseError& e) {
        std::cerr << "Failed to get valid BT XML after retries.\n" << e.what() << "\n";
        if (!e.raw_xml.empty()) std::cerr << "Last XML:\n" << e.raw_xml << "\n";
        return false;
    } catch (const LLMConnectionError& e) {
        std::cerr << e.what() << "\n";
        return false;
    }

    std::cout << "Generated BT XML:\n" << xml << "\n\n";

    auto blackboard = BT::Blackboard::create();
    blackboard->set("world_state", &world);
    auto tree = factory.createTreeFromText(xml, blackboard);

    // Publisher is scoped to this execution so the port is released between goals.
    BT::Groot2Publisher publisher(tree, 1667);
    std::cout << "Executing tree... (connect Groot2 to localhost:1667)\n";

    BT::NodeStatus status = BT::NodeStatus::RUNNING;
    while (status == BT::NodeStatus::RUNNING) {
        status = tree.tickOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Tree finished: "
              << (status == BT::NodeStatus::SUCCESS ? "SUCCESS" : "FAILURE") << "\n";
    return status == BT::NodeStatus::SUCCESS;
}

static WorldState make_initial_world() {
    WorldState world;
    world.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    world.objects["ObjectB"] = {"ObjectB", WorldState::Location::TableB, false};
    world.objects["ObjectC"] = {"ObjectC", WorldState::Location::TableC, false};
    return world;
}

int main(int argc, char* argv[]) {
    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);

    LLMClient llm;
    BTXMLValidator validator;
    BTXMLRepairAgent repair_agent(llm, validator, factory);

    // Single-shot mode: goal provided as argument
    if (argc >= 2) {
        WorldState world = make_initial_world();
        bool ok = run_goal(argv[1], world, repair_agent, factory);
        if (ok) print_world_state(world);
        return ok ? 0 : 1;
    }

    // Interactive loop mode
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

        if (goal == "/exit") {
            std::cout << "Goodbye.\n";
            break;
        }

        if (goal.empty()) continue;

        run_goal(goal, world, repair_agent, factory);
    }

    return 0;
}
