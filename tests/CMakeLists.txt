include(GoogleTest)

# DISCOVERY_MODE PRE_TEST: run discovery at ctest time, not at build time.
# Required to avoid a cmake 4.x / cmake_minimum_required(3.16) compat issue
# where POST_BUILD (the old default) is used, causing a race condition in
# parallel builds.

add_executable(test_world_sim test_world_sim.cpp)
target_link_libraries(test_world_sim PRIVATE world_sim_lib GTest::gtest_main)
gtest_discover_tests(test_world_sim DISCOVERY_MODE PRE_TEST)

add_executable(test_bt_nodes test_bt_nodes.cpp)
target_link_libraries(test_bt_nodes PRIVATE bt_nodes_lib GTest::gtest_main)
gtest_discover_tests(test_bt_nodes DISCOVERY_MODE PRE_TEST)

add_executable(test_llm_client test_llm_client.cpp)
target_link_libraries(test_llm_client PRIVATE llm_client_lib bt_nodes_lib GTest::gtest_main)
gtest_discover_tests(test_llm_client DISCOVERY_MODE PRE_TEST)

add_executable(test_bt_task_agent test_bt_task_agent.cpp)
target_link_libraries(test_bt_task_agent PRIVATE bt_task_agent_lib bt_nodes_lib GTest::gtest_main)
gtest_discover_tests(test_bt_task_agent DISCOVERY_MODE PRE_TEST)

add_executable(test_llm_integration test_llm_integration.cpp)
target_link_libraries(test_llm_integration PRIVATE llm_client_lib bt_nodes_lib GTest::gtest_main)
gtest_discover_tests(test_llm_integration DISCOVERY_MODE PRE_TEST)
