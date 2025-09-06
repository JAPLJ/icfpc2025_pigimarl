#pragma once

#include "schema.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace kagamiz {

struct Room {
    int label;
    std::vector<int> doors;
    
    Room(int label, const std::vector<int>& doors) : label(label), doors(doors) {}
};

struct State {
    std::vector<Room> rooms;
    int idx;
    int current_room_idx;
    
    State(const std::vector<Room>& rooms, int idx, int current_room_idx)
        : rooms(rooms), idx(idx), current_room_idx(current_room_idx) {}
};

std::string serialize_state(const State& state);
std::vector<Connection> create_connections(const std::vector<Room>& rooms);
MapData solve(int n, const std::string& doors, const std::vector<int>& labels);

} // namespace kagamiz
