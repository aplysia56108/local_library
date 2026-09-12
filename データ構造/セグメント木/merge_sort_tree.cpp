/*
https://atcoder.jp/contests/abc339/submissions/49979258
を参考に

set(i,x):i番目がx
init():初期化　O(n log n)
prod(l,r,x):区間[l,r)に含まれるx以下の要素の和 O(logn^2)
*/
template <class T = long long>
struct merge_sort_tree {
    int n;
    vector<vector<T>> a;
    vector<vector<T>> s;
    merge_sort_tree(int mx) {
        n = 1;
        while (n < mx) n <<= 1;
        a.resize(n * 2);
        s.resize(n * 2, vi(1));
    }
    void set(int i, int x) {
        a[n + i] = {x};
        s[n + i].push_back(x);
    }
    void init() {
        rrep(i, 1, n) {
            int l = i << 1, r = l | 1;
            merge(a[l].begin(), a[l].end(), a[r].begin(), a[r].end(),
                  back_inserter(a[i]));
            s[i] = vector<T>(a[i].size() + 1);
            for (int j = 0; j < (int)a[i].size(); j++) {
                s[i][j + 1] = s[i][j] + a[i][j];
            }
        }
    }
    int get(int i, int x) {
        int j = upper_bound(a[i].begin(), a[i].end(), x) - a[i].begin();
        return s[i][j];
    }
    int prod(int l, int r, int x) {
        l += n, r += n;
        int res = 0;
        while (l < r) {
            if (l & 1) res += get(l, x), l++;
            if (r & 1) r--, res += get(r, x);
            l >>= 1, r >>= 1;
        }
        return res;
    }
};
