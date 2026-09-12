template <class T = int>
struct Acc {
#define assertion_Acc
    vector<T> acc;
    Acc() {
        acc.push_back(0);
    }
    Acc(vector<T> v) {
        int n = v.size();
        acc.resize(n + 1);
        for (int i = 0; i < n; i++) {
            acc[i + 1] = acc[i] + v[i];
        }
    }

    T back() {
        return acc.back();
    }

    int size() {
        return acc.size() - 1;
    }

    void push_back(T x) {
        acc.push_back(acc.back() + x);
    }

    // range sum of [l, r)
    T sum(int l, int r) {
#ifdef assertion_Acc
        assert(0 <= l);
        assert(l <= r);
        assert(r <= (int)acc.size() - 1);
        return acc[r] - acc[l];
#else
        if (l > r) return 0;
        l = clamp(l, (int)0, (int)(acc.size() - 1));
        r = clamp(r, (int)0, (int)(acc.size() - 1));
        return acc[r] - acc[l];
#endif
    }

    T sum(int r) {
#ifdef assertion_Acc
        assert(0 <= r);
        assert(r < (int)acc.size() - 1);
#else
        r = clamp(r, (int)0, (int)(acc.size() - 2));
#endif
        return acc[r + 1];
    }

    T operator()(int l, int r) {
        return sum(l, r);
    }

    T operator()(int r) {
        return sum(r);
    }

    T operator[](int i) {
        assert(0 <= i);
        assert(i < acc.size() - 1);
        return acc[i + 1] - acc[i];
    }
};
