#pragma once
#include <string>
#include <vector>
#include "behaviortree_cpp/bt_factory.h"

// Returns a list of human-readable error strings; empty means valid and loadable.
std::vector<std::string> validate_bt_xml(const std::string& xml,
                                         BT::BehaviorTreeFactory& factory);
