class UnionFind:
    def __init__(self, n):
        self.parent = list(range(n))
        self.rank = [0] * n

    def find(self, x):
        if self.parent[x] != x:
            self.parent[x] = self.find(self.parent[x])
        return self.parent[x]

    def union(self, x, y):
        x = self.find(x)
        y = self.find(y)
        if x == y:
            return
        if self.rank[x] < self.rank[y]:
            self.parent[x] = y
        else:
            self.parent[y] = x
            if self.rank[x] == self.rank[y]:
                self.rank[x] += 1


class UnionFindList:
    def __init__(self, list_):
        distinct_list = set(list_)
        n = len(distinct_list)
        self.uf = UnionFind(n)
        self.hash_map = {x: i for i, x in enumerate(distinct_list)}
        self.reverse_hash_map = {i: x for x, i in self.hash_map.items()}

    def find(self, x):
        return self.reverse_hash_map[self.uf.find(self.hash_map[x])]

    def union(self, x, y):
        self.uf.union(self.hash_map[x], self.hash_map[y])

class UnionFindDict:
    def __init__(self, list_):
        distinct_list = set(list_)
        self.parent = {x: x for x in distinct_list}
        self.rank = {x: 0 for x in distinct_list}

    def find(self, x):
        if self.parent[x] != x:
            self.parent[x] = self.find(self.parent[x])
        return self.parent[x]

    def union(self, x, y):
        x = self.find(x)
        y = self.find(y)
        if x == y:
            return
        if self.rank[x] < self.rank[y]:
            self.parent[x] = y
        else:
            self.parent[y] = x
            if self.rank[x] == self.rank[y]:
                self.rank[x] += 1

    def add(self, x):
        if x not in self.parent:
            self.parent[x] = x
            self.rank[x] = 0

    def same(self, x, y):
        if x not in self.parent or y not in self.parent:
            return False
        return self.find(x) == self.find(y)

    def copy(self):
        new_uf = UnionFindDict(list(self.parent.keys()))
        new_uf.parent = self.parent.copy()
        new_uf.rank = self.rank.copy()
        return new_uf

if __name__ == "__main__":
    uf = UnionFindDict([6, 2, "a", 4, 5])
    print(uf.find(6))
    print(uf.find(2))
    print(uf.find("a"))
    print(uf.find(5))
    uf.union(6, 2)
    print("--------------------------------")
    print(uf.find(6))
    print(uf.find(2))
    print(uf.find("a"))
    print(uf.find(5))
    uf.add(111)
    print(uf.find(111))
    uf.union(111, 6)
    print(uf.find(111))
    print(uf.find(6))
