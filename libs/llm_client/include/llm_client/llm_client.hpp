#pragma once
#include <string>

// Thin wrapper over an OpenAI-compatible /chat/completions endpoint (Ollama,
// llama.cpp, OpenAI, ...). It owns the system prompt that constrains the model
// to emit BT.CPP XML; callers supply only the user turn. virtual so tests can
// subclass it with a FakeLLMClient that returns canned responses.
class LLMClient {
public:
    // Reads OPENAI_BASE_URL (default: http://localhost:11434/v1),
    // OPENAI_API_KEY (default: empty), LLM_MODEL (default: granite4.1:3b-q8_0)
    LLMClient();

    // For testing: explicit configuration
    LLMClient(std::string base_url, std::string api_key, std::string model);

    // Sends a chat completion request with user_message as the user turn.
    // Returns the assistant message content.
    // Throws LLMConnectionError on network failure or non-200 response.
    virtual std::string complete(const std::string& user_message);

    virtual ~LLMClient() = default;

private:
    std::string base_url_;
    std::string api_key_;
    std::string model_;

    static std::string system_prompt();
    std::string origin() const;    // e.g. "http://localhost:11434"
    std::string base_path() const; // e.g. "/v1"
};
