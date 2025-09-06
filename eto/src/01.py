import json
import requests
import random
from collections import defaultdict, Counter
from pprint import pprint
from itertools import combinations
import os
import re
from union_find import UnionFindDict

BASE_URL = os.getenv("BASE_URL")
ID = os.getenv("ID")

N = 6
# N = 30

seed = 1

magic_walk = "012345"

def send_select(problem_name):
    res = requests.post(
        f"{BASE_URL}/select",
        json={"id": ID, "problemName": problem_name}
    )
    return res.json()


def create_plans():
    res = []
    random.seed(seed)
    # random_walk = "".join(random.choices("012345", k=18 * N))
    walk = ""
    while len(walk) < 18 * N:
        ll = random.randrange(5)
        walk += "".join(random.choices("012345", k=ll)) + magic_walk
    walk = walk[:18 * N]
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
                print(f"add door {roomB} -- {door} -> {roomA}. a2b: {a2b}, b2a: {b2a}, unknown_from_b: {unknown_from_b}")
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


def create_guess(plans, explore_res):
    # Create a candidate map based on explore results
    labels = explore_res["results"][0]
    rooms = [-1 for i in range(len(labels))]
    map_ = defaultdict(dict)

    suffixes = {0: "a", 1: "a", 2: "a", 3: "a"}
    uf = UnionFindDict([])

    for pattern_len in range(len(magic_walk), 3, -1):
        # magic_walk をしたときに同じラベルが出る => その部分は同じ部屋
        memo = {}
        pattern = re.compile(magic_walk[:pattern_len])
        itr = pattern.finditer(plans[0])
        for match in itr:
            start = match.start()
            end = match.end()
            label_seq = "".join(map(str, labels[start:end + 1]))
            same_seq_start = memo.get(label_seq, -1)
            # そのラベルが出てくる初めの部分
            if same_seq_start == -1:
                for i in range(start, end + 1):
                    if rooms[i] == -1:
                        rooms[i] = str(labels[i]) + suffixes[labels[i]]
                        uf.add(rooms[i])
                        suffixes[labels[i]] = chr(ord(suffixes[labels[i]]) + 1)
                memo[label_seq] = start
            # 同じラベルが出る部分がある
            else:
                for i in range(start, end + 1):
                    rooms[i] = rooms[same_seq_start + i - start]
                    uf.add(rooms[i])

        # map_ を作成
        for i in range(len(plans[0])):
            door = plans[0][i]
            room_from = rooms[i]
            room_to = rooms[i + 1]
            if room_from != -1 and room_to != -1:
                room_to_by_map = map_[room_from].get(door, "_")
                # map_ に記載がない
                if room_to_by_map == "_":
                    map_[room_from][door] = room_to
                # すでに記述がある => その2つの部屋は同じ部屋
                elif room_to_by_map != room_to:
                    assert room_to_by_map[0] == room_to[0], f"different label. room_to_by_map: {room_to_by_map}, room_to: {room_to}"
                    uf.union(room_to, room_to_by_map)
        pprint(map_)

        # uf を使って map_, rooms を更新
        new_map = defaultdict(dict)
        for (room, next_rooms) in map_.items():
            for (door, room_to) in next_rooms.items():
                new_map[uf.find(room)][door] = uf.find(room_to)
        map_ = new_map
        for i, room in enumerate(rooms):
            rooms[i] = uf.find(room)

        for room, label in zip(rooms, labels):
            print(f"{room}|{label}|")
        break

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

    guess = create_guess(plans, explore_res)
    # print(f"'{json.dumps(guess)}'")
    # guessed = send_guess(guess)
    # print(f"'{json.dumps(guessed)}'")

if __name__ == "__main__":
    main()
