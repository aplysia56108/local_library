/*
https://codeforces.com/contest/914/submission/75661781
を参考に

https://nyaannyaan.github.io/library/set-function/subset-convolution.hpp.html
https://atcoder.jp/contests/abc294/editorial/5999?lang=ja


s&t==0となるようなs⊆a,t⊆bについてres[s|t]+=a[s]*b[t]
桁数kのとき
O(k^2 2^k)
*/
template <class t, int L = 20>
vector<t> subset_convolution(const vector<t> &a, const vector<t> &b) {
    int s = a.size();
    assert((int)b.size() == s);
    int n = __lg(s);
    assert(s == (1 << n));
    assert(n <= L);
    vector<array<t, L>> x(s), y(s);
    for (int i = 0; i < s - 1; i++) x[i][__builtin_popcountll(i)] = a[i];
    for (int k = 0; k < n; k++)
        for (int i = 0; i < s; i++)
            if (((i >> k) & 1) == 0)
                for (int j = 0; j < n; j++) x[i | 1 << k][j] += x[i][j];
    for (int i = 0; i < s - 1; i++) y[i][__builtin_popcountll(i)] = b[i];
    for (int k = 0; k < n; k++)
        for (int i = 0; i < s; i++)
            if (((i >> k) & 1) == 0)
                for (int j = 0; j < n; j++) y[i | 1 << k][j] += y[i][j];
    for (int i = 0; i < s; i++)
        for (int j = n - 1; j >= 0; j--) {
            for (int k = 1; k < n - j; k++) x[i][j + k] += x[i][j] * y[i][k];
            x[i][j] *= y[i][0];
        }
    for (int k = 0; k < n; k++)
        for (int i = 0; i < s; i++)
            if (((i >> k) & 1) == 0)
                for (int j = 0; j < n; j++) x[i | 1 << k][j] -= x[i][j];
    vector<t> res(s);
    for (int i = 0; i < s - 1; i++) res[i] = x[i][__builtin_popcountll(i)];
    for (int i = 0; i < s; i++) res[s - 1] += a[i] * b[s - 1 - i];
    return res;
}
