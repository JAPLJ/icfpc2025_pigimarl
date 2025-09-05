import requests
from api_type import SelectRequest, SelectResponse, ExploreRequest, ExploreResponse, GuessRequest, GuessResponse
from type import Problem, Graph, check_plans, check_graph

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
