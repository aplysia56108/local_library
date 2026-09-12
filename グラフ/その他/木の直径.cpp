/*
木の直径(unit cost)

tree_diamater(g):コンストラクタ
pair<int,pair<int,int>> build():直径の長さと両端点を返す。
vector<int> path():直径のpathを返す。
*/

struct tree_diameter {
    vector<vector<int>> g;
    vector<int> dp0, dp1, dp2;
    int n;
    int r1, r2;
    int d;
    tree_diameter(vector<vector<int>> &g)
        : g(g), n(g.size()), r1(0), r2(0), d(0) {
        dp0.resize(n);
        dp1.resize(n);
        dp2.resize(n);
    }
    void dfs(vector<int> &dp, int v, int p = -1) {
        for (auto to : g[v]) {
            if (to == p) continue;
            dp[to] = dp[v] + 1;
            dfs(dp, to, v);
        }
    }

    pair<int, pair<int, int>> build() {
        dfs(dp0, 0);
        for (int i = 0; i < n; i++) {
            if (dp0[i] > dp0[r1]) r1 = i;
        }
        dfs(dp1, r1);
        for (int i = 0; i < n; i++) {
            if (dp1[i] > dp1[r2]) r2 = i;
        }
        dfs(dp2, r2);
        d = dp1[r2];
        return {d, {r1, r2}};
    }

    void dfs_path(vector<int> &PATH, int v, int p = -1) {
        PATH.push_back(v);
        for (auto to : g[v]) {
            if (to == p) continue;
            if (dp1[to] + dp2[to] != d) continue;
            dfs_path(PATH, to, v);
        }
    }

    vector<int> path() {
        vector<int> PATH;
        dfs_path(PATH, r1);
        return PATH;
    }
};

/*
https://ei1333.github.io/luzhiled/snippets/tree/tree-diameter.html
を参考に

辺にコストがある場合

WeightedGraphにedge(重みのデフォルトは1)を追加
tree_diameter:木の直径の距離
*/
// template <typename T = int>
// struct edge {
//     int to;
//     T cost;

//     edge(int to, T cost = 1) : to(to), cost(cost) {
//     }

//     edge &operator=(const int &x) {
//         to = x;
//         return *this;
//     }

//     operator int() const {
//         return to;
//     }
// };

// template <typename T>
// using Edges = vector<edge<T>>;
// template <typename T>
// using WeightedGraph = vector<Edges<T>>;

// template <typename T>
// pair<T, int> dfs(const WeightedGraph<T> &g, int idx, int par) {
//     pair<T, int> ret(0, idx);
//     for (auto &e : g[idx]) {
//         if (e.to == par) continue;
//         auto cost = dfs(g, e.to, idx);
//         cost.first += e.cost;
//         ret = max(ret, cost);
//     }
//     return ret;
// }

// template <typename T>
// T tree_diameter(const WeightedGraph<T> &g) {
//     auto p = dfs(g, 0, -1);
//     auto q = dfs(g, p.second, -1);
//     return (q.first);
// }
