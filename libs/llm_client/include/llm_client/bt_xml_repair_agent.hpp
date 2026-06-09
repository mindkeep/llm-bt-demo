#pragma once
#include <span>
#include <string>
#include "behaviortree_cpp/bt_factory.h"
#include "llm_client/llm_client.hpp"
#include "llm_client/errors.hpp"

class BTXMLRepairAgent {
public:
    BTXMLRepairAgent(LLMClient& client,
                     BT::BehaviorTreeFactory& factory,
                     int max_retries = 3);

    // Returns valid BT XML or throws BTXMLParseError after max_retries.
    std::string get_valid_xml(const std::string& goal);

private:
    LLMClient& client_;
    BT::BehaviorTreeFactory& factory_;
    int max_retries_;

    std::string repair_prompt(const std::string& goal,
                              const std::string& bad_xml,
                              std::span<const std::string> errors) const;
};
