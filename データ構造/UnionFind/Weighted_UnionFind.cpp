/*
https://kopricky.github.io/code/DataStructure_OnGraph/weighted_union_find.html
を参考に
天秤を用いて「おもり1がおもり 2よりも5g重く,おもり2がおもり3よりも3g軽い」
みたいなことがわかっていれば「おもり1はおもり3よりも2g重い」ということがわかる.
このような推移的な関係がUnion Findと同じ計算量で行うことができる.

merge(a,b,w):おもりbがおもりaよりwグラム重い
diff(a,b)おもりbがおもりaより何グラム重いかを返す。
same(a,b):a,bが同じかどうか
*/

template <typename T = long long>
class Weighted_UnionFind {
   public:
    int V;
    vector<int> par, nrank;
    vector<T> wd;
    Weighted_UnionFind(int node_size)
        : V(node_size), par(V), nrank(V, 0), wd(V, 0) {
        iota(par.begin(), par.end(), 0);
    }
    int leader(int x) {
        if (par[x] == x) {
            return x;
        } else {
            int parent = leader(par[x]);
            wd[x] += wd[par[x]];
            return par[x] = parent;
        }
    }
    T weight(int x) {
        leader(x);
        return wd[x];
    }
    void merge(int x, int y, T w) {
        w += weight(x), w -= weight(y);
        x = par[x], y = par[y];
        if (x == y) return;
        if (nrank[x] < nrank[y]) {
            swap(x, y);
            w = -w;
        }
        nrank[x] += (nrank[x] == nrank[y]);
        par[y] = x;
        wd[y] = w;
    }
    bool same(int x, int y) {
        return leader(x) == leader(y);
    }
    T diff(int x, int y) {
        return weight(y) - weight(x);
    }
};
