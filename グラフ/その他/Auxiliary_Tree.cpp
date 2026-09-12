//  required LCA.cpp
/*
https://atcoder.jp/contests/abc340/submissions/50190741
を参考に

https://smijake3.hatenablog.com/entry/2019/09/15/200200

Auxiliary_Tree at(vvi g)
compress(vi vs):
    v同士の関係を失わないように木を圧縮した頂点集合を返す
    グラフはGに
    O(ならし N log N）
*/
struct Auxiliary_Tree {
    int n;
    vector<vector<int>> g;
    vector<int> IN, OUT;
    vector<vector<int>> G;
    LCA<int> lca;
    set<int> used;
    Auxiliary_Tree(vector<vector<int>> &g_)
        : n(g_.size()),
          g(g_),
          IN(g.size()),
          OUT(g_.size()),
          G(g_.size()),
          lca(g) {
        lca.init();

        // オイラーツアー
        int k = 0;
        auto dfs = [&](auto dfs, int v, int p = -1) -> void {
            IN[v] = k++;
            for (auto to : g[v]) {
                if (to == p) continue;
                dfs(dfs, to, v);
            }
            OUT[v] = k;
        };
        dfs(dfs, 0);
    }

    vector<int> compress(vector<int> vs) {
        for (auto c : used) G[c].clear();
        used.clear();
        sort(vs.begin(), vs.end(), [&](int a, int b) { return IN[a] < IN[b]; });
        int m = vs.size();
        rep(i, m - 1) {
            vs.push_back(lca(vs[i], vs[i + 1]));
        }
        sort(all(vs), [&](int a, int b) { return IN[a] < IN[b]; });
        vs.erase(unique(vs.begin(), vs.end()), vs.end());
        for (auto c : vs) used.insert(c);
        {
            vi st;
            for (auto v : vs) {
                while (st.size()) {
                    int p = st.back();
                    if (IN[p] < IN[v] && IN[v] < OUT[p]) break;
                    st.pop_back();
                }
                if (si(st)) G[st.back()].pb(v);
                st.push_back(v);
            }
        }
        return vs;
    }
};
