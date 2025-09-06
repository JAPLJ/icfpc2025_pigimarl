import random
from argparse import ArgumentParser

from api_handler import ApiHandler
from schema import (
    ExploreRequest,
    GuessRequest,
    ProblemName,
    SelectRequest,
    get_problem_size,
)
from yaml import FullLoader, load

from solver import solve


def create_random_plan(n: int) -> str:
    return "".join(str(random.randrange(6)) for _ in range(18 * n))

def main():
    parser = ArgumentParser()
    parser.add_argument("--env", type=str, default="local", choices=["local", "production"])
    args = parser.parse_args()

    with open("config.yaml", "r") as f:
        config = load(f, Loader=FullLoader)

    api_domain = config["api_domain"][args.env]
    request_timeout = config["request_timeout"]

    api_handler = ApiHandler(api_domain, request_timeout)

    select_request = SelectRequest(id="dummy", problem_name=ProblemName(config["problem_name"]))
    select_response = api_handler.select(select_request)
    print(select_response)

    n = get_problem_size(ProblemName(config["problem_name"]))
    plans = [create_random_plan(n)]
    explore_request = ExploreRequest(id="dummy", plans=plans)
    explore_response = api_handler.explore(explore_request)
    print(explore_response)

    map_data = solve(n, plans[0], explore_response.results[0])
    guess_request = GuessRequest(id="dummy", map=map_data)
    guess_response = api_handler.guess(guess_request)
    print(guess_response)


if __name__ == "__main__":
    main()
