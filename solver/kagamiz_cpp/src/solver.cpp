#include "solver.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <openssl/evp.h>
#include <set>
#include <random>
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

std::shared_ptr<MapData> dfs(
    int n, 
    const std::string& doors, 
    const std::vector<int>& labels, 
    const State& state, 
    std::unordered_map<std::string, std::shared_ptr<MapData>>& memo
) {
    static int max_idx = 0;
    static int memo_size_checkpoint = 0;
    if (state.idx > max_idx) {
        max_idx = state.idx;
        std::cout << "max_idx: " << max_idx << std::endl;
    }
    if (memo.size() > memo_size_checkpoint) {
        std::cout << "memo_size_checkpoint: " << memo_size_checkpoint << std::endl;
        memo_size_checkpoint += 100000;
    }
    std::string serialized_state = serialize_state(state);

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
    
    if (state.idx == static_cast<int>(doors.length())) {
        auto rooms = state.rooms;
        std::cout << "dfs completed(rooms=" << format_rooms(rooms) << ")" << std::endl;
        
        std::unordered_map<int, int> label_cnt;
        for (const auto& room : rooms) {
            if (room.label == -1) {
                memo[serialized_state] = nullptr;
                return nullptr;
            }
            label_cnt[room.label]++;
        }
        
        // Check if labels are evenly distributed
        for (const auto& [label, cnt] : label_cnt) {
            if (cnt != n / 4 && cnt != (n + 3) / 4) {
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
                            memo[serialized_state] = nullptr;
                            return nullptr;
                        }
                        break;
                    }
                }
                if (!matching_door_found) {
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

State2 create_random_state2(int n) {
    std::vector<Room> rooms;
    for (int i = 0; i < n; i++) {
        rooms.emplace_back(-1, std::vector<int>(6, -1));
    }
    for (int i = 0; i < n; i++) {
        rooms[i].label = i % 4;
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, n - 1);
    int start_room_idx = dis(gen);
    std::vector<std::vector<std::pair<int, int>>> pair_doors(n, std::vector<std::pair<int, int>>(6, std::make_pair(-1, -1)));
    std::vector<std::pair<int, int>> room_and_door;
    std::vector<bool> used;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 6; j++) {
            room_and_door.push_back(std::make_pair(i, j));
            used.push_back(false);
        }
    }
    for (int i = 0; i < room_and_door.size(); i++) {
        const auto& [room_idx, door_idx] = room_and_door[i];
        if (used[i]) {
            continue;
        }
        std::uniform_int_distribution<> dis2(i, room_and_door.size() - 1);
        while (true) {
            int j = dis2(gen);
            if (used[j]) {
                continue;
            }
            const auto& [room_idx2, door_idx2] = room_and_door[j];
            pair_doors[room_idx][door_idx] = std::make_pair(room_idx2, door_idx2);
            pair_doors[room_idx2][door_idx2] = std::make_pair(room_idx, door_idx);
            rooms[room_idx].doors[door_idx] = room_idx2;
            rooms[room_idx2].doors[door_idx2] = room_idx;
            used[j] = true;
            break;
        }
    }
    return State2(rooms, start_room_idx, pair_doors);
}

struct Feedback {
    double score;
    int right_count;
    std::vector<int> mistakes;
    std::set<std::pair<int, int>> right_doors;
};

Feedback calculate_score(const State2& state, const std::string& doors, const std::vector<int>& labels) {
    double score = 0;
    int right_count = 0;
    int current_room_idx = state.start_room_idx;
    std::vector<int> mistakes;
    std::set<std::pair<int, int>> right_doors;
    std::set<std::pair<int, int>> wrong_doors;
    if (state.rooms[current_room_idx].label == labels[0]) {
        score = 1;
        right_count++;
    }
    else {
        score = -1000000000;
    }
    for (int i = 0; i < doors.length(); i++) {
        int door_idx = doors[i] - '0';
        if (state.rooms[current_room_idx].doors[door_idx] == labels[i + 1]) {
            score++;
            right_doors.insert(std::make_pair(current_room_idx, door_idx));
            right_doors.insert(state.pair_doors[current_room_idx][door_idx]);
            right_count++;
        }
        else {
            score--;
            mistakes.push_back(i);
            wrong_doors.insert(std::make_pair(current_room_idx, door_idx));
            wrong_doors.insert(state.pair_doors[current_room_idx][door_idx]);
        }
        current_room_idx = state.rooms[current_room_idx].doors[door_idx];
    }
    for (const auto& [room_idx, door_idx] : wrong_doors) {
        right_doors.erase(std::make_pair(room_idx, door_idx));
    }
    score += 0.1 * (right_doors.size());
    return Feedback{
        .score = score,
        .right_count = right_count,
        .mistakes = mistakes,
        .right_doors = right_doors
    };
}

