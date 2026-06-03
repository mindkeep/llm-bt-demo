#include <iostream>
#include <thread>
#include <chrono>

#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_cpp/loggers/groot2_publisher.h"
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
