add_library(bt_task_agent_lib STATIC
    src/bt_task_agent.cpp
)
target_include_directories(bt_task_agent_lib PUBLIC include)
target_link_libraries(bt_task_agent_lib PUBLIC
    world_sim_lib
    llm_client_lib
    behaviortree_cpp
)
