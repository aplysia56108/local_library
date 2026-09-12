/*
https://kopricky.github.io/code/Graph/euler_trail.html
を参考に

グラフが連結であることを仮定

一筆書きできるようなグラフ
オイラーグラフ：始点と終点が同じ
準オイラーグラフ：始点と終点が異なる

add_edge(u,v):辺を追加
solve():(準)オイラーグラフかどうか(true/false)を返す。
        trueならansにオイラー路が返る。O(m)

無効グラフの時は上、有効グラフの時は下
準オイラーの時はjudge関数のコメントアウトを消す。
*/
//無効グラフ
class EulerPath {
   private:
    struct edge {
        int to;
        list<edge>::const_iterator rev;
        edge(int to_) : to(to_) {
        }
    };

   public:
    int V;
    list<edge>* G;
    vector<int> ans;
    EulerPath(int node_size) : V(node_size) {
        G = new list<edge>[V];
    }
    void add_edge(int u, int v) {
        G[u].push_back((edge){v});
        list<edge>::const_iterator revu = (--G[u].end());
        G[v].push_back((edge){u});
        list<edge>::const_iterator revv = (--G[v].end());
        G[u].back().rev = revv, G[v].back().rev = revu;
    }
    //オイラー路
    int judge() {
        for (int i = 0; i < V; i++) {
            if ((int)G[i].size() % 2) return -1;
        }
        return 0;
    }
    //準オイラー路
    // int judge() {
    //     int s = -1, odd = 0;
    //     for (int i = 0; i < V; i++) {
    //         if ((int)G[i].size() % 2) {
    //             if (++odd >= 3) return -1;
    //             s = i;
    //         }
    //     }
    //     return max(s, 0);
    // }
    bool solve() {
        int cur = judge();
        if (V == 0 || cur < 0) return false;
        stack<int> cur_path;
        cur_path.push(cur);
        while (!cur_path.empty()) {
            if (!G[cur].empty()) {
                cur_path.push(cur);
                auto nx = G[cur].back();
                G[cur].pop_back(), G[nx.to].erase(nx.rev);
                cur = nx.to;
            } else {
                ans.push_back(cur);
                cur = cur_path.top(), cur_path.pop();
            }
        }
        return true;
    }
};

//有効グラフ
// class EulerPath {
//    public:
//     int V;
//     vector<vector<int> > G;
//     vector<int> degree, ans;
//     EulerPath(int node_size) : V(node_size), G(V), degree(V, 0) {
//     }
//     void add_edge(int u, int v) {
//         G[u].push_back(v), degree[u]++, degree[v]--;
//     }
//     //オイラー路
//     int judge() {
//         for (int i = 0; i < V; i++) {
//             if (degree[i] % 2) return -1;
//         }
//         return 0;
//     }
//     //準オイラー路
//     // int judge() {
//     //     int s = -1;
//     //     bool out_flg = false, in_flg = false;
//     //     for (int i = 0; i < V; i++) {
//     //         if (degree[i] == 0) continue;
//     //         if (degree[i] == 1) {
//     //             if (out_flg) return -1;
//     //             out_flg = true, s = i;
//     //         } else if (degree[i] == -1) {
//     //             if (in_flg) return -1;
//     //             in_flg = true;
//     //         } else
//     //             return -1;
//     //     }
//     //     return max(s, 0);
//     // }
//     bool solve() {
//         int cur = judge();
//         if (V == 0 || cur < 0) return false;
//         stack<int> cur_path;
//         cur_path.push(cur);
//         while (!cur_path.empty()) {
//             if (!G[cur].empty()) {
//                 cur_path.push(cur);
//                 int nx = G[cur].back();
//                 G[cur].pop_back();
//                 cur = nx;
//             } else {
//                 ans.push_back(cur);
//                 cur = cur_path.top(), cur_path.pop();
//             }
//         }
//         reverse(ans.begin(), ans.end());
//         return true;
//     }
// };
