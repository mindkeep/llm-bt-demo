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
