/*
https://ei1333.github.io/luzhiled/snippets/graph/lowlink.html を参考に

LowLink(g):グラフgで初期化.articulationに間接点、bridgeに橋が追加される。O(V)
*/

struct LowLink {
    const vector<vector<int>> &g;
    vector<int> used, ord, low;
    vector<int> articulation;
    vector<pair<int, int>> bridge;

    LowLink(const vector<vector<int>> &g) : g(g) {
        build();
    }

    int dfs(int idx, int k, int par) {
        used[idx] = true;
        ord[idx] = k++;
        low[idx] = ord[idx];
        bool is_articulation = false;
        int cnt = 0;
        for (auto &to : g[idx]) {
            if (!used[to]) {
                ++cnt;
                k = dfs(to, k, idx);
                low[idx] = min(low[idx], low[to]);
                is_articulation |= ~par && low[to] >= ord[idx];
                if (ord[idx] < low[to])
                    bridge.emplace_back(minmax(idx, (int)to));
            } else if (to != par) {
                low[idx] = min(low[idx], ord[to]);
            }
        }
        is_articulation |= par == -1 && cnt > 1;
        if (is_articulation) articulation.push_back(idx);
        return k;
    }

    virtual void build() {
        used.assign(g.size(), 0);
        ord.assign(g.size(), 0);
        low.assign(g.size(), 0);
        int k = 0;
        for (int i = 0; i < (int)g.size(); i++) {
            if (!used[i]) k = dfs(i, k, -1);
        }
    }
};
