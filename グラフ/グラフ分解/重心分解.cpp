/*
https://kopricky.github.io/code/GraphDecomposition/centroid_decomposition.html
を参考に

CentroidDecomposition(int node_size)or(vvi g)
add_edge():辺の追加
build():重心分解を行う。O(n log(n)) parを返す
find_cent(u):部分木uの重心を返す。dead[u]=trueにする。
*/

class CentroidDecomposition {
   private:
    int V;
    vector<vector<int>> G;
    // sz:重心分解後の最大部分木に含まれる頂点の数(自分を含める)
    // par:重心分解後の親の頂点
    // 部分木のサイズを計算
    void calcSize(int u, int p) {
        sz[u] = 1;
        for (int v : G[u]) {
            if (!dead[v] && v != p) {
                calcSize(v, u);
                sz[u] += sz[v];
            }
        }
    }
    void cdBuild(int u, int p) {
        u = find_cent(u);
        par[u] = p;
        // 深さ優先でたどる
        for (int v : G[u]) {
            if (!dead[v]) {
                cdBuild(v, u);
            }
        }
    }

   public:
    vector<bool> dead;
    vector<int> sz, par;
    CentroidDecomposition() {
    }
    CentroidDecomposition(int node_size)
        : V(node_size), G(V), dead(V, false), sz(V, 0), par(V, -1) {
    }
    CentroidDecomposition(vector<vector<int>> &g)
        : V(g.size()), G(g), dead(V, false), sz(V, 0), par(V, -1) {
    }
    void add_edge(int u, int v) {
        G[u].push_back(v), G[v].push_back(u);
    }
    // いま見ている部分木での重心を見つける
    int find_cent(int u) {
        calcSize(u, -1);
        int tot = sz[u];
        bool ok = false;
        int pp = -1;
        while (!ok) {
            ok = true;
            for (int v : G[u]) {
                if (!dead[v] && v != pp && 2 * sz[v] > tot) {
                    pp = u, u = v, ok = false;
                    break;
                }
            }
        }
        dead[u] = true;
        return u;
    }
    vector<int> build() {
        cdBuild(0, -1);
        return par;
    }
};
