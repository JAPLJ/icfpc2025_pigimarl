#include "api_handler.hpp"
#include <iostream>
#include <stdexcept>

namespace kagamiz {

ApiHandler::ApiHandler(const std::string& api_domain, int request_timeout)
    : api_domain_(api_domain), request_timeout_(request_timeout), client_(api_domain) {
    client_.set_connection_timeout(0, request_timeout * 1000); // Convert to milliseconds
    client_.set_read_timeout(request_timeout, 0); // seconds, microseconds
}

SelectResponse ApiHandler::select(const SelectRequest& request) {
    std::string path = "/select";
    std::string json_data = nlohmann::json(request).dump();
    
    httplib::Headers headers = {
        {"Content-Type", "application/json"}
    };
    
    auto res = client_.Post(path.c_str(), headers, json_data, "application/json");
    
    if (!res) {
        throw std::runtime_error("Failed to invoke /select endpoint: No response");
    }
    
    if (res->status != 200) {
        throw std::runtime_error("Failed to invoke /select endpoint: HTTP " + 
                                std::to_string(res->status) + " - " + res->body);
    }
    
    try {
        nlohmann::json j = nlohmann::json::parse(res->body);
        return j.get<SelectResponse>();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse select response: " + std::string(e.what()));
    }
}

ExploreResponse ApiHandler::explore(const ExploreRequest& request) {
    std::string path = "/explore";
    std::string json_data = nlohmann::json(request).dump();
    
    httplib::Headers headers = {
        {"Content-Type", "application/json"}
    };
    
    auto res = client_.Post(path.c_str(), headers, json_data, "application/json");
    
    if (!res) {
        throw std::runtime_error("Failed to invoke /explore endpoint: No response");
    }
    
    if (res->status != 200) {
        throw std::runtime_error("Failed to invoke /explore endpoint: HTTP " + 
                                std::to_string(res->status) + " - " + res->body);
    }
    
    try {
        nlohmann::json j = nlohmann::json::parse(res->body);
        return j.get<ExploreResponse>();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse explore response: " + std::string(e.what()));
    }
}

GuessResponse ApiHandler::guess(const GuessRequest& request) {
    std::string path = "/guess";
    std::string json_data = nlohmann::json(request).dump();
    
    httplib::Headers headers = {
        {"Content-Type", "application/json"}
    };
    
    auto res = client_.Post(path.c_str(), headers, json_data, "application/json");
    
    if (!res) {
        throw std::runtime_error("Failed to invoke /guess endpoint: No response");
    }
    
    if (res->status != 200) {
        throw std::runtime_error("Failed to invoke /guess endpoint: HTTP " + 
                                std::to_string(res->status) + " - " + res->body);
    }
    
    try {
        nlohmann::json j = nlohmann::json::parse(res->body);
        return j.get<GuessResponse>();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse guess response: " + std::string(e.what()));
    }
}

} // namespace kagamiz