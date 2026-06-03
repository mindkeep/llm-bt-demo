#pragma once
#include <stdexcept>
#include <string>

class LLMConnectionError : public std::runtime_error {
public:
    explicit LLMConnectionError(const std::string& msg)
        : std::runtime_error("LLM connection error: " + msg) {}
};

class BTXMLParseError : public std::runtime_error {
public:
    std::string raw_xml;
    explicit BTXMLParseError(const std::string& msg, std::string xml = "")
        : std::runtime_error("BT XML parse error: " + msg), raw_xml(std::move(xml)) {}
};
