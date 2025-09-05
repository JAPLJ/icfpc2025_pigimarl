import requests
from api_type import SelectRequest, SelectResponse, ExploreRequest, ExploreResponse, GuessRequest, GuessResponse, Graph

ID = "kensuke_imanishi@r.recruit.co.jp E-ObYlKstlzFqlUNaQbgrA"
URL = "https://31pwr5t6ij.execute-api.eu-west-2.amazonaws.com"


def post_select(session: requests.Session, request: SelectRequest) -> SelectResponse:
    headers = {'Content-type': 'application/json'}
    response = session.post(
        URL + "/select",
        headers=headers,
        data=request.model_dump_json(by_alias=True)
    ).json()
    if "error" in response:
        print(response["error"])
        exit()
    return SelectResponse(**response)


def post_explore(session: requests.Session, request: ExploreRequest) -> ExploreResponse:
    headers = {'Content-type': 'application/json'}
    response = session.post(
        URL + "/explore",
        headers=headers,
        data=request.model_dump_json(by_alias=True)
    ).json()
    if "error" in response:
        print(response["error"])
        exit()
    return ExploreResponse(**response)


def post_guess(session: requests.Session, request: GuessRequest) -> GuessResponse:
    headers = {'Content-type': 'application/json'}
    response = session.post(
        URL + "/guess",
        headers=headers,
        data=request.model_dump_json(by_alias=True)
    ).json()
    if "error" in response:
        print(response["error"])
        exit()
    return GuessResponse(**response)


def check_plans(graph_size: int, plans: list[str]) -> None:
    for plan in plans:
        assert len(plan) <= 18 * graph_size, "パスの長さが 18n を超えています"


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


class Problem:
    name: str
    size: int

    def __init__(self, name: str, size: int):
        self.name = name
        self.size = size


class Client:
    session: requests.Session
    problem: Problem

    def select(self, problem: Problem) -> None:
        self.session = requests.session()
        self.problem = problem
        response = post_select(self.session, SelectRequest(id=ID, problemName=problem.name))
        assert response.problem_name == problem.name

    def explore(self, plans: list[str]) -> list[list[int]]:
        check_plans(self.problem.size, plans)
        response = post_explore(self.session, ExploreRequest(id=ID, plans=plans))
        return response.results

    def guess(self, graph: Graph) -> bool:
        check_graph(self.problem.size, graph)
        response = post_guess(self.session, GuessRequest(id=ID, map=graph))
        return response.correct
