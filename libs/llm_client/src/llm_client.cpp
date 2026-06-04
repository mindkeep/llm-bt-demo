#include "llm_client/llm_client.hpp"
#include "llm_client/errors.hpp"

#include <cstdlib>
#include <httplib.h>
#include <nlohmann/json.hpp>

static std::string env_or(const char* name, const char* fallback) {
    const char* v = std::getenv(name);
    return v ? v : fallback;
}

LLMClient::LLMClient()
    : base_url_(env_or("OPENAI_BASE_URL", "http://localhost:11434/v1"))
    , api_key_(env_or("OPENAI_API_KEY", ""))
    , model_(env_or("LLM_MODEL", "llama3.2"))
{
    if (!base_url_.empty() && base_url_.back() == '/') {
        base_url_.pop_back();
    }
}

LLMClient::LLMClient(std::string base_url, std::string api_key, std::string model)
    : base_url_(std::move(base_url))
    , api_key_(std::move(api_key))
    , model_(std::move(model))
{
    if (!base_url_.empty() && base_url_.back() == '/') {
        base_url_.pop_back();
    }
}

std::string LLMClient::origin() const {
    auto scheme_end = base_url_.find("://");
    if (scheme_end == std::string::npos) return base_url_;
    auto path_start = base_url_.find('/', scheme_end + 3);
    return path_start == std::string::npos ? base_url_ : base_url_.substr(0, path_start);
}

std::string LLMClient::base_path() const {
    auto scheme_end = base_url_.find("://");
    if (scheme_end == std::string::npos) return "";
    auto path_start = base_url_.find('/', scheme_end + 3);
    return path_start == std::string::npos ? "" : base_url_.substr(path_start);
}

std::string LLMClient::system_prompt() {
    return R"(You are a behavior tree planner for a robot arm. Output ONLY valid BT.CPP v4 XML.
Available nodes:

CONDITIONS (return SUCCESS/FAILURE, no side effects):
  <IsObjectAt object="ObjectA|ObjectB|ObjectC" location="TableA|TableB|TableC|ArmReach"/>
  <IsGripperOpen/>
  <IsArmNear location="TableA|TableB|TableC"/>

ACTIONS (return RUNNING then SUCCESS):
  <MoveArmTo location="TableA|TableB|TableC"/>
  <OpenGripper/>
  <CloseGripper/>
  <PickObject object="ObjectA|ObjectB|ObjectC"/>
  <PlaceObject object="ObjectA|ObjectB|ObjectC" location="TableA|TableB|TableC"/>

COMPOSITES (BT.CPP built-ins):
  <Sequence> <Fallback> <Parallel>

DECORATORS (BT.CPP built-ins):
  <Retry num_attempts="N"> <Timeout msec="N">

Rules:
- The <root> element MUST have the attribute BTCPP_format="4". Example: <root BTCPP_format="4">
- Output a single <root BTCPP_format="4"> element with one <BehaviorTree ID="Main"> child.
- No markdown, no explanation, no code fences. XML only.)";
}

std::string LLMClient::complete(const std::string& user_message) {
    nlohmann::json body = {
        {"model", model_},
        {"messages", {
            {{"role", "system"}, {"content", system_prompt()}},
            {{"role", "user"}, {"content", user_message}}
        }},
        {"temperature", 0.0}
    };

    httplib::Client cli(origin());
    cli.set_connection_timeout(10);
    cli.set_read_timeout(60);

    httplib::Headers headers;
    if (!api_key_.empty()) {
        headers.insert({"Authorization", "Bearer " + api_key_});
    }

    auto res = cli.Post(base_path() + "/chat/completions",
                        headers, body.dump(), "application/json");

    if (!res) {
        throw LLMConnectionError("no response from " + origin());
    }
    if (res->status != 200) {
        throw LLMConnectionError("HTTP " + std::to_string(res->status) + ": " + res->body);
    }

    nlohmann::json resp;
    try {
        resp = nlohmann::json::parse(res->body);
    } catch (const nlohmann::json::exception& e) {
        throw LLMConnectionError("invalid JSON response: " + std::string(e.what()));
    }

    try {
        return resp.at("choices").at(0).at("message").at("content").get<std::string>();
    } catch (const nlohmann::json::exception& e) {
        throw LLMConnectionError("unexpected response structure: " + std::string(e.what()));
    }
}
