#include <iostream>
#include <thread>
#include <chrono>

#include "behaviortree_cpp/bt_factory.h"
#include "bt_nodes/registry.hpp"
#include "world_sim/world_state.hpp"

static const char* HARDCODED_XML = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Sequence>
      <MoveArmTo location="TableA"/>
      <OpenGripper/>
      <PickObject object="ObjectA"/>
      <CloseGripper/>
      <MoveArmTo location="TableC"/>
      <OpenGripper/>
      <PlaceObject object="ObjectA" location="TableC"/>
    </Sequence>
  </BehaviorTree>
</root>)";

int main() {
    WorldState world;
    world.objects["ObjectA"] = {"ObjectA", WorldState::Location::TableA, false};
    world.objects["ObjectB"] = {"ObjectB", WorldState::Location::TableB, false};
    world.objects["ObjectC"] = {"ObjectC", WorldState::Location::TableC, false};

    BT::BehaviorTreeFactory factory;
    register_all_nodes(factory);

    auto blackboard = BT::Blackboard::create();
    blackboard->set("world_state", &world);

    auto tree = factory.createTreeFromText(HARDCODED_XML, blackboard);

    std::cout << "Executing hardcoded pick-and-place tree...\n";

    BT::NodeStatus status = BT::NodeStatus::RUNNING;
    while (status == BT::NodeStatus::RUNNING) {
        status = tree.tickOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Tree finished with status: "
              << (status == BT::NodeStatus::SUCCESS ? "SUCCESS" : "FAILURE") << "\n";
    std::cout << "ObjectA is now at: "
              << (world.objects.at("ObjectA").location == WorldState::Location::TableC
                  ? "TableC" : "elsewhere") << "\n";
    return 0;
}
