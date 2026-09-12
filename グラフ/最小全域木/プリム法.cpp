/*
貪欲的に今の木から最小コストの頂点を追加し、最小全域木を求める。

vector<vector<T>> costを準備
(cost[u][v]:辺u,vが存在したらそのコスト。存在しない場合INF)

Prim<T> p(int v,vector<vector<T>> cost):コンストラクタ
T build():最小全域木を返す。O(|V|^2)
*/
template <typename T>
struct Prim {
    int V;
    vector<vector<T>> cost;
    vector<T> mincost;
    vector<bool> used;
    T INF;

    Prim(int v, vector<vector<T>> c)
        : V(v), cost(c), mincost(v), used(v), INF(numeric_limits<T>::max()){};

    T build() {
        for (int i = 0; i < V; i++) {
            mincost[i] = INF;
            used[i] = false;
        }
        mincost[0] = 0;
        T res = 0;

        while (true) {
            int v = -1;
            for (int u = 0; u < V; u++) {
                if (!used[u] && (v == -1 || mincost[u] < mincost[v])) v = u;
            }
            if (v == -1) break;
            used[v] = true;
            res += mincost[v];

            for (int u = 0; u < V; u++) {
                mincost[u] = min(mincost[u], cost[v][u]);
            }
        }
        return res;
    }
};