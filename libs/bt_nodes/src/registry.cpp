#include "bt_nodes/registry.hpp"
#include "bt_nodes/arm.hpp"
#include "bt_nodes/gripper.hpp"
#include "bt_nodes/object.hpp"

// Maps XML element names to C++ node types. This is the robot's vocabulary: the
// LLM may only use the names registered here, and BTXMLValidator rejects any XML
// referencing a name that is missing. Must run before any tree is created.
void register_all_nodes(BT::BehaviorTreeFactory& factory) {
    factory.registerNodeType<arm::MoveAction>("MoveArmTo");
    factory.registerNodeType<arm::NearCondition>("IsArmNear");
    factory.registerNodeType<gripper::OpenAction>("OpenGripper");
    factory.registerNodeType<gripper::CloseAction>("CloseGripper");
    factory.registerNodeType<gripper::OpenCondition>("IsGripperOpen");
    factory.registerNodeType<object::PickAction>("PickObject");
    factory.registerNodeType<object::PlaceAction>("PlaceObject");
    factory.registerNodeType<object::AtCondition>("IsObjectAt");
}
