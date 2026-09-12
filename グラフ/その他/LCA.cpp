/*
https://atcoder.jp/contests/abc133/submissions/6303455
を参考に

コスト付き
LCA<T>:コンストラクタ
add_edge(a,b,c):頂点a,b間にコストcの辺を張る
init():初期化
up(v,k):頂点vのk個上の頂点
(a,b):a,b間のLCA
length(a,b):a,b間の長さ
dist(a,b):a,b間のコスト
*/
template <typename T = int>
struct LCA {
    int n, root, l;
    vector<vector<int>> to;
    vector<vector<T>> co;
    vector<int> dep;
    vector<T> costs;
    vector<vector<int>> par;
    LCA() {
    }
    LCA(int n) : n(n), to(n), co(n), dep(n), costs(n) {
        l = 0;
        while ((1 << l) < n) ++l;
        par = vector<vector<int>>(n + 1, vector<int>(l, n));
    }
    LCA(vector<vector<int>> &g, int root_ = 0)
        : n(g.size()),
          root(root_),
          to(g.size()),
          co(g.size()),
          dep(g.size()),
          costs(g.size()) {
        l = 0;
        while ((1 << l) < n) ++l;
        par = vector<vector<int>>(n + 1, vector<int>(l, n));
        add_edges(g);
        init(root);
    }
    void add_edge(int a, int b, T c = 0) {
        to[a].push_back(b);
        co[a].push_back(c);
        to[b].push_back(a);
        co[b].push_back(c);
    }
    void add_edges(vector<vector<int>> &g) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < (int)g[i].size(); j++) {
                to[i].push_back(g[i][j]);
                co[i].push_back(1);
            }
        }
    }
    void dfs(int v, int d = 0, T c = 0, int p = -1) {
        if (p != -1) par[v][0] = p;
        dep[v] = d;
        costs[v] = c;
        for (int i = 0; i < (int)to[v].size(); ++i) {
            int u = to[v][i];
            if (u == p) continue;
            dfs(u, d + 1, c + co[v][i], v);
        }
    }

    void init(int _root = 0) {
        root = _root;
        dfs(root);
        for (int i = 0; i < l - 1; ++i) {
            for (int v = 0; v < n; ++v) {
                par[v][i + 1] = par[par[v][i]][i];
            }
        }
    }
    // vのk個上の頂点
    int up(int v, int k) {
        for (int i = l - 1; i >= 0; --i) {
            int len = 1 << i;
            if (k >= len) {
                k -= len;
                v = par[v][i];
            }
        }
        return v;
    }
    // lca
    int operator()(int a, int b) {
        if (dep[a] > dep[b]) swap(a, b);
        b = up(b, dep[b] - dep[a]);
        if (a == b) return a;
        for (int i = l - 1; i >= 0; --i) {
            int na = par[a][i];
            int nb = par[b][i];
            if (na != nb) a = na, b = nb;
        }
        return par[a][0];
    }
    int length(int a, int b) {
        int c = (*this)(a, b);
        return dep[a] + dep[b] - dep[c] * 2;
    }
    T dist(int a, int b) {
        int c = (*this)(a, b);
        return costs[a] + costs[b] - costs[c] * 2;
    }
};
