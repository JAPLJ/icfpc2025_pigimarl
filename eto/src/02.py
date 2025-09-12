import json
import copy
import requests
import random
from collections import defaultdict, Counter, deque
from pprint import pprint
from itertools import combinations
import os
import re
from union_find import UnionFindDict
from math import floor, ceil

BASE_URL = os.getenv("BASE_URL")
ID = os.getenv("ID")

# N = 6
N = 30
MIN_PATTERN_LEN = 4
PLAN_LENGTH = 18 * N

seed = 2

magic_walks = []

def emulate_plans(plans):
    fname = "../tester/graph.txt"
    with open(fname, "r") as f:
        graph_str = f.readlines()
    graph_str = graph_str[-1]
    graph_str = graph_str.replace("graph: Graph {", "{")
    graph = {}
    graph["edges"] = json.loads(re.search("edges: (\[.*\])", graph_str).group(1))
    graph["start_node"] = int(re.search("start_node: (\d+)", graph_str).group(1))
    n = len(graph["edges"])
    total_edges = n * len(graph["edges"][0])
    visited_edges = [[False for _ in range(6)] for _ in range(n)]
    room = graph["start_node"]
    for plan in plans:
        for i in range(len(plan)):
            door = int(plan[i])
            visited_edges[room][door] = True
            room = graph["edges"][room][door]
    print("visited_edges")
    print(f"{sum(sum(visited_edges, []))}/{total_edges}")

def send_select(problem_name):
    res = requests.post(
        f"{BASE_URL}/select",
        json={"id": ID, "problemName": problem_name}
    )
    return res.json()


def create_plans():
    res = []
    random.seed(seed)

    for _ in range(3):
        magic_walk = "".join(random.choices("012345", k=5))
        magic_walks.append(magic_walk)
        print(f"magic_walk: {magic_walk}")
        walk = ""
        while len(walk) < PLAN_LENGTH:
            ll = random.randrange(1, 10)
            walk += "".join(random.choices("012345", k=ll)) + magic_walk
        walk = walk[:PLAN_LENGTH]
        res.append(walk)

    return res

def send_explore(plans):
    res = requests.post(
        f"{BASE_URL}/explore",
        json={"id": ID, "plans": plans}
    )
    return res.json()


def complete_map(map_):
    map_with_door = defaultdict(dict)
    # complete the map by adding the reverse direction of the connections
    for (room, next_rooms) in map_.items():
        for (door, room_to) in next_rooms.items():
            # 同じ部屋に戻る場合(if room_from == room_to)は同じドアに戻ったと考えることにする
            if room == room_to:
                map_with_door[room_to][door] = (room, door)
                continue
            for (door_to, room_from) in map_[room_to].items():
                if door in map_with_door[room_from]:
                    continue
                # if the reverse direction is not in the map, add it
                if room_from == room and door_to not in map_with_door[room_to]:
                    map_[room_to][door_to] = room_from
                    map_with_door[room_from][door] = (room_to, door_to)
                    map_with_door[room_to][door_to] = (room_from, door)
                    break

    # roomA から roomB に入る線でありかつ出口が確定していない線が n 本ある && roomB に入ったことのないドアが n 個ある => roomB から roomA に出る線を追加
    for (roomA, roomB) in combinations(map_.keys(), 2):
        a2b = 0
        b2a = 0
        unknown_from_b = []
        for door in "012345":
            if door in map_[roomA] and map_[roomA][door] == roomB:
                a2b += 1
            if door in map_[roomB] and map_[roomB][door] == roomA:
                b2a += 1
            if door not in map_[roomB]:
                unknown_from_b.append(door)
        if len(unknown_from_b) > 0 and len(unknown_from_b) == a2b - b2a:
            for door in unknown_from_b:
                # print(f"add door {roomB} -- {door} -> {roomA}. a2b: {a2b}, b2a: {b2a}, unknown_from_b: {unknown_from_b}")
                map_[roomB][door] = roomA
            pprint(map_)
            return complete_map(map_)

    # 道の数が足りない部屋が1つの場合は全部その部屋とのループとして考えることにする
    num_room_with_unknown_door = sum(len(next_rooms) < len("012345") for next_rooms in map_.values())
    assert num_room_with_unknown_door <= 1, f"num_room_with_unknown_door: {num_room_with_unknown_door}"
    if num_room_with_unknown_door == 1:
        for (room, next_rooms) in map_.items():
            if len(next_rooms) < len("012345"):
                for door in "012345":
                    if door not in next_rooms:
                        map_[room][door] = room
                        map_with_door[room][door] = (room, door)
    return map_, map_with_door

