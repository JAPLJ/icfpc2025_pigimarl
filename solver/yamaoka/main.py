from argparse import ArgumentParser
import random

from api import Client
from type import PROBLEMS, Graph, Connection
from yaml import FullLoader, load


class UnionFind():
    def __init__(self, n):
        self.n = n
        self.parents = [-1] * n
        self.edges = [[-1] * 6 for i in range(n)]

    def find(self, x):
        if self.parents[x] < 0:
            return x
        else:
            self.parents[x] = self.find(self.parents[x])
            return self.parents[x]

    def union(self, x, y):
        x = self.find(x)
        y = self.find(y)

        if x == y:
            return

        if self.parents[x] > self.parents[y]:
            x, y = y, x

        self.parents[x] += self.parents[y]
        self.parents[y] = x

        unions = []
        for i in range(6):
            if self.edges[y][i] != -1:
                if self.edges[x][i] != -1:
                    unions.append((self.edges[x][i], self.edges[y][i]))
                self.edges[x][i] = self.find(self.edges[y][i])
        for p in unions:
            self.union(p[0], p[1])

    def get_edge(self, x, e):
        x = self.find(x)
        if self.edges[x][e] == -1:
            return -1
        self.edges[x][e] = self.find(self.edges[x][e])
        return self.edges[x][e]

    def add_edge(self, x, e, y):
        if self.edges[x][e] != -1:
            self.union(self.edges[x][e], y)
        self.edges[x][e] = self.find(y)

    def size(self, x):
        return -self.parents[self.find(x)]

    def same(self, x, y):
        return self.find(x) == self.find(y)

    def is_leader(self, x):
        return self.parents[x] < 0


class Zaatsu:
    def __init__(self):
        self.zaatsu_dict = {}
        self.zaatsu_array = []

    def get(self, x):
        if x not in self.zaatsu_dict:
            self.zaatsu_dict[x] = len(self.zaatsu_dict)
            self.zaatsu_array.append(x)
        return self.zaatsu_dict[x]

    def find(self, x):
        if x not in self.zaatsu_dict:
            return None
        return self.zaatsu_dict[x]

    def rev(self, idx):
        if idx >= len(self.zaatsu_array):
            return None
        return self.zaatsu_array[idx]

    def size(self):
        return len(self.zaatsu_dict)


def solve(client: Client, graph_size: int):
    section_length = 6
    section_num = 3 * graph_size // section_length
    bits_length = 3

    plan = []
    sections = list(range(6)) * section_num
    random.shuffle(sections)
    for x in sections:
        plan += [x] * section_length
    # plan = [random.randrange(6) for i in range(18 * graph_size)]
    plan_str = "".join(map(str, plan))
    result = client.explore([plan_str])[0]

    uf = UnionFind(len(result))
    vec_set = set()
    zaatsu = Zaatsu()
    vertex_label = [-1] * len(result)
    for i in range(len(result)):
        if i + bits_length > len(result):
            continue
        vec_set.add(tuple(plan[i:i + bits_length - 1]))
        v = zaatsu.get((tuple(plan[i:i + bits_length - 1]), tuple(result[i:i + bits_length])))
        vertex_label[v] = result[i]

    for i in range(len(result)):
        if i + bits_length + 1 > len(result):
            continue
        u = zaatsu.get((tuple(plan[i:i + bits_length - 1]), tuple(result[i:i + bits_length])))
        v = zaatsu.get((tuple(plan[i + 1:i + bits_length]), tuple(result[i + 1:i + bits_length + 1])))
        uf.add_edge(u, plan[i], v)
        print(u, plan[i], v)

    for i in range(zaatsu.size()):
        if not uf.is_leader(i):
            continue
        print(i, zaatsu.rev(i))
        for vec in vec_set:
            if vec == zaatsu.rev(i)[0]:
                continue
            v = i
            labels = [vertex_label[i]]
            for e in vec:
                v = uf.get_edge(v, e)
                if v == -1:
                    break
                labels.append(vertex_label[v])
            if len(labels) < len(vec):
                continue
            j = zaatsu.find((vec, tuple(labels)))
            if j is None:
                continue
            uf.union(i, j)

    for i in range(zaatsu.size()):
        if not uf.is_leader(i):
            continue
        print(i, [uf.get_edge(i, j) for j in range(6)])

    # connections: list[Connection] = []
    # for room in range(graph_size):
    #     for door in range(6):
    #         connections.append(
    #             Connection(**{
    #                 "from": {"room": room, "door": door},
    #                 "to": {"room": room, "door": door}
    #             })
    #         )
    # dummy_graph = Graph(**{
    #     "rooms": list(range(graph_size)),
    #     "startingRoom": 0,
    #     "connections": connections
    # })
    # client.guess(dummy_graph)


def main(api_domain: str, token: str):
    client = Client(api_domain, token, True)

    problem_id = 2
    client.select(PROBLEMS[problem_id])
    graph_size = PROBLEMS[problem_id].size
    solve(client, graph_size)


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--env", type=str, default="local", choices=["local", "production"])
    args = parser.parse_args()

    with open("config.yaml", "r") as f:
        config = load(f, Loader=FullLoader)
    api_domain = config["api_domain"][args.env]
    token = config["token"]

    main(api_domain, token)
