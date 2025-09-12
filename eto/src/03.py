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

N = 6
# N = 30
PLAN_LENGTH = 18 * N

seed = 1

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


def create_plans(plans_len=3, magic_walk_len=5, random_walk_len=10, use_same_magic_walk=False):
    res = []
    random.seed(seed)

    if use_same_magic_walk:
        magic_walks.append("".join(random.choices("012345", k=magic_walk_len)))
        print(f"magic_walk: {magic_walks}")

    for _ in range(plans_len):
        if use_same_magic_walk:
            magic_walk = magic_walks[0]
        else:
            magic_walk = "".join(random.choices("012345", k=magic_walk_len))
            print(f"magic_walk: {magic_walk}")
        magic_walks.append(magic_walk)
        walk = magic_walk
        while len(walk) < PLAN_LENGTH:
            ll = random.randrange(1, random_walk_len)
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
    signatures = {}

    for li,labels in enumerate(explore_res["results"]):
        rooms = [0 for i in range(len(labels))]

        # 適当に部屋名を作成
        for i in range(len(labels)):
            rooms[i] = f"{labels[i]}_{hex(suffixes[labels[i]])[2:]}"
            suffixes[labels[i]] += 1
        rooms_list.append(rooms)

    # uf を初期化
    uf = UnionFindDict([room for rooms in rooms_list for room in rooms])
    for li in range(1, len(rooms_list)):
        uf.union(rooms_list[0][0], rooms_list[li][0])

    for li, labels in enumerate(explore_res["results"]):
        rooms = rooms_list[li]
        magic_walk = magic_walks[li]
        pattern = re.compile(magic_walk)
        # magic_walk をしたときに同じラベルが出る => その部分は同じ部屋
        changed = True
        memo = {}
        while changed:
            changed = False
            # print(f"pattern: {pattern}")
            itr = pattern.finditer(plans[li])
            for match in itr:
                start = match.start()
                end = match.end()
                label_seq = "".join(map(str, labels[start:end + 1]))
                signatures[label_seq] = uf.find(rooms[start])
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
                        assert room_to_by_map[0] == room_to[0], f"different label. room_from: {room_from}, door: {door}, room_to_by_map: {room_to_by_map}, room_to: {room_to}, pattern: {magic_walk}"
                        if not uf.same(room_to, room_to_by_map):
                            changed = True
                        uf.union(room_to, room_to_by_map)

        map_, rooms_list = squash_rooms_list(rooms_list, map_, uf)

    print(f"signatures: {len(signatures)}: {signatures}")
    # for i in range(len(plans[0])):
    #     print(f"{labels[i]}:{rooms[i]} -- {plans[li][i]} -> {labels[i + 1]}:{rooms[i + 1]}")
    # pprint(map_)
    print(f"len(map_): {len(map_)}")

    map_, rooms_list = attempt_merge_rooms(explore_res["results"], rooms_list, map_, uf)
    # pprint(map_)
    print(f"len(map_): {len(map_)}")
    print(f"rooms: {rooms_list[0][:20]}")

    starting_room = uf.find(rooms_list[0][0])
    print(f"starting_room: {starting_room}")

    # magic_walk 中に辿った経路の signature を取得
    magic_walk = magic_walks[0]
    next_plans = [magic_walk * 2]
    for i in range(1, len(magic_walk)):
        next_plan = magic_walk[:-i] + magic_walk
        next_plans.append(next_plan)
    next_plans = list(reversed(next_plans))
    print(f"next_plans: {next_plans}")
    next_explore_res = send_explore(next_plans)
    next_signatures = []
    for li, labels in enumerate(next_explore_res["results"]):
        next_signatures.append("".join(map(str, labels[-1 - len(magic_walk):])))
    print(f"next_signatures: {next_signatures}")
    # 以前辿ったルート上で、 signature に対応する部屋を探す
    for i, next_signature in enumerate(next_signatures):
        room_by_signature = signatures.get(next_signature, None)
        if room_by_signature is None:
            continue
        room = rooms_list[0][i + 1]
        assert room_by_signature[0] == room[0], f"different label. room: {room}, room_by_signature: {room_by_signature}"
        uf.union(room, room_by_signature)
        print(f"union: {room} -- {room_by_signature}")
    map_, rooms_list = squash_rooms_list(rooms_list, map_, uf)
    print(f"len(map_): {len(map_)}")
    # print(f"map_: {map_}")
    # print(f"rooms: {rooms_list[0][:20]}")

    # map_ を作成
    for li in range(len(plans)):
        rooms = rooms_list[li]
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
                assert room_to_by_map[0] == room_to[0], f"different label. room_from: {room_from}, door: {door}, room_to_by_map: {room_to_by_map}, room_to: {room_to}, pattern: {magic_walk}"
                # if not uf.same(room_to, room_to_by_map):
                #     changed = True
                uf.union(room_to, room_to_by_map)
    map_, rooms_list = squash_rooms_list(rooms_list, map_, uf)
    print(f"len(map_): {len(map_)}")
    # print(f"map_: {map_}")
    # print(f"rooms: {rooms_list[0][:20]}")

    map_, rooms_list = attempt_merge_rooms(explore_res["results"], rooms_list, map_, uf)
    pprint(map_)
    print(f"len(map_): {len(map_)}")


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
    if len(intersection) <= 2:
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
    label_groups = {}
    for li in range(len(labels_list)):
        labels = labels_list[li]
        rooms = rooms_list[li]
        for i, (label, room) in enumerate(zip(labels, rooms)):
            if label not in label_groups:
                label_groups[label] = set()
            label_groups[label].add(room)

    for label, room_set in label_groups.items():
        for r1, r2 in combinations(room_set, 2):
            if uf.same(r1, r2):
                continue
            # 似たドアを使っているならマージ可能
            if has_similar_door(r1, r2, map_):
                uf.union(r1, r2)
                continue
    new_map, new_rooms_list = squash_rooms_list(rooms_list, map_, uf)
    return new_map, new_rooms_list


def send_guess(guess):
    res = requests.post(
        f"{BASE_URL}/guess",
        json={"id": ID, "map": guess}
    )
    return res.json()

def main():
    # send_select(f"{N}")

    plans = create_plans(2, magic_walk_len=4, random_walk_len=5, use_same_magic_walk=True)
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

