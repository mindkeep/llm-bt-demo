add_library(bt_nodes_lib STATIC
    src/arm.cpp
    src/gripper.cpp
    src/object.cpp
    src/registry.cpp
)
target_include_directories(bt_nodes_lib PUBLIC include)
target_link_libraries(bt_nodes_lib PUBLIC
    world_sim_lib
    behaviortree_cpp
)
