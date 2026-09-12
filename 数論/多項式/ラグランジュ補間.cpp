/*
https://atcoder.jp/contests/abc208/submissions/23996263
を参考に

係数がわからないk次多項式fについて、
p={f(0),f(1),f(2)...f(k)}が分かれば,
係数が定まる。
そこからf(n)を求める

Interpolation(vector<mint> p):コンストラクタ。p={f(0),f(1)...}
operator(mint x):f(x)の値を返す。
*/
struct Interpolation {
    int n;
    vector<mint> a;
    Interpolation() {
    }
    Interpolation(const vector<mint> &p) {
        n = p.size();
        a.resize(n);
        vector<mint> invf(n, 1);
        mint f = 1;
        for (int i = 1; i < n; i++) f *= i;
        invf[n - 1] = mint(1) / f;
        for (int i = n - 1; i >= 1; i--) invf[i - 1] = invf[i] * i;
        for (int i = 0; i < n; i++) {
            mint b = invf[i] * invf[n - 1 - i];
            a[i] = p[i] * ((n - 1 - i) % 2 == 1 ? -b : b);
        }
    }
    mint operator()(mint x) const {
        vector<mint> r(n, 1);
        for (int i = n - 1; i >= 1; --i) r[i - 1] = r[i] * (x - i);
        mint l = 1, res;
        for (int i = 0; i < n; i++) {
            res += a[i] * l * r[i];
            l *= x - i;
        }
        return res;
    }
};
