/*
https://kopricky.github.io/code/DataStructure_OnGraph/partial_persistent_union_find.html
を参考に


leader(u,t):時刻tにおいてuを含む木の根
merge(u,v,t):時刻tにおいてu,vを同じ連結成分にする(tは最後のmerge以降の時間にする)
same(u,v,t):時刻tの直後においてuとvが同じ連結成分に入っているか判定する
size(u,t):時刻tの直後においてuを含む木の頂点数

各時間計算量:O(log n)
*/

class PartialPersistentUnionFind {
   public:
    const int V;
    int last;
    vector<pair<int, int>> par;
    vector<vector<pair<int, int>>> sz;
    PartialPersistentUnionFind(const int node_size)
        : V(node_size), last(numeric_limits<int>::min()), par(V), sz(V) {
        for (int i = 0; i < V; ++i) {
            par[i] = {numeric_limits<int>::max(), i};
            sz[i].emplace_back(numeric_limits<int>::min(), 1);
        }
    }
    int leader(int u, const int _time) {
        while (par[u].first <= _time) u = par[u].second;
        return u;
    }
    bool merge(int u, int v, const int _time) {
        assert(last <= _time);
        last = _time;
        u = leader(u, _time), v = leader(v, _time);
        if (u == v) return false;
        if (sz[u].back().first < sz[v].back().first) swap(u, v);
        par[v] = {_time, u};
        sz[u].emplace_back(_time, sz[u].back().second + sz[v].back().second);
        return true;
    }
    bool same(const int u, const int v, const int _time) {
        return leader(u, _time) == leader(v, _time);
    }
    int size(int u, const int _time) {
        u = leader(u, _time);
        return sz[u][static_cast<int>(upper_bound(sz[u].begin(), sz[u].end(),
                                                  make_pair(_time, V + 1)) -
                                      sz[u].begin()) -
                     1]
            .second;
    }
};
