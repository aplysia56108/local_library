/*
https://atcoder.jp/contests/typical90/submissions/30762425
を参考に

[x^n]P(x)/Q(x)を求める
計算量：d次多項式同士の乗算の計算量をM(d)とするとO(M(d)* log(n)))
Q(0)は可逆元である必要がある。

LinearReccurrence(vm a,vm c,n):漸化式cの初期値がn項目(a_(n-1))を求める

BMBM
https://atcoder.jp/contests/arc170/editorial/9153
を参考に
長さkの数列の線形漸化式は先頭の2k項程度から復元可能(Berlekamp_Massey)
nth_term(n,s):先頭の項sからn項目を計算(k log k log n)
*/

mint bostan_mori(vector<mint> num, vector<mint> denom, long long n) {
    while (n) {
        auto denom_bar = denom;
        for (int i = 1; i < (int)denom_bar.size(); i += 2) {
            denom_bar[i] *= -1;
        }
        auto num2 = convolution(num, denom_bar);
        auto denom2 = convolution(denom, denom_bar);
        denom.clear();
        for (int i = 0; i < (int)denom2.size(); i += 2) {
            denom.push_back(denom2[i]);
        }

        num.clear();
        for (int i = n % 2; i < (int)num2.size(); i += 2) {
            num.push_back(num2[i]);
        }
        n /= 2;
    }
    return num[0] / denom[0];
}

mint LinearRecurrence(vector<mint> a, vector<mint> c, int n) {
    int d = c.size();
    vector<mint> q(d + 1);
    q[0] = 1;
    for (int i = 1; i <= d; i++) {
        q[i] = -c[i - 1];
    }
    vector<mint> p = convolution(a, q);
    p.resize(d);
    return bostan_mori(p, q, n);
}

template <typename mint>
vector<mint> Berlekamp_Massey(const vector<mint> &s) {
    const int N = (int)s.size();
    vector<mint> b, c;
    b.reserve(N + 1);
    c.reserve(N + 1);
    b.push_back(mint(1));
    c.push_back(mint(1));
    mint y = mint(1);
    for (int ed_ = 1; ed_ <= N; ed_++) {
        int l = c.size(), m = b.size();
        mint x = 0;
        for (int i = 0; i < l; i++) x += c[i] * s[ed_ - l + i];
        b.emplace_back(mint(0));
        m++;
        if (x == mint(0)) continue;
        mint freq = x / y;
        if (l < m) {
            auto tmp = c;
            c.insert(begin(c), m - l, mint(0));
            for (int i = 0; i < m; i++) c[m - 1 - i] -= freq * b[m - 1 - i];
            b = tmp;
            y = x;
        } else {
            for (int i = 0; i < m; i++) c[l - 1 - i] -= freq * b[m - 1 - i];
        }
    }
    reverse(begin(c), end(c));
    c.erase(begin(c));
    for (auto &x : c) x = -x;
    return c;
}
template <typename mint>
mint nth_term(long long n, vector<mint> s) {
    vector<mint> bm = Berlekamp_Massey<mint>(s);
    return LinearRecurrence(s, bm, n);
}
