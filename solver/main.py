import random

from api import Client
from type import PROBLEMS, Graph, Connection

PROBLEM_ID = 0


if __name__ == "__main__":
    client = Client()

    client.select(PROBLEMS[PROBLEM_ID])
    graph_size = PROBLEMS[PROBLEM_ID].size

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
