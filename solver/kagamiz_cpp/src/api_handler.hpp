#pragma once

#include "schema.hpp"
#include <string>
#include <httplib.h>

namespace kagamiz {

class ApiHandler {
private:
    std::string api_domain_;
    int request_timeout_;
    httplib::Client client_;
    
public:
    ApiHandler(const std::string& api_domain, int request_timeout);
    
    SelectResponse select(const SelectRequest& request);
    ExploreResponse explore(const ExploreRequest& request);
    GuessResponse guess(const GuessRequest& request);
};

} // namespace kagamiz
