#pragma once
#include <string>
#include <cstdint>
#include "behaviortree_cpp/bt_factory.h"
#include "world_sim/world_state.hpp"
#include "llm_client/bt_xml_repair_agent.hpp"

// Orchestrates the full cycle: generate BT XML via LLM → execute against a
// live WorldState → recover from runtime errors by feeding context back to
// the LLM. Up to max_retries recovery attempts are made before giving up.
//
// Set groot2_port=0 to disable the Groot2 live publisher (useful in tests).
class BTTaskAgent {
public:
    BTTaskAgent(BTXMLRepairAgent& repair_agent,
                BT::BehaviorTreeFactory& factory,
                int max_retries = 3,
                uint16_t groot2_port = 1667);

    // Attempts to achieve goal. World state is updated in-place.
    // Throws std::exception after all retries exhausted.
    // Progress and errors are printed to stdout/stderr.
    void execute_goal(const std::string& goal, WorldState& world);

private:
    BTXMLRepairAgent& repair_agent_;
    BT::BehaviorTreeFactory& factory_;
    int max_retries_;
    uint16_t groot2_port_;

    // Ticks tree to completion. Throws std::exception on FAILURE status or node exception.
    static void tick_tree(BT::Tree& tree);

    // Serialises world state into a compact, prompt-friendly string.
    static std::string world_state_to_string(const WorldState& ws);

    // Builds the recovery prompt sent to the LLM after a runtime failure.
    static std::string build_recovery_prompt(const std::string& goal,
                                             const WorldState& state_before,
                                             const std::string& xml,
                                             const std::string& error,
                                             const WorldState& state_after);

    // Builds the recovery prompt sent to the LLM after XML parse/validation failure.
    static std::string build_xml_error_prompt(const std::string& goal,
                                              const std::string& error,
                                              const std::string& raw_xml,
                                              const WorldState& world);
};
