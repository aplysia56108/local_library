/*
https://algo-logic.info/union-find-tree/
https://ei1333.github.io/luzhiled/snippets/structure/union-find.html
を参考に

UnionFind u(n):素集合系管理の構造体(union by rank)
same(x, y):x と y が同じ集合にいるか。 計算量はならし O(α(n))
merge(x, y):x と y を同じ集合にする。計算量はならし O(α(n))
leader(x):xの代表元
size(k):要素kが属する集合の要素数
vector<int> roots():根の一覧。O（n）
int group_size():根の数。O(1)
vector<vector<int>> groups(): グラフを連結成分に分け、各連結成分に属する
*/
struct UnionFind {  // The range of node number is from 0 to n-1
    //'rank[x]' is a rank of the union find tree the root of which is x.
    //'parents[x]' is the parent of x
    vector<int> rank, parents;
    vector<int> data;
    int n, groupsize;
    // constructor
    UnionFind() {
    }
    UnionFind(int n) : n(n), groupsize(n) {  // make n trees.
        rank.resize(n, 0);
        parents.resize(n, 0);
        data.assign(n, -1);
        for (int i = 0; i < n; i++) {
            parents[i] = i;  // the parent of x is x
            rank[i] = 0;
        }
    }

    // check whether the root of x is the same as that of y
    bool same(int x, int y) {
        return leader(x) == leader(y);
    }

    // merge two tree
    bool merge(int x, int y) {
        x = leader(x);
        y = leader(y);
        if (x == y) return false;
        groupsize--;
        if (rank[x] < rank[y]) {
            swap(x, y);
        }
        parents[y] = x;
        data[x] += data[y];

        if (rank[x] == rank[y]) {
            rank[x]++;
        }
        return true;
    }

    // travel the parents of tree recursivily to find root
    int leader(int x) {
        if (x != parents[x]) {
            parents[x] = leader(
                parents[x]);  // change the x's parent to the root of tree.
        }
        return parents[x];
    }

    int size(int k) {
        return (-data[leader(k)]);
    }

    vector<int> roots() {
        vector<int> v;
        for (int i = 0; i < n; i++) {
            if (parents[i] == i) {
                v.push_back(i);
            }
        }
        return v;
    }

    int group_size() {
        return groupsize;
    }
    vector<std::vector<int>> groups() {
        std::vector<int> leader_buf(n), group_size(n);
        for (int i = 0; i < n; i++) {
            leader_buf[i] = leader(i);
            group_size[leader_buf[i]]++;
        }
        vector<std::vector<int>> result(n);
        for (int i = 0; i < n; i++) {
            result[i].reserve(group_size[i]);
        }
        for (int i = 0; i < n; i++) {
            result[leader_buf[i]].push_back(i);
        }
        result.erase(
            remove_if(result.begin(), result.end(),
                      [&](const std::vector<int> &v) { return v.empty(); }),
            result.end());
        return result;
    }
};
