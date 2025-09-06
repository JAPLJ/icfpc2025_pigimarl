#include "schema.hpp"
#include <stdexcept>

namespace kagamiz {

int get_problem_size(ProblemName problem_name) {
    switch (problem_name) {
        case ProblemName::PROBATIO: return 3;
        case ProblemName::PRIMUS: return 6;
        case ProblemName::SECUNDUS: return 12;
        case ProblemName::TERTIUS: return 18;
        case ProblemName::QUARTUS: return 24;
        case ProblemName::QUINTUS: return 30;
        default:
            throw std::invalid_argument("Unknown problem name");
    }
}

void to_json(nlohmann::json& j, const ProblemName& p) {
    switch (p) {
        case ProblemName::PROBATIO: j = "probatio"; break;
        case ProblemName::PRIMUS: j = "primus"; break;
        case ProblemName::SECUNDUS: j = "secundus"; break;
        case ProblemName::TERTIUS: j = "tertius"; break;
        case ProblemName::QUARTUS: j = "quartus"; break;
        case ProblemName::QUINTUS: j = "quintus"; break;
    }
}

void from_json(const nlohmann::json& j, ProblemName& p) {
    std::string name = j.get<std::string>();
    if (name == "probatio") p = ProblemName::PROBATIO;
    else if (name == "primus") p = ProblemName::PRIMUS;
    else if (name == "secundus") p = ProblemName::SECUNDUS;
    else if (name == "tertius") p = ProblemName::TERTIUS;
    else if (name == "quartus") p = ProblemName::QUARTUS;
    else if (name == "quintus") p = ProblemName::QUINTUS;
    else throw std::invalid_argument("Unknown problem name: " + name);
}

void to_json(nlohmann::json& j, const SelectRequest& req) {
    j = nlohmann::json{
        {"id", req.id},
        {"problemName", req.problem_name}
    };
}

void from_json(const nlohmann::json& j, SelectRequest& req) {
    j.at("id").get_to(req.id);
    j.at("problemName").get_to(req.problem_name);
}

void to_json(nlohmann::json& j, const SelectResponse& resp) {
    j = nlohmann::json{
        {"problemName", resp.problem_name}
    };
}

void from_json(const nlohmann::json& j, SelectResponse& resp) {
    j.at("problemName").get_to(resp.problem_name);
}

void to_json(nlohmann::json& j, const ExploreRequest& req) {
    j = nlohmann::json{
        {"id", req.id},
        {"plans", req.plans}
    };
}

void from_json(const nlohmann::json& j, ExploreRequest& req) {
    j.at("id").get_to(req.id);
    j.at("plans").get_to(req.plans);
}

void to_json(nlohmann::json& j, const ExploreResponse& resp) {
    j = nlohmann::json{
        {"results", resp.results},
        {"queryCount", resp.query_count}
    };
}

void from_json(const nlohmann::json& j, ExploreResponse& resp) {
    j.at("results").get_to(resp.results);
    j.at("queryCount").get_to(resp.query_count);
}

void to_json(nlohmann::json& j, const RoomDoor& rd) {
    j = nlohmann::json{
        {"room", rd.room},
        {"door", rd.door}
    };
}

void from_json(const nlohmann::json& j, RoomDoor& rd) {
    j.at("room").get_to(rd.room);
    j.at("door").get_to(rd.door);
}

void to_json(nlohmann::json& j, const Connection& conn) {
    j = nlohmann::json{
        {"from", conn.src},
        {"to", conn.dst}
    };
}

void from_json(const nlohmann::json& j, Connection& conn) {
    j.at("from").get_to(conn.src);
    j.at("to").get_to(conn.dst);
}

void to_json(nlohmann::json& j, const MapData& map) {
    j = nlohmann::json{
        {"rooms", map.rooms},
        {"startingRoom", map.starting_room},
        {"connections", map.connections}
    };
}

void from_json(const nlohmann::json& j, MapData& map) {
    j.at("rooms").get_to(map.rooms);
    j.at("startingRoom").get_to(map.starting_room);
    j.at("connections").get_to(map.connections);
}

void to_json(nlohmann::json& j, const GuessRequest& req) {
    j = nlohmann::json{
        {"id", req.id},
        {"map", req.map}
    };
}

void from_json(const nlohmann::json& j, GuessRequest& req) {
    j.at("id").get_to(req.id);
    j.at("map").get_to(req.map);
}

void to_json(nlohmann::json& j, const GuessResponse& resp) {
    j = nlohmann::json{
        {"correct", resp.correct}
    };
}

void from_json(const nlohmann::json& j, GuessResponse& resp) {
    j.at("correct").get_to(resp.correct);
}

} // namespace kagamiz