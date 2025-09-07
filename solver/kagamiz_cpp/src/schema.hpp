#pragma once

#include <string>
#include <vector>
#include "nlohmann_json.hpp"

namespace kagamiz {

enum class ProblemName {
    PROBATIO,
    PRIMUS,
    SECUNDUS,
    TERTIUS,
    QUARTUS,
    QUINTUS
};

struct ApiDomain {
    std::string local;
    std::string production;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ApiDomain, local, production)
};

struct Config {
    ApiDomain api_domain;
    std::string token;
    int request_timeout;
    std::string solver_type;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Config, api_domain, token, request_timeout, solver_type)
};

int get_problem_size(ProblemName problem_name);

struct SelectRequest {
    std::string id;
    ProblemName problem_name;
};

struct SelectResponse {
    ProblemName problem_name;
};

struct ExploreRequest {
    std::string id;
    std::vector<std::string> plans;
};

struct ExploreResponse {
    std::vector<std::vector<int>> results;
    int query_count;
};

struct RoomDoor {
    int room;
    int door;
};

struct Connection {
    RoomDoor src;
    RoomDoor dst;
};

struct MapData {
    std::vector<int> rooms;
    int starting_room;
    std::vector<Connection> connections;
};

struct GuessRequest {
    std::string id;
    MapData map;
};

struct GuessResponse {
    bool correct;
};

// JSON serialization helpers
void to_json(nlohmann::json& j, const ProblemName& p);
void from_json(const nlohmann::json& j, ProblemName& p);

void to_json(nlohmann::json& j, const SelectRequest& req);
void from_json(const nlohmann::json& j, SelectRequest& req);

void to_json(nlohmann::json& j, const SelectResponse& resp);
void from_json(const nlohmann::json& j, SelectResponse& resp);

void to_json(nlohmann::json& j, const ExploreRequest& req);
void from_json(const nlohmann::json& j, ExploreRequest& req);

void to_json(nlohmann::json& j, const ExploreResponse& resp);
void from_json(const nlohmann::json& j, ExploreResponse& resp);

void to_json(nlohmann::json& j, const RoomDoor& rd);
void from_json(const nlohmann::json& j, RoomDoor& rd);

void to_json(nlohmann::json& j, const Connection& conn);
void from_json(const nlohmann::json& j, Connection& conn);

void to_json(nlohmann::json& j, const MapData& map);
void from_json(const nlohmann::json& j, MapData& map);

void to_json(nlohmann::json& j, const GuessRequest& req);
void from_json(const nlohmann::json& j, GuessRequest& req);

void to_json(nlohmann::json& j, const GuessResponse& resp);
void from_json(const nlohmann::json& j, GuessResponse& resp);

} // namespace kagamiz
