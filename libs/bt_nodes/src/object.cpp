#include "bt_nodes/object.hpp"
#include "world_sim/world_sim.hpp"
#include <cassert>

static WorldState* get_world(const BT::TreeNode& node) {
    auto* ws = node.config().blackboard->get<WorldState*>("world_state");
    assert(ws && "world_state not set on blackboard");
    return ws;
}

namespace object {

BT::NodeStatus PickAction::onStart()  { return BT::NodeStatus::RUNNING; }
BT::NodeStatus PickAction::onRunning() {
    auto res = getInput<std::string>("object");
    if (!res) return BT::NodeStatus::FAILURE;
    WorldSim::pick_object(*get_world(*this), res.value());
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus PlaceAction::onStart()  { return BT::NodeStatus::RUNNING; }
BT::NodeStatus PlaceAction::onRunning() {
    auto obj_res = getInput<std::string>("object");
    auto loc_res = getInput<std::string>("location");
    if (!obj_res || !loc_res) return BT::NodeStatus::FAILURE;
    WorldSim::place_object(*get_world(*this), obj_res.value(),
                           location_from_string(loc_res.value()));
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus AtCondition::tick() {
    auto obj_res = getInput<std::string>("object");
    auto loc_res = getInput<std::string>("location");
    if (!obj_res || !loc_res) return BT::NodeStatus::FAILURE;
    auto* ws = get_world(*this);
    auto it = ws->objects.find(obj_res.value());
    if (it == ws->objects.end()) return BT::NodeStatus::FAILURE;
    return it->second.location == location_from_string(loc_res.value())
        ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

} // namespace object
