#include "solver.hpp"
#include "helper.hpp"
#include <iostream>
#include <stdexcept>
#include <openssl/evp.h>
#include <random>
#include <sstream>

namespace kagamiz {

std::string serialize_state(const State& state) {
    // Calculate SHA256 hash directly from State data
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    EVP_DigestInit_ex(ctx, md, nullptr);
    
    // Hash rooms data
    for (const auto& room : state.rooms) {
        // Hash room label
        EVP_DigestUpdate(ctx, &room.label, sizeof(room.label));
        // Hash doors array
        EVP_DigestUpdate(ctx, room.doors.data(), room.doors.size() * sizeof(int));
    }
    
    // Hash idx and current_room_idx
    EVP_DigestUpdate(ctx, &state.idx, sizeof(state.idx));
    EVP_DigestUpdate(ctx, &state.current_room_idx, sizeof(state.current_room_idx));
    
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);
    
    // Pre-allocate result string and build hex string directly
    std::string result;
    result.reserve(hash_len * 2); // Each byte becomes 2 hex chars
    
    // Use lookup table for hex conversion (faster than stringstream)
    static const char hex_chars[] = "0123456789abcdef";
    for (unsigned int i = 0; i < hash_len; i++) {
        result += hex_chars[(hash[i] >> 4) & 0xF];
        result += hex_chars[hash[i] & 0xF];
    }
    return result;
}

