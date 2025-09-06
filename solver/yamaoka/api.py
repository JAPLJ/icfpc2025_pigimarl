import requests
from api_type import SelectRequest, SelectResponse, ExploreRequest, ExploreResponse, GuessRequest, GuessResponse
from type import Problem, Graph, check_plans, check_graph


def post_select(api_domain: str, session: requests.Session, request: SelectRequest) -> SelectResponse:
    headers = {'Content-type': 'application/json'}
    response = session.post(
        api_domain + "/select",
        headers=headers,
        data=request.model_dump_json(by_alias=True)
    ).json()
    if "error" in response:
        print(response["error"])
        exit()
    return SelectResponse(**response)


def post_explore(api_domain: str, session: requests.Session, request: ExploreRequest) -> ExploreResponse:
    headers = {'Content-type': 'application/json'}
    response = session.post(
        api_domain + "/explore",
        headers=headers,
        data=request.model_dump_json(by_alias=True)
    ).json()
    if "error" in response:
        print(response["error"])
        exit()
    return ExploreResponse(**response)


def post_guess(api_domain: str, session: requests.Session, request: GuessRequest) -> GuessResponse:
    headers = {'Content-type': 'application/json'}
    response = session.post(
        api_domain + "/guess",
        headers=headers,
        data=request.model_dump_json(by_alias=True)
    ).json()
    if "error" in response:
        print(response["error"])
        exit()
    return GuessResponse(**response)


class Client:
    api_domain: str
    token: str
    debug: bool
    session: requests.Session
    problem: Problem

    def __init__(self, api_domain: str, token: str, debug: bool):
        self.api_domain = api_domain
        self.token = token
        self.debug = debug

    def select(self, problem: Problem) -> None:
        self.session = requests.session()
        self.problem = problem
        response = post_select(
            self.api_domain,
            self.session,
            SelectRequest(id=self.token, problemName=problem.name)
        )
        if self.debug:
            print(f"SELECT problem:{problem.name} (size:{problem.size})")
        assert response.problem_name == problem.name

    def explore(self, plans: list[str]) -> list[list[int]]:
        check_plans(self.problem.size, plans)
        response = post_explore(
            self.api_domain,
            self.session,
            ExploreRequest(id=self.token, plans=plans)
        )
        if self.debug:
            print(f"EXPLORE plan:{plans}, response:{response}")
        return response.results

    def guess(self, graph: Graph) -> bool:
        check_graph(self.problem.size, graph)
        response = post_guess(
            self.api_domain,
            self.session,
            GuessRequest(id=self.token, map=graph)
        )
        if self.debug:
            print(f"GUESS graph:{graph}, response:{response}")
        return response.correct
