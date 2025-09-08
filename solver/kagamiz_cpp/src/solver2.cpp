#include "solver2.hpp"
#include "helper.hpp"
#include <iostream>
#include <sstream>
#include <random>

namespace kagamiz {


State2 create_random_state2(int n, const std::string& doors, const std::vector<int>& labels) {
    std::vector<Room> rooms;
    for (int i = 0; i < n; i++) {
        rooms.emplace_back(-1, std::vector<int>(6, -1));
    }
    std::vector<int> start_room_idx_candidates;
    std::vector<std::vector<int>> label_to_rooms(4);
    for (int i = 0; i < n; i++) {
        rooms[i].label = i % 4;
        if (rooms[i].label == labels[0]) {
            start_room_idx_candidates.push_back(i);
        }
        label_to_rooms[rooms[i].label].push_back(i);
    }

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0, start_room_idx_candidates.size() - 1);
    int start_room_idx = start_room_idx_candidates[dis(gen)];

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

    // それっぽくなるようにつなぎ直していく
    int current_room_idx = start_room_idx;
    for (int i = 0; i < doors.length(); i++) {
        int door_idx = doors[i] - '0';
        std::uniform_int_distribution<> dis(0, label_to_rooms[labels[i + 1]].size() - 1);
        int next_room_idx = label_to_rooms[labels[i + 1]][dis(gen)];
        std::uniform_int_distribution<> dis2(0, 5);
        int next_door_idx = dis2(gen);

        auto [room_idx3, door_idx3] = pair_doors[current_room_idx][door_idx];
        auto [room_idx4, door_idx4] = pair_doors[next_room_idx][next_door_idx];
        rooms[current_room_idx].doors[door_idx] = next_room_idx;
        rooms[next_room_idx].doors[next_door_idx] = current_room_idx;
        rooms[room_idx3].doors[door_idx3] = room_idx4;
        rooms[room_idx4].doors[door_idx4] = room_idx3;
        pair_doors[current_room_idx][door_idx] = std::make_pair(next_room_idx, next_door_idx);
        pair_doors[next_room_idx][next_door_idx] = std::make_pair(current_room_idx, door_idx);
        pair_doors[room_idx3][door_idx3] = std::make_pair(room_idx4, door_idx4);
        pair_doors[room_idx4][door_idx4] = std::make_pair(room_idx3, door_idx3);
        current_room_idx = next_room_idx;
    }
    return State2(rooms, start_room_idx, pair_doors);
}

