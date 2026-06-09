#include "bt_task_agent/bt_task_agent.hpp"
#include "llm_client/errors.hpp"
#include "world_sim/world_sim.hpp"
#include "behaviortree_cpp/loggers/groot2_publisher.h"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

BTTaskAgent::BTTaskAgent(BTXMLRepairAgent& repair_agent,
                         BT::BehaviorTreeFactory& factory,
                         int max_retries,
                         uint16_t groot2_port)
    : repair_agent_(repair_agent)
    , factory_(factory)
    , max_retries_(max_retries)
    , groot2_port_(groot2_port)
{}

// One retry loop, two failure modes. Each pass: (1) get valid XML from the
// repair agent, (2) execute it. A failure in either step rebuilds current_prompt
// with the relevant context and loops; success returns. After max_retries the
// last exception propagates to the caller.
void BTTaskAgent::execute_goal(const std::string& goal, WorldState& world) {
    std::string current_prompt = build_initial_prompt(goal, world);

    for (int attempt = 0; attempt <= max_retries_; ++attempt) {
        // --- Generate XML ---
        std::string xml;
        try {
            if (attempt == 0)
                std::cout << "Sending goal to LLM: \"" << goal << "\"\n";
            else
                std::cout << "Asking LLM for recovery plan (attempt "
                          << attempt << "/" << max_retries_ << ")...\n";

            xml = repair_agent_.get_valid_xml(current_prompt);
        } catch (const BTXMLParseError& e) {
            std::cerr << "\nXML error: " << e.what() << "\n";
            if (!e.raw_xml.empty())
                std::cerr << "Last XML:\n" << e.raw_xml << "\n";

            if (attempt >= max_retries_) {
                std::cerr << "Giving up after " << max_retries_ << " recovery attempt"
                          << (max_retries_ == 1 ? "" : "s") << ".\n";
                throw;
            }

            std::cout << "Attempting recovery (" << (attempt + 1) << "/"
                      << max_retries_ << ")...\n";
            current_prompt = build_xml_error_prompt(goal, e.what(), e.raw_xml, world);
            continue;
        }
        // LLMConnectionError propagates immediately — no retry.

        std::cout << "Generated BT XML:\n" << xml << "\n\n";

        // --- Execute tree ---
        WorldState state_before = world; // snapshot for recovery prompt

        auto blackboard = BT::Blackboard::create();
        blackboard->set("world_state", &world);
        auto tree = factory_.createTreeFromText(xml, blackboard);

        // Publisher is optional: port 0 disables it (used in tests).
        std::unique_ptr<BT::Groot2Publisher> publisher;
        if (groot2_port_ > 0) {
            publisher = std::make_unique<BT::Groot2Publisher>(tree, groot2_port_);
            std::cout << "Executing tree... (connect Groot2 to localhost:"
                      << groot2_port_ << ")\n";
        } else {
            std::cout << "Executing tree...\n";
        }

        try {
            tick_tree(tree);
            std::cout << "Tree finished: SUCCESS\n";
            return;
        } catch (const std::exception& e) {
            std::cerr << "\nExecution error: " << e.what() << "\n";

            if (attempt >= max_retries_) {
                std::cerr << "Giving up after " << max_retries_ << " recovery attempt"
                          << (max_retries_ == 1 ? "" : "s") << ".\n";
                throw;
            }

            std::cout << "Attempting recovery (" << (attempt + 1) << "/"
                      << max_retries_ << ")...\n";

            current_prompt = build_recovery_prompt(goal, state_before, xml,
                                                   e.what(), world);
        }
    }
}

void BTTaskAgent::tick_tree(BT::Tree& tree) {
    BT::NodeStatus status = BT::NodeStatus::RUNNING;
    while (status == BT::NodeStatus::RUNNING) {
        status = tree.tickOnce();
        // Pace the loop so a Groot2 viewer can show node transitions; nothing
        // here is time-sensitive, the delay is purely for human-visible replay.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    // Normalise FAILURE into an exception so execute_goal has a single recovery
    // path: both a thrown node error and a plain FAILURE land in the same catch.
    if (status != BT::NodeStatus::SUCCESS)
        throw std::runtime_error("Tree returned FAILURE status");
}

std::string BTTaskAgent::world_state_to_string(const WorldState& ws) {
    std::string s;
    s += "  Gripper: ";
    s += (ws.gripper == WorldState::GripperState::Open ? "Open" : "Closed");
    s += "\n  Arm: ";
    s += location_to_string(ws.arm_position);
    s += "\n";
    for (const auto& [name, obj] : ws.objects) {
        s += "  " + name + " @ " + std::string(location_to_string(obj.location));
        if (obj.held) s += " (held)";
        s += "\n";
    }
    return s;
}

std::string BTTaskAgent::build_initial_prompt(const std::string& goal,
                                              const WorldState& world) {
    // BTXMLRepairAgent prepends "Goal: " on the first attempt, so omit it here.
    return goal + "\n\nCurrent world state:\n" + world_state_to_string(world);
}

std::string BTTaskAgent::build_xml_error_prompt(const std::string& goal,
                                                const std::string& error,
                                                const std::string& raw_xml,
                                                const WorldState& world) {
    std::string prompt =
        "The previous attempt to generate a valid behavior tree failed.\n\n"
        "Original goal: \"" + goal + "\"\n\n"
        "XML error: " + error + "\n\n";

    if (!raw_xml.empty())
        prompt += "Invalid XML produced:\n" + raw_xml + "\n\n";

    prompt +=
        "Current world state:\n" + world_state_to_string(world) + "\n"
        "Generate a valid behavior tree using ONLY the nodes listed in the system prompt.\n"
        "Do NOT use nodes that are not available (e.g. If, While, Loop, Switch).";

    return prompt;
}

std::string BTTaskAgent::build_recovery_prompt(const std::string& goal,
                                               const WorldState& state_before,
                                               const std::string& xml,
                                               const std::string& error,
                                               const WorldState& state_after) {
    return
        "The previous execution attempt failed and needs recovery.\n\n"
        "Original goal: \"" + goal + "\"\n\n"
        "World state before this attempt:\n" + world_state_to_string(state_before) +
        "\nBT XML that was attempted:\n" + xml + "\n"
        "Runtime error: " + error + "\n\n"
        "Current world state after partial execution:\n" + world_state_to_string(state_after) +
        "\nPlan from the CURRENT world state, not from the start. "
        "If an object is already held, place it -- do NOT pick again. "
        "If the arm is already at the right table, do not move it again.";
}