State2 mutate(const State2& state, const std::vector<int>& labels, const std::string& doors, const std::vector<int>& mistakes, const std::set<std::pair<int, int>>& right_doors) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0, 999);
    int p = dis(gen);
    // 1% の確率で完全なる突然変異を起こす
    if (p < 10) {
        return create_random_state2(state.rooms.size());
    }
    // 9% の確率でランダムに部屋を繋ぎ変え
    else if (p < 100) {
        std::vector<std::pair<int, int>> room_and_door_candidates;

        int pp = dis(gen);
        for (int i = 0; i < state.rooms.size(); i++) {
            for (int j = 0; j < 6; j++) {
                if (!right_doors.contains(std::make_pair(i, j)) || pp < 500) {
                    room_and_door_candidates.push_back(std::make_pair(i, j));
                }
            }
        }
        if (room_and_door_candidates.size() <= 2) {
            return state;
        }
        std::shuffle(room_and_door_candidates.begin(), room_and_door_candidates.end(), gen);
        int room_idx1 = room_and_door_candidates[0].first;
        int door_idx1 = room_and_door_candidates[0].second;
        int room_idx2 = room_and_door_candidates[1].first;
        int door_idx2 = room_and_door_candidates[1].second;
        const auto& [room_idx3, door_idx3] = state.pair_doors[room_idx1][door_idx1];
        const auto& [room_idx4, door_idx4] = state.pair_doors[room_idx2][door_idx2];
        auto new_state = state;
        new_state.pair_doors[room_idx1][door_idx1] = std::make_pair(room_idx2, door_idx2);
        new_state.pair_doors[room_idx2][door_idx2] = std::make_pair(room_idx1, door_idx1);
        new_state.pair_doors[room_idx3][door_idx3] = std::make_pair(room_idx4, door_idx4);
        new_state.pair_doors[room_idx4][door_idx4] = std::make_pair(room_idx3, door_idx3);
        new_state.rooms[room_idx1].doors[door_idx1] = room_idx2;
        new_state.rooms[room_idx2].doors[door_idx2] = room_idx1;
        new_state.rooms[room_idx3].doors[door_idx3] = room_idx4;
        new_state.rooms[room_idx4].doors[door_idx4] = room_idx3;
        return new_state;
    }
    // 85% の確率で間違えたドアを修正
    else if (p < 950) {
        std::uniform_int_distribution<> dis_mistake(0, mistakes.size() - 1);
        int mistake_idx = dis_mistake(gen);
        std::vector<int> room_transition_history;
        int current_room_idx = state.start_room_idx;
        room_transition_history.push_back(current_room_idx);
        for (int i = 0; i < doors.length(); i++) {
            int door_idx = doors[i] - '0';
            current_room_idx = state.rooms[current_room_idx].doors[door_idx];
            room_transition_history.push_back(current_room_idx);
        }
        // i 回目のドア通過時の行き先が間違っていた。正しいラベルは labels[i + 1]。
        int mistake_door_idx = mistakes[mistake_idx];
        std::vector<std::pair<int, int>> new_room_and_door_candidates;

        int pp = dis(gen);
        for (int i = 0; i < state.rooms.size(); i++) {
            if (state.rooms[i].label == labels[mistake_door_idx + 1]) {
                for (int j = 0; j < 6; j++) {
                    if (!right_doors.contains(std::make_pair(i, j)) || pp < 500) {
                        new_room_and_door_candidates.push_back(std::make_pair(i, j));
                    }
                }
            }
        }
        if (new_room_and_door_candidates.size() == 0) {
            return state;
        }
        std::uniform_int_distribution<> dis_new_room_and_door(0, new_room_and_door_candidates.size() - 1);

        int room_idx = room_transition_history[mistake_idx + 1];
        int door_idx = doors[mistake_idx] - '0';
        const auto& [room_idx2, door_idx2] = new_room_and_door_candidates[dis_new_room_and_door(gen)];
        const auto& [room_idx3, door_idx3] = state.pair_doors[room_idx][door_idx];
        const auto& [room_idx4, door_idx4] = state.pair_doors[room_idx2][door_idx2];
        auto new_state = state;
        // 1, 2 と 3, 4 を pair にするように書き換える。
        new_state.pair_doors[room_idx][door_idx] = std::make_pair(room_idx2, door_idx2);
        new_state.pair_doors[room_idx2][door_idx2] = std::make_pair(room_idx, door_idx);
        new_state.pair_doors[room_idx3][door_idx3] = std::make_pair(room_idx4, door_idx4);
        new_state.pair_doors[room_idx4][door_idx4] = std::make_pair(room_idx3, door_idx3);
        new_state.rooms[room_idx].doors[door_idx] = room_idx2;
        new_state.rooms[room_idx2].doors[door_idx2] = room_idx;
        new_state.rooms[room_idx3].doors[door_idx3] = room_idx4;
        new_state.rooms[room_idx4].doors[door_idx4] = room_idx3;
        return new_state;
    }
    // 2.5% の確率で初期位置を変更
    else if (p < 975) {
        std::vector<int> room_idx_candidates;
        for (int i = 0; i < state.rooms.size(); i++) {
            if (state.rooms[i].label == labels[0]) {
                room_idx_candidates.push_back(i);
            }
        }
        auto new_state = state;

        std::uniform_int_distribution<> dis2(0, room_idx_candidates.size() - 1);
        new_state.start_room_idx = room_idx_candidates[dis2(gen)];
        return new_state;
    }
    // 2.5% の確率でラベルを変更
    else {
        std::vector<std::vector<int>> label_groups(4);
        for (int i = 0; i < state.rooms.size(); i++) {
            label_groups[state.rooms[i].label].push_back(i);
        }
        // 最大数のラベルと最小数のラベルを選ぶ
        size_t max_label_num = 0;
        size_t min_label_num = std::numeric_limits<size_t>::max();
        for (int i = 0; i < label_groups.size(); i++) {
            if (label_groups[i].size() > 0) {
                max_label_num = std::max(max_label_num, label_groups[i].size());
                min_label_num = std::min(min_label_num, label_groups[i].size());
            }
        }
        if (max_label_num == min_label_num) {
            return state;
        }
        std::vector<int> min_labels, max_labels;
        for (int i = 0; i < label_groups.size(); i++) {
            if (label_groups[i].size() == min_label_num) {
                min_labels.push_back(i);
            }
            else if (label_groups[i].size() == max_label_num) {
                max_labels.push_back(i);
            }
        }
        std::uniform_int_distribution<> dis2(0, min_labels.size() - 1);
        std::uniform_int_distribution<> dis3(0, max_labels.size() - 1);
        int min_label = min_labels[dis2(gen)];
        int max_label = max_labels[dis3(gen)];
        
        std::uniform_int_distribution<> dis4(0, label_groups[min_label].size() - 1);
        std::uniform_int_distribution<> dis5(0, label_groups[max_label].size() - 1);
        int min_label_room_idx = label_groups[min_label][dis4(gen)];
        int max_label_room_idx = label_groups[max_label][dis5(gen)];

        auto new_state = state;
        new_state.rooms[min_label_room_idx].label = max_label;
        new_state.rooms[max_label_room_idx].label = min_label;
        return new_state;
    }
    return state;
}

