/*
https://trap.jp/post/1489/
を参考に

kDTree(a.begin(),a.end()):コンストラクタ
get(x,y):[-INF, x] * [-INF, y] にある点の重みを数える。O(sqet(n))
get(x1,y1,x2,y2):[x1,x2]*[y1,y2]にある点の重みを数える。
*/

using T = array<int, 3>;
struct kDTree {
    using Iter = vector<T>::iterator;
    kDTree *l = nullptr, *r = nullptr;
    int xmin = INT_MAX, xmax = INT_MIN, ymin = INT_MAX, ymax = INT_MIN;
    ll sum = 0;
    kDTree(Iter begin, Iter end, bool divx = true) {
        for (auto p = begin; p != end; p++) {
            auto [x, y, w] = *p;
            xmin = min(xmin, x);
            xmax = max(xmax, x);
            ymin = min(ymin, y);
            ymax = max(ymax, y);
            sum += w;
        }
        const int size = signed(end - begin);
        if (size <= 1) return;
        auto cen = begin + size / 2;
        if (divx) {
            nth_element(begin, cen, end,
                        [](T &a, T &b) { return a[0] < b[0]; });
        } else {
            nth_element(begin, cen, end,
                        [](T &a, T &b) { return a[1] < b[1]; });
        }
        l = new kDTree(begin, cen, !divx);
        r = new kDTree(cen, end, !divx);
    }
    // [-INF, x] * [-INF, y] にある点の重みを数える
    ll get(int x, int y) const {
        // [xmin, xmax] * [ymin, ymax] と [-INF, x] * [-INF, y] に共通部分がない
        if (x < xmin || y < ymin) return 0;
        // [xmin, xmax] * [ymin, ymax] 全体が [-INF, x] * [-INF, y]
        // に含まれている
        if (xmax <= x && ymax <= y) return sum;
        // [xmin, xmax] * [ymin, ymax] の一部が [x1, x2] * [y1, y2]
        // に含まれている -> 子に任せる
        return l->get(x, y) + r->get(x, y);
    }

    //[x1,x2]*[y1,y2]に含まれる値
    ll get(int x1, int x2, int y1, int y2) {
        return get(x2, y2) - get(x2, y1 - 1) - get(x1 - 1, y2) +
               get(x1 - 1, y1 - 1);
    }
};
