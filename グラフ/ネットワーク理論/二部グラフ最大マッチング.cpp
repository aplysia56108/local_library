/*
https://ei1333.github.io/luzhiled/snippets/graph/hopcroft-karp.html を参考に

HopcroftKarp(n,m):一方の頂点数をn,もう一方の頂点数をmで初期化する。
add_edge(u,v):頂点u,v間に辺を張る。
build():二部グラフの最大マッチングを返す。O(E√V)
*/

struct HopcroftKarp {
    vector<vector<int> > graph;
    vector<int> dist, match;
    vector<bool> used, vv;

    HopcroftKarp(int n, int m) : graph(n), match(m, -1), used(n) {
    }

    void add_edge(int u, int v) {
        graph[u].push_back(v);
    }

    void bfs() {
        dist.assign(graph.size(), -1);
        queue<int> que;
        for (int i = 0; i < (int)graph.size(); i++) {
            if (!used[i]) {
                que.emplace(i);
                dist[i] = 0;
            }
        }

        while (!que.empty()) {
            int a = que.front();
            que.pop();
            for (auto &b : graph[a]) {
                int c = match[b];
                if (c >= 0 && dist[c] == -1) {
                    dist[c] = dist[a] + 1;
                    que.emplace(c);
                }
            }
        }
    }

    bool dfs(int a) {
        vv[a] = true;
        for (auto &b : graph[a]) {
            int c = match[b];
            if (c < 0 || (!vv[c] && dist[c] == dist[a] + 1 && dfs(c))) {
                match[b] = a;
                used[a] = true;
                return (true);
            }
        }
        return (false);
    }

    int build() {
        int ret = 0;
        while (true) {
            bfs();
            vv.assign(graph.size(), false);
            int flow = 0;
            for (int i = 0; i < (int)graph.size(); i++) {
                if (!used[i] && dfs(i)) ++flow;
            }
            if (flow == 0) return (ret);
            ret += flow;
        }
    }

    void output() {
        for (int i = 0; i < (int)match.size(); i++) {
            if (~match[i]) {
                cout << i << "-" << match[i] << endl;
            }
        }
    }
};
