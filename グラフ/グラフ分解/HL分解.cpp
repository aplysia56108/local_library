/*
https://github.com/drken1215/algorithm/blob/master/DataStructureOnTree/heavy_light_decomposition.cpp
を参考に

HLDecomposition hld(G):インスタンス生成
hld.build():HL木生成
foreach_nodes(u,v,function f):[u,v]間の頂点に対するクエリ
foreach_edge(u,v,fucntion f):[u,v]間の辺に対するクエリ

vid: id of v after HL-Decomposition
inv: inv[vid[v]] = v
par: id of parent
depth
subsize: size of subtree
head: head-id in the heavy-path
prev, next: prev-id, next-id in the heavy-path
type: the id of tree for forest
vend: the last-id of node in v-subtree
*/
typedef vector<vector<int> > Graph;
struct HLDecomposition {
    int n;
    Graph G;
    vector<int> vid, inv, par, depth, subsize, head, prev, next, type;

    // construct
    HLDecomposition() {
    }
    HLDecomposition(const Graph &G_)
        : n((int)G_.size()),
          G(G_),
          vid(n, -1),
          inv(n),
          par(n),
          depth(n),
          subsize(n, 1),
          head(n),
          prev(n, -1),
          next(n, -1),
          type(n) {
    }
    void build(vector<int> roots = {0}) {
        int curtype = 0, pos = 0;
        for (auto r : roots)
            decide_heavy_edge(r), reconstruct(r, curtype++, pos);
    }
    void decide_heavy_edge(int r) {
        stack<pair<int, int> > st;
        par[r] = -1, depth[r] = 0;
        st.emplace(r, 0);
        while (!st.empty()) {
            int v = st.top().first;
            int &i = st.top().second;
            if (i < (int)G[v].size()) {
                int e = G[v][i++];
                if (e == par[v]) continue;
                par[e] = v, depth[e] = depth[v] + 1;
                st.emplace(e, 0);
            } else {
                st.pop();
                int maxsize = 0;
                for (auto e : G[v]) {
                    if (e == par[v]) continue;
                    subsize[v] += subsize[e];
                    if (maxsize < subsize[e])
                        maxsize = subsize[e], prev[e] = v, next[v] = e;
                }
            }
        }
    }
    void reconstruct(int r, int curtype, int &pos) {
        stack<int> st({r});
        while (!st.empty()) {
            int start = st.top();
            st.pop();
            for (int v = start; v != -1; v = next[v]) {
                type[v] = curtype;
                vid[v] = pos++;
                inv[vid[v]] = v;
                head[v] = start;
                for (auto e : G[v])
                    if (e != par[v] && e != next[v]) st.push(e);
            }
        }
    }

    // node query [u, v], f([left, right])
    void foreach_nodes(int u, int v, const function<void(int, int)> &f) {
        while (true) {
            if (vid[u] > vid[v]) swap(u, v);
            f(max(vid[head[v]], vid[u]), vid[v]);
            if (head[u] != head[v])
                v = par[head[v]];
            else
                break;
        }
    }

    // edge query [u, v], f([left, right])
    void foreach_edges(int u, int v, const function<void(int, int)> &f) {
        while (true) {
            if (vid[u] > vid[v]) swap(u, v);
            if (head[u] != head[v]) {
                f(vid[head[v]], vid[v]);
                v = par[head[v]];
            } else {
                if (u != v) {
                    f(vid[u] + 1, vid[v]);
                }
                break;
            }
        }
    }

    // LCA
    int lca(int u, int v) {
        while (true) {
            if (vid[u] > vid[v]) swap(u, v);
            if (head[u] == head[v]) return u;
            v = par[head[v]];
        }
    }
};