from pydantic import BaseModel, Field

from type import Graph


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


class GuessRequest(BaseModel):
    id: str
    map: Graph


class GuessResponse(BaseModel):
    correct: bool