Feedback calculate_score(const State2& state, const std::string& doors, const std::vector<int>& labels) {
    double score = 0;
    int right_count = 0;
    int consecutive_right_count = 0;
    int consecutive_wrong_count = 0;
    int current_room_idx = state.start_room_idx;
    std::vector<int> mistakes;
    std::set<std::pair<int, int>> right_doors;
    std::set<std::pair<int, int>> wrong_doors;
    if (state.rooms[current_room_idx].label == labels[0]) {
        score = 1;
        right_count++;
        consecutive_right_count++;
    }
    else {
        score = -1000000000;
        consecutive_wrong_count++;
    }
    for (int i = 0; i < doors.length(); i++) {
        int door_idx = doors[i] - '0';
        if (state.rooms[current_room_idx].doors[door_idx] == labels[i + 1]) {
            score += 2 * consecutive_right_count + 1;
            right_doors.insert(std::make_pair(current_room_idx, door_idx));
            right_doors.insert(state.pair_doors[current_room_idx][door_idx]);
            right_count++;
            consecutive_right_count++;
            consecutive_wrong_count = 0;
        }
        else {
            consecutive_right_count = 0;
            consecutive_wrong_count++;
            score -= (2 * consecutive_wrong_count + 1);
            mistakes.push_back(i);
            wrong_doors.insert(std::make_pair(current_room_idx, door_idx));
            wrong_doors.insert(state.pair_doors[current_room_idx][door_idx]);
        }
        current_room_idx = state.rooms[current_room_idx].doors[door_idx];
    }

    for (const auto& [room_idx, door_idx] : wrong_doors) {
        right_doors.erase(std::make_pair(room_idx, door_idx));
    }

    //score += 10 * right_doors.size() * right_doors.size();
    score = right_count;
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
    // 95% の確率で間違えたドアを修正
    if (true) {
        auto new_state = state;
//        for (auto& mistake_door_idx : mistakes) {
//            std::uniform_int_distribution<> dis_mistake(0, mistakes.size() - 1);
//            int mistake_door_idx = mistakes[dis_mistake(gen)];
            std::uniform_int_distribution<> dis_all(0, doors.length() - 1);
            int mistake_door_idx = dis_all(gen);
            std::vector<int> room_transition_history;
            int current_room_idx = state.start_room_idx;
            room_transition_history.push_back(current_room_idx);
            for (int i = 0; i < doors.length(); i++) {
                int door_idx = doors[i] - '0';
                current_room_idx = state.rooms[current_room_idx].doors[door_idx];
                room_transition_history.push_back(current_room_idx);
            }

            std::vector<std::pair<int, int>> new_room_and_door_candidates;

            for (int i = 0; i < state.rooms.size(); i++) {
                if (state.rooms[i].label == labels[mistake_door_idx + 1]) {
                    for (int j = 0; j < 6; j++) {
//                        if (!right_doors.contains(std::make_pair(i, j))) {
                            new_room_and_door_candidates.push_back(std::make_pair(i, j));
//                        }
                    }
                }
            }
            if (new_room_and_door_candidates.size() == 0) {
                // continue;
                return state;
            }
            std::uniform_int_distribution<> dis_new_room_and_door(0, new_room_and_door_candidates.size() - 1);

            int room_idx = room_transition_history[mistake_door_idx + 1];
            int door_idx = doors[mistake_door_idx] - '0';
            const auto& [room_idx2, door_idx2] = new_room_and_door_candidates[dis_new_room_and_door(gen)];
            const auto& [room_idx3, door_idx3] = state.pair_doors[room_idx][door_idx];
            const auto& [room_idx4, door_idx4] = state.pair_doors[room_idx2][door_idx2];
            // 1, 2 と 3, 4 を pair にするように書き換える。
            new_state.pair_doors[room_idx][door_idx] = std::make_pair(room_idx2, door_idx2);
            new_state.pair_doors[room_idx2][door_idx2] = std::make_pair(room_idx, door_idx);
            new_state.pair_doors[room_idx3][door_idx3] = std::make_pair(room_idx4, door_idx4);
            new_state.pair_doors[room_idx4][door_idx4] = std::make_pair(room_idx3, door_idx3);
            new_state.rooms[room_idx].doors[door_idx] = room_idx2;
            new_state.rooms[room_idx2].doors[door_idx2] = room_idx;
            new_state.rooms[room_idx3].doors[door_idx3] = room_idx4;
            new_state.rooms[room_idx4].doors[door_idx4] = room_idx3;
//        }
        return new_state;
    }
    // 5% の確率でラベルを変更
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

std::string format_right_doors(const std::set<std::pair<int, int>>& right_doors) {
    std::stringstream ss;
    ss << "[";
    for (const auto& [room_idx, door_idx] : right_doors) {
        ss << "{" << room_idx << ", " << door_idx << "}";
        if (room_idx != right_doors.rbegin()->first || door_idx != right_doors.rbegin()->second) {
            ss << ",";
        }
    }
    ss << "]";
    return ss.str();
}

std::string format_mistakes(const std::vector<int>& mistakes) {
    std::stringstream ss;
    ss << "[";
    for (const auto& mistake : mistakes) {
        ss << mistake;
        if (mistake != mistakes.back()) {
            ss << ",";
        }
    }
    ss << "]";
    return ss.str();
}

MapData solve2(int n, const std::string& doors, const std::vector<int>& labels) {
    State2 state = create_random_state2(n, doors, labels);
    Feedback feedback = calculate_score(state, doors, labels);
    long long iter_count = 0;
    auto f = [](long long iter_count, long long stagnation_count) -> double {
        const double K = 1.0;
        double t = iter_count;
        return std::max(0.1, K * std::exp(-t / 1000000.0));
    };
    double max_score = feedback.score;
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
        double temperature = f(iter_count, stagnation_count);
        double accept_prob = std::exp(delta / temperature);

        if (next_feedback.score > max_score) {
            stagnation_count = 0;
        }
        else {
            stagnation_count++;
        }

        if (next_feedback.score > feedback.score || p < accept_prob) {
            if (next_feedback.score > max_score) {
                std::cout << "score: " << max_score << " -> " << next_feedback.score << " (right_count: " << next_feedback.right_count << ", temperature: " << temperature << ", iter_count: " << iter_count << ")" << std::endl;
                max_score = next_feedback.score;
            }
            state = next_state;
            feedback = next_feedback;
        }
        iter_count++;
        if (stagnation_count == stagnation_threshold) {
            std::cout << "stagnation detected" << std::endl;
            state = create_random_state2(n, doors, labels);
            feedback = calculate_score(state, doors, labels);
            max_score = feedback.score;
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
