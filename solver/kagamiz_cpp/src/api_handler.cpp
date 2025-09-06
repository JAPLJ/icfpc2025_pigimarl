#include "api_handler.hpp"
#include <iostream>
#include <stdexcept>

namespace kagamiz {

ApiHandler::ApiHandler(const std::string& api_domain, int request_timeout)
    : api_domain_(api_domain), request_timeout_(request_timeout) {
    
    // Parse the domain to extract host and port
    std::string host;
    int port = 80;
    bool is_https = false;
    
    if (api_domain.find("https://") == 0) {
        is_https = true;
        port = 443;
        host = api_domain.substr(8); // Remove "https://"
    } else if (api_domain.find("http://") == 0) {
        host = api_domain.substr(7); // Remove "http://"
    } else {
        host = api_domain;
    }
    
    // Remove trailing slash if present
    if (!host.empty() && host.back() == '/') {
        host.pop_back();
    }
    
    // Parse port number from host:port format
    size_t colon_pos = host.find(':');
    if (colon_pos != std::string::npos) {
        std::string port_str = host.substr(colon_pos + 1);
        try {
            port = std::stoi(port_str);
        } catch (const std::exception& e) {
            // If port parsing fails, use default port
            if (is_https) {
                port = 443;
            } else {
                port = 80;
            }
        }
        host = host.substr(0, colon_pos);
    }
    
    // Initialize client with parsed host and port
    if (is_https) {
        client_ = std::make_unique<httplib::Client>(host, port);
        client_->enable_server_certificate_verification(false);
    } else {
        client_ = std::make_unique<httplib::Client>(host, port);
    }
    
    client_->set_connection_timeout(0, request_timeout * 1000); // Convert to milliseconds
    client_->set_read_timeout(request_timeout, 0); // seconds, microseconds
}

SelectResponse ApiHandler::select(const SelectRequest& request) {
    std::string path = "/select";
    std::string json_data = nlohmann::json(request).dump();
    
    httplib::Headers headers = {
        {"Content-Type", "application/json"}
    };
    
    auto res = client_->Post(path.c_str(), headers, json_data, "application/json");
    
    if (!res) {
        std::string error_msg = "Failed to invoke /select endpoint: No response";
        if (res.error() == httplib::Error::Connection) {
            error_msg += " (Connection error)";
        } else if (res.error() == httplib::Error::Read) {
            error_msg += " (Read error)";
        } else if (res.error() == httplib::Error::Write) {
            error_msg += " (Write error)";
        } else if (res.error() == httplib::Error::ExceedRedirectCount) {
            error_msg += " (Exceed redirect count)";
        } else if (res.error() == httplib::Error::Canceled) {
            error_msg += " (Canceled)";
        } else if (res.error() == httplib::Error::SSLConnection) {
            error_msg += " (SSL connection error)";
        } else if (res.error() == httplib::Error::SSLLoadingCerts) {
            error_msg += " (SSL loading certs error)";
        } else if (res.error() == httplib::Error::SSLServerVerification) {
            error_msg += " (SSL server verification error)";
        } else if (res.error() == httplib::Error::UnsupportedMultipartBoundaryChars) {
            error_msg += " (Unsupported multipart boundary chars)";
        } else if (res.error() == httplib::Error::Compression) {
            error_msg += " (Compression error)";
        } else {
            error_msg += " (Unknown error: " + std::to_string(static_cast<int>(res.error())) + ")";
        }
        throw std::runtime_error(error_msg);
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
    
    auto res = client_->Post(path.c_str(), headers, json_data, "application/json");
    
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
    
    auto res = client_->Post(path.c_str(), headers, json_data, "application/json");
    
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