def construct_connections(map_with_door, room_list):
    connections = []
    for (room, next_rooms) in map_with_door.items():
        room_index = room_list.index(room)
        for (door, (room_to, door_to)) in next_rooms.items():
            connections.append({
                "from": {"room": room_index, "door": door},
                "to": {"room": room_list.index(room_to), "door": door_to}
            })
    return connections

def squash_rooms_list(rooms_list, map_, uf):
    new_map = defaultdict(dict)
    for (room, next_rooms) in map_.items():
        for (door, room_to) in next_rooms.items():
            new_map[uf.find(room)][door] = uf.find(room_to)
    map_ = new_map
    for i in range(len(rooms_list)):
        for j in range(len(rooms_list[i])):
            rooms_list[i][j] = uf.find(rooms_list[i][j])
    return map_, rooms_list


def create_guess(plans, explore_res):
    # Create a candidate map based on explore results
    map_ = defaultdict(dict)
    suffixes = {0: 0, 1: 0, 2: 0, 3: 0}
    rooms_list = []

    for li,labels in enumerate(explore_res["results"]):
        rooms = [0 for i in range(len(labels))]

        # 適当に部屋名を作成
        for i in range(len(labels)):
            rooms[i] = f"{labels[i]}_{hex(suffixes[labels[i]])[2:]}"
            suffixes[labels[i]] += 1
        rooms_list.append(rooms)
    uf = UnionFindDict([room for rooms in rooms_list for room in rooms])
    for li in range(1, len(rooms_list)):
        uf.union(rooms_list[0][0], rooms_list[li][0])

    for li, labels in enumerate(explore_res["results"]):
        rooms = rooms_list[li]
        magic_walk = magic_walks[li]
        for pattern_len in range(len(magic_walk), MIN_PATTERN_LEN - 1, -1):
            # magic_walk をしたときに同じラベルが出る => その部分は同じ部屋
            changed = True
            while changed:
                changed = False
                for pi in range(0, len(magic_walk) - pattern_len + 1):
                    memo = {}
                    pattern = re.compile(magic_walk[pi:pi + pattern_len])
                    # print(f"pattern: {pattern}")
                    itr = pattern.finditer(plans[li])
                    for match in itr:
                        start = match.start()
                        end = match.end()
                        label_seq = "".join(map(str, labels[start:end + 1]))
                        same_seq_start = memo.get(label_seq, -1)
                        # 初めてその label_seq が出てくるならその場所をメモる
                        if same_seq_start == -1:
                            memo[label_seq] = start
                        # 以前見た label_seq ならその部分は同じ部屋
                        else:
                            for i in range(start, end + 1):
                                if not uf.same(rooms[i], rooms[same_seq_start + i - start]):
                                    changed = True
                                    uf.union(rooms[i], rooms[same_seq_start + i - start])

                    # map_ を作成
                    for i in range(len(plans[li])):
                        door = plans[li][i]
                        room_from = rooms[i]
                        room_to = rooms[i + 1]
                        room_to_by_map = map_[room_from].get(door, "_")
                        # map_ に記載がない
                        if room_to_by_map == "_":
                            map_[room_from][door] = room_to
                        # すでに記述がある => その2つの部屋は同じ部屋
                        elif room_to_by_map != room_to:
                            assert room_to_by_map[0] == room_to[0], f"different label. room_from: {room_from}, door: {door}, room_to_by_map: {room_to_by_map}, room_to: {room_to}, pattern: {magic_walk[pi:pi + pattern_len]}"
                            if not uf.same(room_to, room_to_by_map):
                                changed = True
                            uf.union(room_to, room_to_by_map)

            map_, rooms_list = squash_rooms_list(rooms_list, map_, uf)

    # for i in range(len(plans[0])):
    #     print(f"{labels[i]}:{rooms[i]} -- {plans[li][i]} -> {labels[i + 1]}:{rooms[i + 1]}")
    # pprint(map_)
    print(f"len(map_): {len(map_)}")

    map_ = attempt_merge_rooms(explore_res["results"], rooms_list, map_, uf)
    # pprint(map_)
    # print(len(map_))

    # # room0 --door0-> room1 --door1-> room2 --door2-> room3 --door3-> room4
    # rdr_counter = Counter()
    # rd_counter = Counter()
    # for i in range(len(plans[0])):
    #     door = plans[0][i]
    #     room_from = rooms[i]
    #     room_to = rooms[i + 1]
    #     rdr_counter[(room_from, door, room_to)] += 1
    #     rd_counter[(room_from, door)] += 1
    # pprint(rdr_counter)
    # print(len(rdr_counter))
    # for (room_from, door, room_to), count in rdr_counter.items():
    #     print(f"{room_from} -- {door} -> {room_to}: {count:>2}; {count / rd_counter[(room_from, door)]}")
    # # mermaid graph
    # print("```mermaid")
    # print("graph TD")
    # for (room_from, door, room_to), count in rdr_counter.items():
    #     print(f"{room_from} --{door},{count / rd_counter[(room_from, door)]}--> {room_to};")
    # print("```")

    # pprint(map_)
    # map_, map_with_door = complete_map(map_)
    # pprint(map_)
    # pprint(map_with_door)

    # # decide starting room
    # starting_room = None
    # for (room, next_rooms) in map_with_door.items():
    #     if next_rooms[plans[0][0]][0] == rooms[0]:
    #         starting_room = room
    #         break


    # room_list = list(map_.keys())
    # map_data = {
    #     "rooms": room_list,
    #     "startingRoom": room_list.index(starting_room),
    #     "connections": construct_connections(map_with_door, room_list)     # List of connections between rooms
    # }
    # return map_data


