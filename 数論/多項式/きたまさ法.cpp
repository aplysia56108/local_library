/*
https://outline.hatenadiary.jp/entry/2020/07/02/205628
を参考に

別名：Feduccia’s algorithm
隣接k項間漸化式の一般公a_nの値を高速に求める方法
行列累乗を用いるとO(k^3 log(n))でもとまる。
きたまさ法を用いれば、O(k^2 log (n))で求めることが可能
(Modが998244353等NTT-friendlyの場合、Bostan_Mori法を用いてO(k*log(k)*log(n))で求めることが可能)

a={a_0,a_1,...,a_(k-1)}
d={d_0,d_1,...,d_(k-1)}

Kitamasa<T> kt(a,d):コンストラクタ
calc(n):a_nを返す。
*/

template <typename T>
struct Kitamasa {
    vector<T> a;  // 初期値ベクトル
    vector<T> d;  // 係数ベクトル
    int k;

    Kitamasa(vector<T> &a, vector<T> &d) : a(a), d(d), k((int)a.size()) {
    }

    // a_n の係数ベクトルを求める
    vector<T> dfs(int64_t n) {
        if (n == k) return d;

        vector<T> res(k);
        if (n & 1 || n < k * 2) {
            vector<T> x = dfs(n - 1);
            for (int i = 0; i < k; ++i) res[i] = d[i] * x[k - 1];
            for (int i = 0; i + 1 < k; ++i) res[i + 1] += x[i];
        } else {
            vector<vector<T>> x(k, vector<T>(k));
            x[0] = dfs(n >> 1);
            for (int i = 0; i + 1 < k; ++i) {
                for (int j = 0; j < k; ++j) x[i + 1][j] = d[j] * x[i][k - 1];
                for (int j = 0; j + 1 < k; ++j) x[i + 1][j + 1] += x[i][j];
            }
            for (int i = 0; i < k; ++i) {
                for (int j = 0; j < k; ++j) {
                    res[j] += x[0][i] * x[i][j];
                }
            }
        }

        return res;
    }

    // a_n を求める
    T calc(int64_t n) {
        if (n < (int)a.size()) return a[n];
        vector<T> x = dfs(n);
        T res = 0;
        for (int i = 0; i < k; ++i) res += x[i] * a[i];
        return res;
    }
};
