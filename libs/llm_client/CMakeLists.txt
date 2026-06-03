add_library(llm_client_lib STATIC
    src/llm_client.cpp
    src/bt_xml_validator.cpp
    src/bt_xml_repair_agent.cpp
)
target_include_directories(llm_client_lib PUBLIC include)

find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_compile_definitions(llm_client_lib PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(llm_client_lib PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

target_link_libraries(llm_client_lib PUBLIC
    behaviortree_cpp
    httplib::httplib
    nlohmann_json::nlohmann_json
)