def merge_possible(r1, r2, map_, mergeable={}, uf=None):
    if uf is not None and uf.same(r1, r2):
        return True
    # 違うラベルならマージ不可能
    if r1[0] != r2[0]:
        return False
    # すでに試したことがあればそれを返す
    if (r1, r2) in mergeable:
        return mergeable[(r1, r2)]
    mergeable[(r1, r2)] = True

    # 同じドアを使ってないならマージ可能
    r1_doors = set(map_[r1].keys())
    r2_doors = set(map_[r2].keys())
    intersection = r1_doors & r2_doors
    if len(intersection) == 0:
        return True

    # 同じドアを使っていて、それらがマージ不可能な部屋に行くならマージ不可能
    for door in intersection:
        if not merge_possible(map_[r1][door], map_[r2][door], map_, mergeable, uf):
            mergeable[(r1, r2)] = False
            return False
    return True

def has_similar_door(r1, r2, map_):
    r1_doors = set(map_[r1].keys())
    r2_doors = set(map_[r2].keys())
    intersection = r1_doors & r2_doors
    if len(intersection) <= 1:
        return False
    for door in intersection:
        if map_[r1][door][0] != map_[r2][door][0]:
            return False
    return True


def is_good_map(map_, rooms_list, uf):
    new_map, new_rooms_list = squash_rooms_list(rooms_list, map_, uf)
    if len(new_map) != N:
        return False

    return is_good_groups([0, 1, 2, 3], new_rooms_list, new_map, uf)


def is_good_groups(used_labels, rooms_list, map_, uf):
    new_map, new_rooms_list = squash_rooms_list(rooms_list, map_, uf)
    labels = [x[0] for x in new_map.keys()]
    lb = floor(N / 4)
    ub = ceil(N / 4)
    c = Counter(labels)
    num_used_rooms = 0
    # 使用済みのラベルの部屋の数が適切かどうか
    for label in used_labels:
        if not (lb <= c[label] <= ub):
            return False
        num_used_rooms += c[label]
    # 残りのラベルの部屋の数が適切かどうか
    left_labels = set(labels) - used_labels
    if len(left_labels) * ub + num_used_rooms < N:
        return False
    if len(left_labels) * lb + num_used_rooms > N:
        return False
    return True


