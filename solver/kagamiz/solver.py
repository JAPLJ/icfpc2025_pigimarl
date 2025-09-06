import json
from hashlib import sha256

from pydantic import BaseModel
from schema import Connection, MapData, RoomDoor


class Room(BaseModel):
    label: int
    doors: list[int]


class State(BaseModel):
    rooms: list[Room]
    idx: int
    current_room_idx: int


def serialize_state(state: State) -> str:
    return sha256(json.dumps(state.model_dump()).encode()).hexdigest()


def create_connections(rooms: list[Room]) -> list[Connection]:
    n = len(rooms)
    done: dict[tuple[int, int], bool] = {}
    connections: list[Connection] = []

    for i in range(n):
        for j in range(6):
            if done.get((i, j), False):
                continue
            done[(i, j)] = True
            dst = rooms[i].doors[j]
            dst_door = next((k for k in range(6) if rooms[dst].doors[k] == i), -1)
            if dst_door == -1:
                raise Exception("Logic Error")
            done[(dst, dst_door)] = True
            connections.append(
                Connection(
                    src=RoomDoor(room=i, door=j), dst=RoomDoor(room=dst, door=dst_door)
                )
            )

    return connections


def dfs(
    n: int, doors: str, labels: list[int], state: State, memo: dict[str, MapData | None]
) -> MapData | None:
    serialized_state = serialize_state(state)
    if state.idx == len(doors):
        print(f"dfs completed(rooms={state.rooms})")
        label_cnt: dict[int, int] = {}  
        for room in state.rooms:
            if room.label == -1:
                memo[serialized_state] = None
                return None
            label_cnt[room.label] = label_cnt.get(room.label, 0) + 1
            for door in room.doors:
                if door == -1:
                    memo[serialized_state] = None
                    return None
        # label は均等に分配されているはず
        for _, cnt in label_cnt.items():
            if cnt not in [n // 4, (n + 3) // 4]:
                memo[serialized_state] = None
                return None
        memo[serialized_state] = MapData(
            rooms=[room.label for room in state.rooms],
            starting_room=0,
            connections=create_connections(state.rooms),
        )
        return memo[serialized_state]

    # current_room_idx の部屋から doors[idx] のドアを通ると label[idx+1] の部屋に移動した
    door_idx = int(doors[state.idx])
    if state.rooms[state.current_room_idx].doors[door_idx] != -1:
        next_room_idx = state.rooms[state.current_room_idx].doors[door_idx]
        if state.rooms[next_room_idx].label == labels[state.idx + 1]:
            next_state = State(
                rooms=state.rooms, idx=state.idx + 1, current_room_idx=next_room_idx
            )
            next_state_serialized = serialize_state(next_state)
            if next_state_serialized not in memo:
                memo[next_state_serialized] = dfs(n, doors, labels, next_state, memo)
            if memo[next_state_serialized] is not None:
                return memo[next_state_serialized]
        memo[serialized_state] = None
        return memo[serialized_state]

    # 次の部屋の候補を探す
    next_room_candidates_count = 0
    for next_room_idx in range(n):
        if state.rooms[next_room_idx].label != labels[state.idx + 1]:
            continue
        next_room_candidates_count += 1
        next_state = State(
            rooms=state.rooms, idx=state.idx + 1, current_room_idx=next_room_idx
        )
        next_state.rooms[state.current_room_idx].doors[door_idx] = next_room_idx
        next_state_serialized = serialize_state(next_state)
        if next_state_serialized not in memo:
            memo[next_state_serialized] = dfs(n, doors, labels, next_state, memo)
        if memo[next_state_serialized] is not None:
            return memo[next_state_serialized]
        next_state.rooms[state.current_room_idx].doors[door_idx] = -1

    if next_room_candidates_count >= (n + 3) // 4:
        memo[serialized_state] = None
        return memo[serialized_state]

    for next_room_idx in range(n):
        if state.rooms[next_room_idx].label == -1:
            next_state = State(
                rooms=state.rooms, idx=state.idx + 1, current_room_idx=next_room_idx
            )
            next_state.rooms[next_room_idx].label = labels[state.idx + 1]
            next_state.rooms[state.current_room_idx].doors[door_idx] = next_room_idx
            next_state_serialized = serialize_state(next_state)
            if next_state_serialized not in memo:
                memo[next_state_serialized] = dfs(n, doors, labels, next_state, memo)
            if memo[next_state_serialized] is not None:
                return memo[next_state_serialized]
            next_state.rooms[next_room_idx].label = -1
            next_state.rooms[state.current_room_idx].doors[door_idx] = -1

    memo[serialized_state] = None
    return memo[serialized_state]


def solve(n: int, doors: str, labels: list[int]) -> MapData:
    rooms = [Room(label=-1, doors=[-1] * 6) for _ in range(n)]
    rooms[0].label = labels[0]
    result = dfs(n, doors, labels, State(rooms=rooms, idx=0, current_room_idx=0), {})

    if result is None:
        raise Exception("No solution found")
    return result
