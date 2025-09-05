from pydantic import BaseModel, Field


class SelectRequest(BaseModel):
    id: str
    problem_name: str = Field(alias="problemName")


class SelectResponse(BaseModel):
    problem_name: str = Field(alias="problemName")


class ExploreRequest(BaseModel):
    id: str
    plans: list[str]


class ExploreResponse(BaseModel):
    results: list[list[int]]
    query_count: int = Field(alias="queryCount")


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


class GuessRequest(BaseModel):
    id: str
    map: Graph


class GuessResponse(BaseModel):
    correct: bool
