#pragma once
#include <string>
#include <vector>
#include "behaviortree_cpp/bt_factory.h"

class BTXMLValidator {
public:
    // Returns a list of human-readable error strings.
    // Empty list means the XML is valid and loadable.
    std::vector<std::string> validate(const std::string& xml,
                                      BT::BehaviorTreeFactory& factory) const;
};
