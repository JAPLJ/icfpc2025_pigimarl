from argparse import ArgumentParser
import random

from api import Client
from type import PROBLEMS, Graph, Connection
from yaml import FullLoader, load


def main(api_domain: str, token: str):
    client = Client(api_domain, token)

    problem_id = 0
    client.select(PROBLEMS[problem_id])
    graph_size = PROBLEMS[problem_id].size

    plan = ""
    for i in range(18 * graph_size):
        plan += str(random.randrange(6))
    print(f"explore plan: {plan}")
    response = client.explore([plan])
    print(f"explore response: {response}")

    connections: list[Connection] = []
    for room in range(graph_size):
        for door in range(6):
            connections.append(
                Connection(**{
                    "from": {"room": room, "door": door},
                    "to": {"room": room, "door": door}
                })
            )
    dummy_graph = Graph(**{
        "rooms": list(range(graph_size)),
        "startingRoom": 0,
        "connections": connections
    })
    response = client.guess(dummy_graph)
    print(f"result: {str(response)}")


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--env", type=str, default="local", choices=["local", "production"])
    args = parser.parse_args()

    with open("config.yaml", "r") as f:
        config = load(f, Loader=FullLoader)
    api_domain = config["api_domain"][args.env]
    token = config["token"]

    main(api_domain, token)
