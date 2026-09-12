/*
https://ei1333.github.io/luzhiled/snippets/structure/segment-tree.html
を参考に

DualSegmenttree<S,S mapping(S,S),S id> (n)or(vector)
apply(l,r,x):[l,r)にxを作用
[x]
*/
template <typename S, S (*mapping)(S, S), S (*id)()>
struct DualSegmentTree {
   private:
    int sz, height;
    vector<S> lazy;

    void init(int n) {
        sz = 1;
        height = 0;
        while (sz < n) sz <<= 1, height++;
        lazy.assign(2 * sz, id());
    }

    inline void propagate(int k) {
        if (lazy[k] != id()) {
            lazy[2 * k + 0] = mapping(lazy[k], lazy[2 * k + 0]);
            lazy[2 * k + 1] = mapping(lazy[k], lazy[2 * k + 1]);
            lazy[k] = id();
        }
    }

    inline void thrust(int k) {
        for (int i = height; i > 0; i--) propagate(k >> i);
    }

   public:
    DualSegmentTree(int n) {
        init(n);
    }
    DualSegmentTree(vector<S> &a) {
        int n = a.size();
        init(n);
        for (int i = 0; i < n; i++) apply(i, i + 1, a[i]);
    }
    void apply(int a, int b, const S &x) {
        thrust(a += sz);
        thrust(b += sz - 1);
        for (int l = a, r = b + 1; l < r; l >>= 1, r >>= 1) {
            if (l & 1) lazy[l] = mapping(x, lazy[l]), ++l;
            if (r & 1) --r, lazy[r] = mapping(x, lazy[r]);
        }
    }
    S operator[](int k) {
        thrust(k += sz);
        return lazy[k];
    }
};
