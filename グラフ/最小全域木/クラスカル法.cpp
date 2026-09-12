/*
Kruskal(int v):頂点数v
add_edge(u,v,cost):uとvの間にcostの辺
build():最少全域木の総コストを返す。
 */

// Union Findをはる

struct Kruskal {
    struct Edge {
        int u, v, cost;
        Edge() {
        }
        Edge(int u, int v, int cost) : u(u), v(v), cost(cost) {
        }
        bool operator<(const Edge &other) const {
            return cost < other.cost;
        }
        bool operator>(const Edge &other) const {
            return cost > other.cost;
        }
    };

    int V;
    vector<Edge> es;
    UnionFind uf;

    Kruskal(int v) : V(v), uf(v){};

    void add_edge(int u, int v, int cost) {
        es.emplace_back(u, v, cost);
    }

    int build() {
        sort(es.begin(), es.end());
        int res = 0;
        for (auto e : es) {
            if (!uf.same(e.u, e.v)) {
                uf.merge(e.u, e.v);
                res += e.cost;
            }
        }
        return res;
    }
};
/////
