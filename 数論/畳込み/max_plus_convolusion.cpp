/*
https://atcoder.jp/contests/abc348/submissions/52124047
を参考に

res[i+j]=max(a[i]+b[j])
bは単調増加かつ凸が必要
O(n log n);
*/
vector<ll> max_plus_convolusion(vector<ll> a, vector<ll> b) {
    int na = a.size(), nb = b.size();
    int n = na + nb - 1;
    auto f = [&](int i, int j) {
        if (0 <= i - j && i - j < nb) return a[j] + b[i - j];
        return -inf;
    };
    auto get = [&](auto get, int dep = 0) -> vi {
        int m = ((n - 1) >> dep) + 1;
        if (m == 1) {
            int res = -1, mx = -inf;
            rep(j, na) {
                int now = f(0, j);
                if (chmax(mx, now)) res = j;
            }
            return {res};
        }
        auto js = get(get, dep + 1);
        js.push_back(na - 1);
        vector<ll> res(m);
        rep(i, m) {
            if (i % 2 == 0) {
                res[i] = js[i / 2];
            } else {
                int l = js[i / 2], r = js[i / 2 + 1];
                int bj = l;
                int mx = -inf;
                for (int j = l; j <= r; j++) {
                    ll now = f(i << dep, j);
                    if (chmax(mx, now)) bj = j;
                }
                res[i] = bj;
            }
        }
        return res;
    };
    vector<int> js = get(get);
    vector<ll> res(n);
    rep(i, n) res[i] = f(i, js[i]);
    return res;
}
