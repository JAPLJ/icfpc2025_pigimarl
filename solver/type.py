from enum import StrEnum
from pydantic import BaseModel, Field


class ProblemName(StrEnum):
    PROBATIO = "probatio"
    PRIMUS = "primus"
    SECUNDUS = "secundus"
    TERTIUS = "tertius"
    QUARTUS = "quartus"
    QUINTUS = "quintus"


class Problem:
    name: ProblemName
    size: int

    def __init__(self, name: ProblemName, size: int):
        self.name = name
        self.size = size


PROBLEMS = [
    Problem(ProblemName.PROBATIO, 3),
    Problem(ProblemName.PRIMUS, 6),
    Problem(ProblemName.SECUNDUS, 12),
    Problem(ProblemName.TERTIUS, 18),
    Problem(ProblemName.QUARTUS, 24),
    Problem(ProblemName.QUINTUS, 30),
]


class Position(BaseModel):
    room: int
    door: int


class Connection(BaseModel):
    pos_from: Position = Field(alias="from")
    pos_to: Position = Field(alias="to")


class Graph(BaseModel):
    rooms: list[int]
    starting_room: int = Field(alias="startingRoom")
    connections: list[Connection]


def check_plans(graph_size: int, plans: list[str]) -> None:
    for plan in plans:
        assert len(plan) <= 18 * graph_size, "パスの長さが 18n を超えています"
        for door in plan:
            assert 0 <= int(door) and int(door) <= 5, "door が [0, 5] の範囲外です"


def check_graph(graph_size: int, graph: Graph) -> None:
    graph_rooms = set(graph.rooms)
    used_door = set()
    assert len(graph.rooms) == graph_size, "rooms の長さが不正です"
    assert graph.starting_room in graph_rooms, "starting_room が rooms にありません"
    for connection in graph.connections:
        assert connection.pos_from.room in graph_rooms, "connection.from.room が rooms にありません"
        assert connection.pos_from.door in range(6), "door が [0, 5] の範囲外です"
        assert connection.pos_to.room in graph_rooms, "connection.to.room が rooms にありません"
        assert connection.pos_to.door in range(6), "door が [0, 5] の範囲外です"
        # 自己辺があるのでサボる
        # assert (connection.pos_from.room, connection.pos_from.door) not in used_door
        # assert (connection.pos_from.room, connection.pos_to.door) not in used_door
        used_door.add((connection.pos_from.room, connection.pos_from.door))
        used_door.add((connection.pos_from.room, connection.pos_to.door))
    for room in graph_rooms:
        for door in range(6):
            assert (room, door) in used_door, f"({room}, {door}) が connections にありません"