std::string format_vector(const std::vector<int>& vector) {
    std::stringstream ss;
    ss << "[";
    for (const auto& v : vector) {
        ss << v;
        if (v != vector.back()) {
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
    static int max_idx = 0;
    static int memo_size_checkpoint = 0;
    
    auto serialized_state = serialize_state(state);

    std::vector<int> label_cnt(4, 0);
    for (int i = 0; i < state.rooms.size(); i++) {
        if (state.rooms[i].label != -1) {
            label_cnt[state.rooms[i].label]++;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (label_cnt[i] > (n + 3) / 4) {
            memo[serialized_state] = nullptr;
            return nullptr;
        }
    }

    std::vector<int> room_in_degree(n, 0);
    for (const auto& room : state.rooms) {
        for (const auto& door : room.doors) {
            if (door != -1) {
                room_in_degree[door]++;
            }
        }
    }

    // 扉は 6 個しかないので、入次数が 6 を超える部屋は存在しない
    for (const auto& degree : room_in_degree) {
        if (degree > 6) {
            memo[serialized_state] = nullptr;
            return nullptr;
        }
    }

    // グラフ全体が連結でなければならない
    UnionFind uf(n);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 6; j++) {
            if (state.rooms[i].doors[j] != -1) {
                uf.unite(i, state.rooms[i].doors[j]);
            }
        }
    }
    std::vector<std::vector<int>> connected_components(n);
    for (int i = 0; i < n; i++) {
        connected_components[uf.find(i)].push_back(i);
    }

    for (const auto& connected_component : connected_components) {
        if (connected_component.size() == 0) {
            continue;
        }
        bool has_undetermined_door = false;
        for (const auto& room : connected_component) {
            for (int j = 0; j < 6; j++) {
                if (state.rooms[room].doors[j] == -1) {
                    has_undetermined_door = true;
                    break;
                }
            }
            if (has_undetermined_door) {
                break;
            }
        }
        static int counter = 0;
        if (!has_undetermined_door && connected_component.size() < n) {
            counter++;
            std::cout << "counter: " << counter << std::endl;
            memo[serialized_state] = nullptr;
            return nullptr;
        }
    }

    // 行き先が決まっているドアの情報から逆引きのための情報を作成
    std::vector<std::vector<int>> reverse_lookup(n);
    for (int i = 0; i < n; i++) {
        const auto& room = state.rooms[i];
        for (const auto& door : room.doors) {
            if (door != -1) {
                reverse_lookup[door].push_back(i);
            }
        }
    }
    {
    // 逆引き情報から行き先が決まっていないドアの情報の補完を試みる
    for (int i = 0; i < n; i++) {
        auto& room = state.rooms[i];

        bool used[6] = {false, false, false, false, false, false};
        for (const auto& expected_room: reverse_lookup[i]) {
            bool matching_door_found = false;
            for (int door_idx = 0; door_idx < 6; door_idx++) {
                if (used[door_idx]) {
                    continue;
                }
                if (room.doors[door_idx] == expected_room) {
                    used[door_idx] = true;
                    matching_door_found = true;
                    break;
                }
            }
            if (matching_door_found) {
                continue;
            }
            for (int door_idx = 0; door_idx < 6; door_idx++) {
                if (used[door_idx]) {
                    continue;
                }
                if (room.doors[door_idx] == -1) {
                    used[door_idx] = true;
                    matching_door_found = true;
                    break;
                }
            }
            if (!matching_door_found) {
//                    std::cout << "matching_door_found is false" << std::endl;
                memo[serialized_state] = nullptr;
                return nullptr;
            }
        }
    }
}

    if (state.idx > max_idx) {
        max_idx = state.idx;
        std::cout << "max_idx: " << max_idx << std::endl;
//        std::cout << format_rooms(state.rooms) << std::endl;
    }
    
    if (state.idx == static_cast<int>(doors.length())) {
        auto rooms = state.rooms;
//        std::cout << "dfs completed(rooms=" << format_rooms(rooms) << ")" << std::endl;

        if (connected_components[uf.find(0)].size() != n) {
            memo[serialized_state] = nullptr;
            return nullptr;
        }
        
        std::unordered_map<int, int> label_cnt;
        for (const auto& room : rooms) {
            if (room.label == -1) {
//                std::cout << "room.label == -1" << std::endl;
                memo[serialized_state] = nullptr;
                return nullptr;
            }
            label_cnt[room.label]++;
        }
        
        // Check if labels are evenly distributed
        for (const auto& [label, cnt] : label_cnt) {
            if (cnt != n / 4 && cnt != (n + 3) / 4) {
//                std::cout << "label_cnt[" << label << "] = " << cnt << " is not " << n / 4 << " or " << (n + 3) / 4 << std::endl;
                memo[serialized_state] = nullptr;
                return nullptr;
            }
        }
        // 行き先が決まっているドアの情報から逆引きのための情報を作成
        std::vector<std::vector<int>> reverse_lookup(n);
        for (int i = 0; i < n; i++) {
            const auto& room = rooms[i];
            for (const auto& door : room.doors) {
                if (door != -1) {
                    reverse_lookup[door].push_back(i);
                }
            }
        }
        // 逆引き情報から行き先が決まっていないドアの情報の補完を試みる
        for (int i = 0; i < n; i++) {
            auto& room = rooms[i];

            bool used[6] = {false, false, false, false, false, false};
            for (const auto& expected_room: reverse_lookup[i]) {
                bool matching_door_found = false;
                for (int door_idx = 0; door_idx < 6; door_idx++) {
                    if (used[door_idx]) {
                        continue;
                    }
                    if (room.doors[door_idx] == expected_room) {
                        used[door_idx] = true;
                        matching_door_found = true;
                        break;
                    }
                }
                if (matching_door_found) {
                    continue;
                }
                for (int door_idx = 0; door_idx < 6; door_idx++) {
                    if (used[door_idx]) {
                        continue;
                    }
                    if (room.doors[door_idx] == -1) {
                        room.doors[door_idx] = expected_room;
                        used[door_idx] = true;
                        matching_door_found = true;
                        room_in_degree[expected_room]++;
                        if (room_in_degree[expected_room] > 6) {
//                            std::cout << "room_in_degree[" << expected_room << "] > 6" << std::endl;
                            memo[serialized_state] = nullptr;
                            return nullptr;
                        }
                        break;
                    }
                }
                if (!matching_door_found) {
//                    std::cout << "matching_door_found is false" << std::endl;
                    memo[serialized_state] = nullptr;
                    return nullptr;
                }
            }
        }
        // まだ行き先が決まっていないドアを列挙 (部屋番号, ドア番号)
        std::vector<int> undetermined_rooms;
        std::vector<std::pair<int, int>> undetermined_doors;
        for (int i = 0; i < n; i++) {
            for (int door_idx = 0; door_idx < 6; door_idx++) {
                if (rooms[i].doors[door_idx] == -1) {
                    undetermined_rooms.push_back(i);
                    undetermined_doors.push_back(std::make_pair(i, door_idx));
                }
            }
        }
        std::shuffle(undetermined_rooms.begin(), undetermined_rooms.end(), std::mt19937(std::random_device()()));
        for (int i = 0; i < undetermined_rooms.size(); i++) {
            const auto& [room_idx, door_idx] = undetermined_doors[i];
            rooms[room_idx].doors[door_idx] = undetermined_rooms[i];
            room_in_degree[undetermined_rooms[i]]++;
        }
        std::vector<int> room_labels;
        for (const auto& room : rooms) {
            room_labels.push_back(room.label);
        }
        auto result = std::make_shared<MapData>(MapData{
            .rooms = room_labels,
            .starting_room = 0,
            .connections = create_connections(rooms)
        });
        std::cout << "dfs completed(rooms=" << format_rooms(rooms) << ")" << std::endl;
        
        memo[serialized_state] = result;
        return result;
    }
    
    // Move from current room through door doors[idx] to room with label labels[idx+1]
    int door_idx = doors[state.idx] - '0';

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
            break;
        }
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