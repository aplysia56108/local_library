/*
https://nyaannyaan.github.io/library/data-structure/rollback-union-find.hpp.html
を参考に

RollbackUnionFind(int sz):コンストラクタ、サイズszのUnionFind
merge(x,y):x,yをmerge、mergeしたらtrue,else flase を返す
leader(int k):kの根　O(log n)
same(x,y):xとyが同じ連結成分か　O(log n)
undo():直線のmergeを取り消す　O(1)
snapshot():現在の状態を保存。O(1)
get_state();現在のmergeが呼ばれた回数を返す。O(1)
rollback(int
state=-1):state==-1の時、snapshot()で保存した状態に、そうでない時、mergeがstate回呼び出された時の状態にロールバックする。O(undoの回数)
*/
struct RollbackUnionFind {
    vector<int> data;
    stack<pair<int, int>> history;
    int inner_snap;

    RollbackUnionFind(int sz) : inner_snap(0) {
        data.assign(sz, -1);
    }

    bool merge(int x, int y) {
        x = leader(x), y = leader(y);
        history.emplace(x, data[x]);
        history.emplace(y, data[y]);
        if (x == y) return false;
        if (data[x] > data[y]) swap(x, y);
        data[x] += data[y];
        data[y] = x;
        return true;
    }

    int leader(int k) {
        if (data[k] < 0) return k;
        return leader(data[k]);
    }

    int same(int x, int y) {
        return leader(x) == leader(y);
    }

    int size(int k) {
        return (-data[leader(k)]);
    }

    void undo() {
        data[history.top().first] = history.top().second;
        history.pop();
        data[history.top().first] = history.top().second;
        history.pop();
    }

    void snapshot() {
        inner_snap = signed(history.size() >> 1);
    }

    int get_state() {
        return signed(history.size() >> 1);
    }

    void rollback(int state = -1) {
        if (state == -1) state = inner_snap;
        state <<= 1;
        assert(state <= (int)history.size());
        while (state < (int)history.size()) undo();
    }
};
