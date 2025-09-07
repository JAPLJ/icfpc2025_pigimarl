#pragma once

#include "helper.hpp"
#include <string>
#include <vector>

namespace kagamiz {

struct State {
    std::vector<Room> rooms;
    int idx;
    int current_room_idx;
    
    State(const std::vector<Room>& rooms, int idx, int current_room_idx)
        : rooms(rooms), idx(idx), current_room_idx(current_room_idx) {}
};


std::string serialize_state(const State& state);
MapData solve(int n, const std::string& doors, const std::vector<int>& labels);

} // namespace kagamiz
