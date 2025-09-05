from argparse import ArgumentParser

from api_handler import ApiHandler
from schema import ProblemName, SelectRequest
from yaml import FullLoader, load


def main():
    parser = ArgumentParser()
    parser.add_argument("--env", type=str, default="local", choices=["local", "production"])
    args = parser.parse_args()

    with open("config.yaml", "r") as f:
        config = load(f, Loader=FullLoader)

    api_domain = config["api_domain"][args.env]
    request_timeout = config["request_timeout"]

    api_handler = ApiHandler(api_domain, request_timeout)

    select_request = SelectRequest(id="dummy", problem_name=ProblemName.PROBATIO)
    select_response = api_handler.select(select_request)
    print(select_response)

if __name__ == "__main__":
    main()