def attempt_merge_rooms(labels_list, rooms_list, map_, uf):
    def dfs_merge(rooms_list, map_, uf, mergeable_pairs, mergeable={}, mpi=0):
        stack = deque()
        initial_state = (rooms_list, map_, uf, mergeable.copy(), mpi, set())
        stack.append(initial_state)
        last_label = mergeable_pairs[0][0][0]

        while stack:
            # print(f"stack: {len(stack)}")
            current = stack[0]
            rooms_list, map_, uf, mergeable, mpi, used_labels = current
            # print(f"stack: r1r2 {mergeable_pairs[mpi]}, uf.parent: {uf.parent}")
            # input()
            stack.popleft()  # 現在の状態を削除

            # Base case: processed all mergeable pairs
            if mpi >= len(mergeable_pairs):
                map_, rooms_list = squash_rooms_list(rooms_list, map_, uf)
                print(f"is_good_map?")
                pprint(map_)
                input()
                if is_good_map(map_, rooms_list, uf):
                    return map_, rooms_list
                continue

            r1, r2 = mergeable_pairs[mpi]
            # print(f"r1: {r1}, r2: {r2}")

            if uf.same(r1, r2):
                new_state = (rooms_list, map_, uf, mergeable, mpi + 1, used_labels)
                stack.appendleft(new_state)
                continue

            label = r1[0]
            if not label in used_labels:
                if not is_good_groups(used_labels, rooms_list, map_, uf):
                    # print(f"not good groups: {used_labels}. stack: {len(stack)}")
                    continue
                used_labels.add(label)
                print(f"used_labels: {used_labels}")

            #  don't merge this pair
            new_mergeable = mergeable.copy()
            new_mergeable[(r1, r2)] = False
            new_state = (rooms_list, map_, uf, new_mergeable, mpi + 1, used_labels)
            stack.appendleft(new_state)

            # merge this pair
            if merge_possible(r1, r2, map_, mergeable, uf):
                new_mergeable[(r1, r2)] = True
                new_uf = uf.copy()
                new_uf.union(r1, r2)
                new_state = (rooms_list, map_, new_uf, new_mergeable, mpi + 1, used_labels)
                stack.appendleft(new_state)
                # print(f"merge {r1} and {r2}")

        raise ValueError("not mergeable")

    mergeable_pairs = []
    mergeable = {}

    label_groups = {}
    for li in range(len(labels_list)):
        labels = labels_list[li]
        rooms = rooms_list[li]
        for i, (label, room) in enumerate(zip(labels, rooms)):
            if label not in label_groups:
                label_groups[label] = set()
            label_groups[label].add(room)

    # print(f"label_groups: {label_groups}")
    for label, room_set in label_groups.items():
        # print(f"label: {label}, room_set: {room_set}")
        for r1, r2 in combinations(room_set, 2):
            if uf.same(r1, r2):
                continue
            # 似たドアを使っているならマージ可能
            if has_similar_door(r1, r2, map_):
                uf.union(r1, r2)
                mergeable[(r1, r2)] = True
                continue
            # マージしても良いならマージ候補に追加
            if merge_possible(r1, r2, map_, mergeable, uf):
                mergeable_pairs.append((r1, r2))
        break
    # pprint(mergeable_pairs)

    print(f"mergeable_pairs: {len(mergeable_pairs)}")
    mergeable_pairs = list(sorted(mergeable_pairs, key=lambda x: x[0][0]))
    # pprint(mergeable_pairs[:20])
    new_map, new_rooms_list = dfs_merge(rooms_list, map_, uf, mergeable_pairs, mergeable)
    if new_map is False:
        print("not mergeable")
        return False, False
    print(f"new_map: {len(new_map)}")
    print(f"new_rooms_list: {len(new_rooms_list)}")
    return new_map, new_rooms_list


def send_guess(guess):
    res = requests.post(
        f"{BASE_URL}/guess",
        json={"id": ID, "map": guess}
    )
    return res.json()

def main():
    # send_select(f"{N}")

    plans = create_plans()
    print(f"'{json.dumps(plans)}'")

    explore_res = send_explore(plans)
    # explore_res = json.loads('{"results": [[0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 2, 2, 2, 2, 2, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 2, 2, 2, 1, 0, 0, 2, 0, 2, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0]], "queryCount": 2}')
    print(f"'{json.dumps(explore_res)}'")
    emulate_plans(plans)

    guess = create_guess(plans, explore_res)
    # print(f"'{json.dumps(guess)}'")
    # guessed = send_guess(guess)
    # print(f"'{json.dumps(guessed)}'")

if __name__ == "__main__":
    main()
