/*
https:opt-cp.com/fps-implementation/
https:judge.yosupo.jp/submission/42413
を参考に

以下、d:サイズdにresizeする
+,-,*,/,>>,<<
inv(d):逆数
multiply(g,d):gをconvolution
divide(g,d):gで割る
eval(a):f(a)
log(d):log(f)
exp(d):exp(f)
pow(k,d):f^k

MOD = 998244353（NTT-friendry）の時はfastが使える。
MOD =1000000007などの他の時は使えないので、
define fastをコメントアウトして、その下のnaiveを使う。
（*fpsや/fpsに関わる部分。）
*/

#define fast

#define rep_(i, a_, b_, a, b, ...) \
    for (int i = (a), lim##i = (b); i < lim##i; ++i)
#define rep2_(i, ...) rep_(i, __VA_ARGS__, __VA_ARGS__, 0, __VA_ARGS__)
#define drep_(i, a_, b_, a, b, ...) \
    for (int i = (a)-1, lim##i = (b); i >= lim##i; --i)
#define drep(i, ...) drep_(i, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__, 0)

template <typename T>
struct Factorial {
    int MAX;
    vector<T> fac, finv;
    Factorial(int m = 0) : MAX(m), fac(m + 1, 1), finv(m + 1, 1) {
        rep2_(i, 2, MAX + 1) fac[i] = fac[i - 1] * i;
        finv[MAX] /= fac[MAX];
        drep(i, MAX + 1, 3) finv[i - 1] = finv[i] * i;
    }
    T binom(int n, int k) {
        if (k < 0 || n < k) return 0;
        return fac[n] * finv[k] * finv[n - k];
    }
    T perm(int n, int k) {
        if (k < 0 || n < k) return 0;
        return fac[n] * finv[n - k];
    }
};
Factorial<mint> fc;

template <class T>
struct FormalPowerSeries : vector<T> {
    using vector<T>::vector;
    using vector<T>::operator=;
    using F = FormalPowerSeries;

    F operator-() const {
        F res(*this);
        for (auto &e : res) e = -e;
        return res;
    }
    F &operator*=(const T &g) {
        for (auto &e : *this) e *= g;
        return *this;
    }
    F &operator/=(const T &g) {
        assert(g != T(0));
        *this *= g.inv();
        return *this;
    }
    F &operator+=(const F &g) {
        int n = this->size(), m = g.size();
        rep2_(i, min(n, m))(*this)[i] += g[i];
        return *this;
    }
    F &operator-=(const F &g) {
        int n = this->size(), m = g.size();
        rep2_(i, min(n, m))(*this)[i] -= g[i];
        return *this;
    }
    F &operator<<=(const int d) {
        int n = this->size();
        if (d >= n) *this = F(n);
        this->insert(this->begin(), d, 0);
        this->resize(n);
        return *this;
    }
    F &operator>>=(const int d) {
        int n = this->size();
        this->erase(this->begin(), this->begin() + min(n, d));
        this->resize(n);
        return *this;
    }

    // O(n log n)
    F inv(int d = -1) const {
        int n = this->size();
        assert(n != 0 && (*this)[0] != 0);
        if (d == -1) d = n;
        assert(d >= 0);
        F res{(*this)[0].inv()};
        for (int m = 1; m < d; m *= 2) {
            F f(this->begin(), this->begin() + min(n, 2 * m));
            F g(res);
            f.resize(2 * m), internal::butterfly(f);
            g.resize(2 * m), internal::butterfly(g);
            rep2_(i, 2 * m) f[i] *= g[i];
            internal::butterfly_inv(f);
            f.erase(f.begin(), f.begin() + m);
            f.resize(2 * m), internal::butterfly(f);
            rep2_(i, 2 * m) f[i] *= g[i];
            internal::butterfly_inv(f);
            T iz = T(2 * m).inv();
            iz *= -iz;
            rep2_(i, m) f[i] *= iz;
            res.insert(res.end(), f.begin(), f.begin() + m);
        }
        res.resize(d);
        return res;
    }

#ifdef fast
    // fast: FMT-friendly modulus only
    // O(n log n)
    F &multiply_inplace(const F &g, int d = -1) {
        int n = this->size();
        if (d == -1) d = n;
        assert(d >= 0);
        *this = convolution(move(*this), g);
        this->resize(d);
        return *this;
    }
    F multiply(const F &g, const int d = -1) const {
        return F(*this).multiply_inplace(g, d);
    }
    F &operator*=(const F &g) {
        int n = this->size();
        *this = convolution(move(*this), g);
        this->resize(n);
        return *this;
    }
    // O(n log n)
    F &divide_inplace(const F &g, int d = -1) {
        int n = this->size();
        if (d == -1) d = n;
        assert(d >= 0);
        *this = convolution(move(*this), g.inv(d));
        this->resize(d);
        return *this;
    }
    F divide(const F &g, const int d = -1) const {
        return F(*this).divide_inplace(g, d);
    }
    F &operator/=(const F &g) {
        int n = (*this).size();
        *this = convolution(*this, g.inv(n));
        (*this).resize(n);
        return *this;
    }
#else
    // naive
    // // O(n^2)
    F &multiply_inplace(const F &g) {
        int n = this->size(), m = g.size();
        drep(i, n) {
            (*this)[i] *= g[0];
            rep2_(j, 1, min(i + 1, m))(*this)[i] += (*this)[i - j] * g[j];
        }
        return *this;
    }
    F multiply(const F &g) const {
        return F(*this).multiply_inplace(g);
    }
    // O(n^2)
    F &divide_inplace(const F &g) {
        assert(g[0] != T(0));
        T ig0 = g[0].inv();
        int n = this->size(), m = g.size();
        rep(i, n) {
            rep2_(j, 1, min(i + 1, m))(*this)[i] -= (*this)[i - j] * g[j];
            (*this)[i] *= ig0;
        }
        return *this;
    }
    F divide(const F &g) const {
        return F(*this).divide_inplace(g);
    }
    F &operator*=(const F &g) {
        int n = (*this).size(), m = g.size();
        drep(i, n) {
            (*this)[i] *= g[0];
            rep2_(j, 1, min(i + 1, m))(*this)[i] += (*this)[i - j] * g[j];
        }
        return *this;
    }
    F &operator/=(const F &g) {
        assert(g[0] != T(0));
        T ig0 = g[0].inv();
        int n = (*this).size(), m = g.size();
        rep(i, n) {
            rep2_(j, 1, min(i + 1, m))(*this)[i] -= (*this)[i - j] * g[j];
            (*this)[i] *= ig0;
        }
        return *this;
    }
#endif

    // sparse
    // O(nk)
    F &multiply_inplace(vector<pair<int, T>> g) {
        int n = this->size();
        auto [d, c] = g.front();
        if (d == 0)
            g.erase(g.begin());
        else
            c = 0;
        drep(i, n) {
            (*this)[i] *= c;
            for (auto &[j, b] : g) {
                if (j > i) break;
                (*this)[i] += (*this)[i - j] * b;
            }
        }
        return *this;
    }
    F multiply(const vector<pair<int, T>> &g) const {
        return F(*this).multiply_inplace(g);
    }
    F &operator*=(vector<pair<int, T>> g) {
        int n = (*this).size();
        auto [d, c] = g.front();
        if (d == 0)
            g.erase(g.begin());
        else
            c = 0;
        drep(i, n) {
            (*this)[i] *= c;
            for (auto &[j, b] : g) {
                if (j > i) break;
                (*this)[i] += (*this)[i - j] * b;
            }
        }
        return *this;
    }

    // O(nk)
    F &divide_inplace(vector<pair<int, T>> g) {
        int n = this->size();
        auto [d, c] = g.front();
        assert(d == 0 && c != T(0));
        T ic = c.inv();
        g.erase(g.begin());
        rep2_(i, n) {
            for (auto &[j, b] : g) {
                if (j > i) break;
                (*this)[i] -= (*this)[i - j] * b;
            }
            (*this)[i] *= ic;
        }
        return *this;
    }
    F divide(const vector<pair<int, T>> &g) const {
        return F(*this).divide_inplace(g);
    }
    F &operator/=(vector<pair<int, T>> g) {
        int n = (*this).size();
        auto [d, c] = g.front();
        assert(d == 0 && c != T(0));
        T ic = c.inv();
        g.erase(g.begin());
        rep2_(i, n) {
            for (auto &[j, b] : g) {
                if (j > i) break;
                (*this)[i] -= (*this)[i - j] * b;
            }
            (*this)[i] *= ic;
        }
        return *this;
    }

    // multiply and divide (1 + cz^d)
    // O(n)
    void multiply_inplace(const int d, const T c) {
        int n = this->size();
        if (c == T(1))
            drep(i, n - d)(*this)[i + d] += (*this)[i];
        else if (c == T(-1))
            drep(i, n - d)(*this)[i + d] -= (*this)[i];
        else
            drep(i, n - d)(*this)[i + d] += (*this)[i] * c;
    }
    // O(n)
    void divide_inplace(const int d, const T c) {
        int n = this->size();
        if (c == T(1))
            rep2_(i, n - d)(*this)[i + d] -= (*this)[i];
        else if (c == T(-1))
            rep2_(i, n - d)(*this)[i + d] += (*this)[i];
        else
            rep2_(i, n - d)(*this)[i + d] -= (*this)[i] * c;
    }

    // O(n)
    T eval(const T &a) const {
        T x(1), res(0);
        for (auto e : *this) res += e * x, x *= a;
        return res;
    }

    // O(n)
    F &integral_inplace() {
        int n = this->size();
        assert(n > 0);
        if (n == 1) return *this = F{0};
        this->insert(this->begin(), 0);
        this->pop_back();
        vector<T> inv(n);
        inv[1] = 1;
        int p = T::mod();
        rep2_(i, 2, n) inv[i] = -inv[p % i] * (p / i);
        rep2_(i, 2, n)(*this)[i] *= inv[i];
        return *this;
    }
    F integral() const {
        return F(*this).integral_inplace();
    }

    // O(n)
    F &derivative_inplace() {
        int n = this->size();
        assert(n > 0);
        rep2_(i, 2, n)(*this)[i] *= i;
        this->erase(this->begin());
        this->push_back(0);
        return *this;
    }
    F derivative() const {
        return F(*this).derivative_inplace();
    }

    // O(n log n)
    F &log_inplace(int d = -1) {
        int n = this->size();
        assert(n > 0 && (*this)[0] == 1);
        if (d == -1) d = n;
        assert(d >= 0);
        if (d < n) this->resize(d);
        F f_inv = this->inv();
        this->derivative_inplace();
        this->multiply_inplace(f_inv);
        this->integral_inplace();
        return *this;
    }
    F log(const int d = -1) const {
        return F(*this).log_inplace(d);
    }

    // O(n log n)
    // https://arxiv.org/abs/1301.5804 (Figure 1, right)
    F &exp_inplace(int d = -1) {
        int n = this->size();
        assert(n > 0 && (*this)[0] == 0);
        if (d == -1) d = n;
        assert(d >= 0);
        F g{1}, g_fft{1, 1};
        (*this)[0] = 1;
        this->resize(d);
        F h_drv(this->derivative());
        for (int m = 2; m < d; m *= 2) {
            // prepare
            F f_fft(this->begin(), this->begin() + m);
            f_fft.resize(2 * m), internal::butterfly(f_fft);

            // Step 2.a'
            {
                F _g(m);
                rep2_(i, m) _g[i] = f_fft[i] * g_fft[i];
                internal::butterfly_inv(_g);
                _g.erase(_g.begin(), _g.begin() + m / 2);
                _g.resize(m), internal::butterfly(_g);
                rep2_(i, m) _g[i] *= g_fft[i];
                internal::butterfly_inv(_g);
                _g.resize(m / 2);
                _g /= T(-m) * m;
                g.insert(g.end(), _g.begin(), _g.begin() + m / 2);
            }

            // Step 2.b'--d'
            F t(this->begin(), this->begin() + m);
            t.derivative_inplace();
            {
                // Step 2.b'
                F r{h_drv.begin(), h_drv.begin() + m - 1};
                // Step 2.c'
                r.resize(m);
                internal::butterfly(r);
                rep2_(i, m) r[i] *= f_fft[i];
                internal::butterfly_inv(r);
                r /= -m;
                // Step 2.d'
                t += r;
                t.insert(t.begin(), t.back());
                t.pop_back();
            }

            // Step 2.e'
            if (2 * m < d) {
                t.resize(2 * m);
                internal::butterfly(t);
                g_fft = g;
                g_fft.resize(2 * m);
                internal::butterfly(g_fft);
                rep2_(i, 2 * m) t[i] *= g_fft[i];
                internal::butterfly_inv(t);
                t.resize(m);
                t /= 2 * m;
            } else {  // この場合分けをしても数パーセントしか速くならない
                F g1(g.begin() + m / 2, g.end());
                F s1(t.begin() + m / 2, t.end());
                t.resize(m / 2);
                g1.resize(m), internal::butterfly(g1);
                t.resize(m), internal::butterfly(t);
                s1.resize(m), internal::butterfly(s1);
                rep2_(i, m) s1[i] = g_fft[i] * s1[i] + g1[i] * t[i];
                rep2_(i, m) t[i] *= g_fft[i];
                internal::butterfly_inv(t);
                internal::butterfly_inv(s1);
                rep2_(i, m / 2) t[i + m / 2] += s1[i];
                t /= m;
            }

            // Step 2.f'
            F v(this->begin() + m, this->begin() + min<int>(d, 2 * m));
            v.resize(m);
            t.insert(t.begin(), m - 1, 0);
            t.push_back(0);
            t.integral_inplace();
            rep2_(i, m) v[i] -= t[m + i];

            // Step 2.g'
            v.resize(2 * m);
            internal::butterfly(v);
            rep2_(i, 2 * m) v[i] *= f_fft[i];
            internal::butterfly_inv(v);
            v.resize(m);
            v /= 2 * m;

            // Step 2.h'
            rep2_(i, min(d - m, m))(*this)[m + i] = v[i];
        }
        return *this;
    }
    F exp(const int d = -1) const {
        return F(*this).exp_inplace(d);
    }

    // O(n log n)
    F &pow_inplace(const ll k, int d = -1) {
        int n = this->size();
        if (d == -1) d = n;
        assert(d >= 0 && k >= 0);
        if (d == 0) return *this = F(0);
        if (k == 0) {
            *this = F(d);
            (*this)[0] = 1;
            return *this;
        }
        int l = 0;
        while (l < n && (*this)[l] == 0) ++l;
        if (l == n || l > (d - 1) / k) return *this = F(d);
        T c{(*this)[l]};
        this->erase(this->begin(), this->begin() + l);
        *this /= c;
        this->log_inplace(d - l * k);
        *this *= k;
        this->exp_inplace();
        *this *= c.pow(k);
        this->insert(this->begin(), l * k, 0);
        return *this;
    }
    F pow(const ll k, const int d = -1) const {
        return F(*this).pow_inplace(k, d);
    }

    // O(n log n)
    F &shift_inplace(const T c) {
        int n = this->size();
        fc = Factorial<T>(n);
        rep2_(i, n)(*this)[i] *= fc.fac[i];
        reverse(this->begin(), this->end());
        F g(n);
        T cp = 1;
        rep2_(i, n) g[i] = cp * fc.finv[i], cp *= c;
        this->multiply_inplace(g, n);
        reverse(this->begin(), this->end());
        rep2_(i, n)(*this)[i] *= fc.finv[i];
        return *this;
    }
    F shift(const T c) const {
        return F(*this).shift_inplace(c);
    }

    F operator*(const T &g) const {
        return F(*this) *= g;
    }
    F operator/(const T &g) const {
        return F(*this) /= g;
    }
    F operator+(const F &g) const {
        return F(*this) += g;
    }
    F operator-(const F &g) const {
        return F(*this) -= g;
    }
    F operator<<(const int d) const {
        return F(*this) <<= d;
    }
    F operator>>(const int d) const {
        return F(*this) >>= d;
    }
    F operator*(const F &g) const {
        return F(*this) *= g;
    }
    F operator/(const F &g) const {
        return F(*this) /= g;
    }
    F operator*(vector<pair<int, T>> g) const {
        return F(*this) *= g;
    }
    F operator/(vector<pair<int, T>> g) const {
        return F(*this) /= g;
    }
};

using fps = FormalPowerSeries<mint>;
using sfps = vector<pair<int, mint>>;
