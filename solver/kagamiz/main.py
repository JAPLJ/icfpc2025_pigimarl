import random
from argparse import ArgumentParser

import yaml
from api_handler import ApiHandler
from schema import (
    Config,
    ExploreRequest,
    GuessRequest,
    ProblemName,
    SelectRequest,
    get_problem_size,
)

from solver import solve


def create_random_plan(n: int) -> str:
    return "".join(str(random.randrange(6)) for _ in range(18 * n))

def main():
    parser = ArgumentParser()
    parser.add_argument("--env", type=str, default="local", choices=["local", "production"])
    parser.add_argument("--problem", type=str, required=True, 
                       choices=["probatio", "primus", "secundus", "tertius", "quartus", "quintus"],
                       help="Problem name to solve")
    args = parser.parse_args()

    with open("config.yaml", "r") as f:
        config = Config.model_validate(yaml.load(f, Loader=yaml.FullLoader))

    if args.env == "local":
        api_domain = config.api_domain.local
    else:
        api_domain = config.api_domain.production

    api_handler = ApiHandler(api_domain, config.request_timeout)

    # Convert string to ProblemName enum
    problem_name = ProblemName(args.problem)
    
    select_request = SelectRequest(id=config.token, problem_name=problem_name)
    select_response = api_handler.select(select_request)
    print(select_response)

    n = get_problem_size(problem_name)
    plans = [create_random_plan(n)]
    explore_request = ExploreRequest(id=config.token, plans=plans)
    explore_response = api_handler.explore(explore_request)
    print(explore_response)

    map_data = solve(n, plans[0], explore_response.results[0])
    guess_request = GuessRequest(id=config.token, map=map_data)
    guess_response = api_handler.guess(guess_request)
    print(guess_response)


if __name__ == "__main__":
    main()
