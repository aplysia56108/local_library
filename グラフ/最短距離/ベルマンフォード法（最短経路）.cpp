/*
Bellman_Ford<T> bf(int v,vector<edge<T>> ES):コンストラクタ
bool build(int　s):
頂点sからの各点への最短距離を求める。負経路があった場合falseを返す。O(|V||E|)
vector<int> get_path(int t):頂点tへの経路復元
*/
template <typename T>
struct edge {
    int from, to;
    T cost;
};

template <typename T>
struct Bellman_Ford {
    vector<edge<T>> es;
    int V, E;
    vector<T> d;
    vector<int> prev1;
    T INF;

    Bellman_Ford(int v, vector<edge<T>> Es)
        : es(Es),
          V(v),
          E(Es.size()),
          d(v),
          prev1(v),
          INF(numeric_limits<T>::max()){};

    // s番目の頂点から各頂点への最短距離を求める。もし負経路が存在したらfalseを返す。
    bool build(int s) {
        for (int i = 0; i < V; i++) d[i] = INF;
        d[s] = 0;
        int k = 0;
        for (int i = 0; i < V; i++) prev1[i] = -1;
        while (true) {
            bool update = false;
            for (int i = 0; i < E; i++) {
                edge e = es[i];
                if (d[e.from] != INF && d[e.to] > d[e.from] + e.cost) {
                    d[e.to] = d[e.from] + e.cost;
                    prev1[e.to] = e.from;
                    update = true;
                }
            }
            if (!update) break;
            k++;
            if (k == V) return false;
        }
        return true;
    }

    //頂点tへの最短経路復元
    vector<int> get_path(int t) {
        vector<int> path;
        for (; t != -1; t = prev1[t]) path.push_back(t);
        reverse(path.begin(), path.end());
        return path;
    }
};