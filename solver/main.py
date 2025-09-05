import random

from api import Problem, Client
from api_type import Graph


problems = [
    Problem("probatio", 3),
    Problem("primus", 6),
    Problem("secundus", 12),
    Problem("tertius", 18),
    Problem("quartus", 24),
    Problem("quintus", 30),
]

PROBLEM_ID = 0

DUMMY_GRAPH = Graph(**{
    "rooms": [0, 1, 2],
    "startingRoom": 0,
    "connections": [
        {
            "from": {
                "room": 0,
                "door": 0
            },
            "to": {
                "room": 1,
                "door": 1
            }
        },
        {
            "from": {
                "room": 2,
                "door": 2
            },
            "to": {
                "room": 3,
                "door": 3
            }
        }
    ]
})

if __name__ == "__main__":
    client = Client()

    client.select(problems[PROBLEM_ID])
    graph_size = problems[PROBLEM_ID].size

    plan = ""
    for i in range(18 * graph_size):
        plan += str(random.randrange(6))
    response = client.explore(graph_size, [plan])

    client.guess(DUMMY_GRAPH)
