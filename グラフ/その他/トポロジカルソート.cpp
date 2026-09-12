/*
https://kopricky.github.io/code/Graph/topological_sort.html を参考に

tsotr(vvi g):コンストラクタ
bool　solve():
閉路があればfalse,DAGならtrueが返り、resにトポロジカル順序にソートされた頂点列が格納される。O(n+m)
*/

struct Tsort {
    vector<vector<int>> g;
    int v;
    vector<int> deg, res;
    Tsort(vector<vector<int>> &G) : g(G), v(G.size()), deg(G.size(), 0) {
    }
    bool solve() {
        for (int i = 0; i < v; i++) {
            for (auto u : g[i]) deg[u]++;
        }
        queue<int> q;
        for (int i = 0; i < v; i++) {
            if (deg[i] == 0) {
                q.push(i);
            }
        }
        while (!q.empty()) {
            int p = q.front();
            q.pop();
            res.push_back(p);
            for (int v : g[p]) {
                if (--deg[v] == 0) {
                    q.push(v);
                }
            }
        }
        return *max_element(deg.begin(), deg.end()) == 0;
    }
    int operator[](int k) {
        return res[k];
    }
};
