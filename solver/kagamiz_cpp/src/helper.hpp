#pragma once

#include "schema.hpp"
#include <vector>
#include <string>

namespace kagamiz {

struct Room {
    int label;
    std::vector<int> doors;
    
    Room(int label, const std::vector<int>& doors) : label(label), doors(doors) {}
};

std::vector<Connection> create_connections(const std::vector<Room>& rooms);
std::string format_rooms(const std::vector<Room>& rooms);

class UnionFind {
    public:
    UnionFind(int n) : parent(n), rank(n, 0) {
        for (int i = 0; i < n; i++) {
            parent[i] = i;
        }
    }
    int find(int x) {
        if (parent[x] == x) {
            return x;
        }
        return parent[x] = find(parent[x]);
    }
    void unite(int x, int y) {
        x = find(x);
        y = find(y);
        if (x == y) {
            return;
        }
        if (rank[x] < rank[y]) {
            parent[x] = y;
        }
        else {
            parent[y] = x;
            if (rank[x] == rank[y]) {
                rank[x]++;
            }
        }
    }
    bool same(int x, int y) {
        return find(x) == find(y);
    }
    private:
    std::vector<int> parent;
    std::vector<int> rank;
};

} // namespace kagamiz
