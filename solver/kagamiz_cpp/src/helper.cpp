#include "helper.hpp"
#include <set>
#include <sstream>

namespace kagamiz {

std::vector<Connection> create_connections(const std::vector<Room>& rooms) {
    int n = rooms.size();
    std::set<std::pair<int, int>> done;
    std::vector<Connection> connections;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 6; j++) {
            auto key = std::make_pair(i, j);
            if (done.contains(key)) {
                continue;
            }
            done.insert(key);
            
            int dst = rooms[i].doors[j];
            int dst_door = -1;
            for (int k = 0; k < 6; k++) {
                if (!done.contains(std::make_pair(dst, k)) && rooms[dst].doors[k] == i) {
                    dst_door = k;
                    break;
                }
            }
            
            if (dst_door == -1) {
                // 自己ループのケース
                if (i == dst) {
                    dst_door = j;
                }
                else {
                    throw std::runtime_error("Logic Error");
                }
            }
            
            done.insert(std::make_pair(dst, dst_door));
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

} // namespace kagamiz
