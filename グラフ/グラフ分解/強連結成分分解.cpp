/*
https://ei1333.github.io/luzhiled/snippets/graph/strongly-connected-components.html　を参考に

StronglyConnectedComponents(vector<vector<int> > g):グラフgで初期化
built(t):tに強連結成分分解してトポロジカルソートされたグラフtが返る。(O(n+m))
        多重辺が生えているので注意。
        多重辺を削除したい場合は、最後のコメントアウトを外す。O(mlog m)
[k]:頂点kが属する強連結成分の番号を返す
*/

struct SCC {
    const vector<vector<int>> &g;
    vector<vector<int>> gg, rg;
    vector<int> comp, order, used;

    SCC(vector<vector<int>> &g)
        : g(g), gg(g.size()), rg(g.size()), comp(g.size(), -1), used(g.size()) {
        for (int i = 0; i < (int)g.size(); i++) {
            for (auto e : g[i]) {
                gg[i].emplace_back((int)e);
                rg[(int)e].emplace_back(i);
            }
        }
    }

    int operator[](int k) {
        return comp[k];
    }

    void dfs(int idx) {
        if (used[idx]) return;
        used[idx] = true;
        for (int to : gg[idx]) dfs(to);
        order.push_back(idx);
    }

    void rdfs(int idx, int cnt) {
        if (comp[idx] != -1) return;
        comp[idx] = cnt;
        for (int to : rg[idx]) rdfs(to, cnt);
    }

    vector<vector<int>> build() {
        for (int i = 0; i < (int)gg.size(); i++) dfs(i);
        reverse(order.begin(), order.end());
        int ptr = 0;
        for (int i : order)
            if (comp[i] == -1) {
                rdfs(i, ptr), ptr++;
            }
        vector<vector<int>> t(ptr);
        for (int i = 0; i < (int)g.size(); i++) {
            for (auto &to : g[i]) {
                int x = comp[i], y = comp[to];
                if (x == y) continue;
                t[x].push_back(y);
            }
        }
        //多重辺削除
        // for (int i = 0; i < ptr; i++) {
        //     sort(t[i].begin(), t[i].end());
        //     t[i].erase(unique(t[i].begin(), t[i].end()), t[i].end());
        // }
        return t;
    }
};
