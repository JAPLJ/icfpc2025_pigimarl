#pragma once

#include "helper.hpp"
#include <set>

namespace kagamiz {

struct State2 {
    std::vector<Room> rooms;
    std::vector<std::vector<std::pair<int, int>>> pair_doors;
    int start_room_idx;

    State2(const std::vector<Room>& rooms, int start_room_idx, const std::vector<std::vector<std::pair<int, int>>>& pair_doors)
        : rooms(rooms), start_room_idx(start_room_idx), pair_doors(pair_doors) {};
};

struct Feedback {
    double score;
    int right_count;
    std::vector<int> mistakes;
    std::set<std::pair<int, int>> right_doors;
};

State2 create_random_state2(int n, const std::string& doors, const std::vector<int>& labels);
Feedback calculate_score(const State2& state, const std::string& doors, const std::vector<int>& labels);
State2 mutate(const State2& state, const std::vector<int>& labels, const std::string& doors, const std::vector<int>& mistakes, const std::set<std::pair<int, int>>& right_doors);
std::string format_right_doors(const std::set<std::pair<int, int>>& right_doors);
std::string format_mistakes(const std::vector<int>& mistakes);
MapData solve2(int n, const std::string& doors, const std::vector<int>& labels);

} // namespace kagamiz
