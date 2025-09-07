#pragma once

#include "schema.hpp"
#include <vector>
#include <string>

namespace kagamiz {

struct Room {
    int label;
    std::vector<int> doors;
    
    Room(int label, const std::vector<int>& doors) : label(label), doors(doors) {}
};

std::vector<Connection> create_connections(const std::vector<Room>& rooms);
std::string format_rooms(const std::vector<Room>& rooms);

} // namespace kagamiz
