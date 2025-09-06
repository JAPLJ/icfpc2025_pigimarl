#include "solver.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <openssl/evp.h>

namespace kagamiz {

std::string serialize_state(const State& state) {
    nlohmann::json j;
    j["rooms"] = nlohmann::json::array();
    for (const auto& room : state.rooms) {
        nlohmann::json room_json;
        room_json["label"] = room.label;
        room_json["doors"] = room.doors;
        j["rooms"].push_back(room_json);
    }
    j["idx"] = state.idx;
    j["current_room_idx"] = state.current_room_idx;
    
    std::string json_str = j.dump();
    
    // Calculate SHA256 hash using EVP interface
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, json_str.c_str(), json_str.length());
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Custom hash function for std::pair<int, int>
struct PairHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

std::vector<Connection> create_connections(const std::vector<Room>& rooms) {
    int n = rooms.size();
    std::unordered_map<std::pair<int, int>, bool, PairHash> done;
    std::vector<Connection> connections;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 6; j++) {
            auto key = std::make_pair(i, j);
            if (done[key]) {
                continue;
            }
            done[key] = true;
            
            int dst = rooms[i].doors[j];
            int dst_door = -1;
            for (int k = 0; k < 6; k++) {
                if (rooms[dst].doors[k] == i) {
                    dst_door = k;
                    break;
                }
            }
            
            if (dst_door == -1) {
                throw std::runtime_error("Logic Error");
            }
            
            done[std::make_pair(dst, dst_door)] = true;
            connections.push_back(Connection{
                .src = RoomDoor{.room = i, .door = j},
                .dst = RoomDoor{.room = dst, .door = dst_door}
            });
        }
    }
    
    return connections;
}

std::string format_rooms(const std::vector<Room>& rooms) {
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < rooms.size(); i++) {
        const auto& room = rooms[i];
        ss << "{\"label\": " << room.label << ", \"doors\": [";
        ss << "[";
        for (int i = 0; i < room.doors.size(); i++) {
            ss << room.doors[i];
            if (i != room.doors.size() - 1) {
                ss << ",";
            }
        }
        ss << "]}";
        if (i != rooms.size() - 1) {
            ss << ",";
        }
    }
    ss << "]";
    return ss.str();
}

std::shared_ptr<MapData> dfs(
    int n, 
    const std::string& doors, 
    const std::vector<int>& labels, 
    const State& state, 
    std::unordered_map<std::string, std::shared_ptr<MapData>>& memo
) {
    std::string serialized_state = serialize_state(state);
    
    if (state.idx == static_cast<int>(doors.length())) {
        std::cout << "dfs completed(rooms=" << format_rooms(state.rooms) << ")" << std::endl;
        
        std::unordered_map<int, int> label_cnt;
        for (const auto& room : state.rooms) {
            if (room.label == -1) {
                memo[serialized_state] = nullptr;
                return nullptr;
            }
            label_cnt[room.label]++;
            for (int door : room.doors) {
                if (door == -1) {
                    memo[serialized_state] = nullptr;
                    return nullptr;
                }
            }
        }
        
        // Check if labels are evenly distributed
        for (const auto& [label, cnt] : label_cnt) {
            if (cnt != n / 4 && cnt != (n + 3) / 4) {
                memo[serialized_state] = nullptr;
                return nullptr;
            }
        }
        
        std::vector<int> room_labels;
        for (const auto& room : state.rooms) {
            room_labels.push_back(room.label);
        }
        
        auto result = std::make_shared<MapData>(MapData{
            .rooms = room_labels,
            .starting_room = 0,
            .connections = create_connections(state.rooms)
        });
        
        memo[serialized_state] = result;
        return result;
    }
    
    // Move from current room through door doors[idx] to room with label labels[idx+1]
    int door_idx = doors[state.idx] - '0';
    if (state.rooms[state.current_room_idx].doors[door_idx] != -1) {
        int next_room_idx = state.rooms[state.current_room_idx].doors[door_idx];
        if (state.rooms[next_room_idx].label == labels[state.idx + 1]) {
            State next_state(state.rooms, state.idx + 1, next_room_idx);
            std::string next_state_serialized = serialize_state(next_state);
            
            if (memo.find(next_state_serialized) == memo.end()) {
                memo[next_state_serialized] = dfs(n, doors, labels, next_state, memo);
            }
            
            if (memo[next_state_serialized] != nullptr) {
                return memo[next_state_serialized];
            }
        }
        memo[serialized_state] = nullptr;
        return nullptr;
    }
    
    // Find next room candidates
    int next_room_candidates_count = 0;
    for (int next_room_idx = 0; next_room_idx < n; next_room_idx++) {
        if (state.rooms[next_room_idx].label != labels[state.idx + 1]) {
            continue;
        }
        next_room_candidates_count++;
        
        State next_state(state.rooms, state.idx + 1, next_room_idx);
        next_state.rooms[state.current_room_idx].doors[door_idx] = next_room_idx;
        
        std::string next_state_serialized = serialize_state(next_state);
        if (memo.find(next_state_serialized) == memo.end()) {
            memo[next_state_serialized] = dfs(n, doors, labels, next_state, memo);
        }
        
        if (memo[next_state_serialized] != nullptr) {
            return memo[next_state_serialized];
        }
        next_state.rooms[state.current_room_idx].doors[door_idx] = -1;
    }
    
    if (next_room_candidates_count >= (n + 3) / 4) {
        memo[serialized_state] = nullptr;
        return nullptr;
    }
    
    for (int next_room_idx = 0; next_room_idx < n; next_room_idx++) {
        if (state.rooms[next_room_idx].label == -1) {
            State next_state(state.rooms, state.idx + 1, next_room_idx);
            next_state.rooms[next_room_idx].label = labels[state.idx + 1];
            next_state.rooms[state.current_room_idx].doors[door_idx] = next_room_idx;
            
            std::string next_state_serialized = serialize_state(next_state);
            if (memo.find(next_state_serialized) == memo.end()) {
                memo[next_state_serialized] = dfs(n, doors, labels, next_state, memo);
            }
            
            if (memo[next_state_serialized] != nullptr) {
                return memo[next_state_serialized];
            }
            next_state.rooms[next_room_idx].label = -1;
            next_state.rooms[state.current_room_idx].doors[door_idx] = -1;
        }
    }
    
    memo[serialized_state] = nullptr;
    return nullptr;
}

MapData solve(int n, const std::string& doors, const std::vector<int>& labels) {
    std::vector<Room> rooms;
    for (int i = 0; i < n; i++) {
        rooms.emplace_back(-1, std::vector<int>(6, -1));
    }
    rooms[0].label = labels[0];
    
    std::unordered_map<std::string, std::shared_ptr<MapData>> memo;
    State initial_state(rooms, 0, 0);
    auto result = dfs(n, doors, labels, initial_state, memo);
    
    if (result == nullptr) {
        throw std::runtime_error("No solution found");
    }
    
    return *result;
}

} // namespace kagamiz
