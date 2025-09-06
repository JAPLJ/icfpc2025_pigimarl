from enum import StrEnum
from typing import Annotated

from pydantic import BaseModel, Field


class ProblemName(StrEnum):
    PROBATIO = "probatio"
    PRIMUS = "primus"
    SECUNDUS = "secundus"
    TERTIUS = "tertius"
    QUARTUS = "quartus"
    QUINTUS = "quintus"

def get_problem_size(problem_name: ProblemName) -> int:
    return {
        ProblemName.PROBATIO: 3,
        ProblemName.PRIMUS: 6,
        ProblemName.SECUNDUS: 12,
        ProblemName.TERTIUS: 18,
        ProblemName.QUARTUS: 24,
        ProblemName.QUINTUS: 30,
    }[problem_name]

class SelectRequest(BaseModel):
    id: str
    problem_name: ProblemName = Field(..., serialization_alias="problemName")

class SelectResponse(BaseModel):
    problem_name: ProblemName = Field(..., validation_alias="problemName")

type ExplorePlan = Annotated[str, Field(pattern=r"^[0-5]+$")]

class ExploreRequest(BaseModel):
    id: str
    plans: list[ExplorePlan]

class ExploreResponse(BaseModel):
    results: list[list[int]]
    query_count: int = Field(validation_alias="queryCount")

class RoomDoor(BaseModel):
    room: int
    door: int

class Connection(BaseModel):
    src: RoomDoor = Field(serialization_alias="from")
    dst: RoomDoor = Field(serialization_alias="to")


class MapData(BaseModel):
    rooms: list[int]
    starting_room: int = Field(serialization_alias="startingRoom")
    connections: list[Connection]

class GuessRequest(BaseModel):
    id: str
    map: MapData

class GuessResponse(BaseModel):
    correct: bool
