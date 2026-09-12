/*
Dijkstra<T> d1(int n,vector<vector<edge<T>>> g):コンストラクタ
void build(int s):頂点sからの最短距離をd[]に保存
vector<int> get_path(int t):頂点tへの経路復元
*/
template <typename T>
struct edge {
    int to;
    T cost;
};

template <typename T>
struct Dijkstra {
    int V;
    vector<vector<edge<T>>> G;
    vector<T> d;
    vector<int> prev1;

    T INF;

    Dijkstra(int v, vector<vector<edge<T>>> g)
        : V(v), G(g), d(v), prev1(v), INF(numeric_limits<T>::max()) {
        for (int i = 0; i < V; i++) {
            d[i] = INF;
            prev1[i] = -1;
        }
    };

    vector<T> build(int s) {
        priority_queue<pair<T, int>, vector<pair<T, int>>,
                       greater<pair<T, int>>>
            que;
        d[s] = 0;
        que.push(pi(0, s));

        while (!que.empty()) {
            pi p = que.top();
            que.pop();
            int v = p.second;
            if (d[v] < p.first) continue;
            for (int i = 0; i < G[v].size(); i++) {
                edge<T> e = G[v][i];
                if (d[v] != INF) {
                    if (d[e.to] > d[v] + e.cost) {
                        d[e.to] = d[v] + e.cost;
                        prev1[e.to] = v;
                        que.push(pi(d[e.to], e.to));
                    }
                }
            }
        }
        return d;
    }

    //頂点tへの最短経路復元
    vector<int> get_path(int t) {
        vector<int> path;
        for (; t != -1; t = prev1[t]) path.push_back(t);
        reverse(path.begin(), path.end());
        return path;
    }
};