MapData solve2(int n, const std::string& doors, const std::vector<int>& labels) {
    State2 state = create_random_state2(n);
    Feedback feedback = calculate_score(state, doors, labels);
    long long iter_count = 0;
    auto f = [](long long iter_count) -> double {
        const double K = 100.0;
        return std::max(0.1, K * (1 - std::exp(-iter_count / 1000000.0)));
    };
    int max_right_count = feedback.right_count;
    int stagnation_threshold = 1000000;
    int stagnation_count = 0;
    while (true) {
        if (feedback.right_count > max_right_count) {
            std::cout << "max_right_count: " << "(" << max_right_count << "/" << labels.size() << ")" << " -> " << feedback.right_count << std::endl;
            max_right_count = feedback.right_count;
        }
        if (feedback.right_count == (int)(labels.size())) {
            break;
        }
        State2 next_state = mutate(state, labels, doors, feedback.mistakes, feedback.right_doors);
        Feedback next_feedback = calculate_score(next_state, doors, labels);
        // スコアが悪くなっても、一定確率で遷移を受け入れる
        std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<> dis(0, 1);
        double p = dis(gen);
        double delta = next_feedback.score - feedback.score;
        double accept_prob = std::exp(delta / f(iter_count));
        if (next_feedback.score > feedback.score || p < accept_prob) {
            state = next_state;
            feedback = next_feedback;
        }
        if (next_feedback.score <= feedback.score) {
            stagnation_count++;
        }
        else {
            stagnation_count = 0;
        }
        iter_count++;
        if (stagnation_count == stagnation_threshold) {
            std::cout << "stagnation detected" << std::endl;
            state = create_random_state2(n);
            feedback = calculate_score(state, doors, labels);
            stagnation_count = 0;
            iter_count = 0;
            max_right_count = feedback.right_count;
        }
    }
    std::vector<int> rooms;
    for (const auto& room : state.rooms) {
        rooms.push_back(room.label);
    }
    return MapData{
        .rooms=rooms,
        .starting_room=state.start_room_idx,
        .connections=create_connections(state.rooms)
    };
}

} // namespace kagamiz