/*
https://github.com/drken1215/algorithm/blob/master/MathNumberTheory/pollard_rho.cpp
を参考に

Pollard_rhoの素因数分解法を使用
vector<ll> divisor(ll n):約数列挙。O(n^(1/4)*log n)
*/

// Miller-Rabin 素数判定法
template <class T>
T pow_mod(T A, T N, T M) {
    T res = 1 % M;
    A %= M;
    while (N) {
        if (N & 1) res = (res * A) % M;
        A = (A * A) % M;
        N >>= 1;
    }
    return res;
}

bool is_prime(long long N) {
    if (N <= 1) return false;
    if (N == 2 || N == 3) return true;
    if (N % 2 == 0) return false;
    vector<long long> A = {2, 325, 9375, 28178, 450775, 9780504, 1795265022};
    long long s = 0, d = N - 1;
    while (d % 2 == 0) {
        ++s;
        d >>= 1;
    }
    for (auto a : A) {
        if (a % N == 0) return true;
        long long t, x = pow_mod<__int128_t>(a, d, N);
        if (x != 1) {
            for (t = 0; t < s; ++t) {
                if (x == N - 1) break;
                x = __int128_t(x) * x % N;
            }
            if (t == s) return false;
        }
    }
    return true;
}

// Pollard のロー法
long long pollard(long long N) {
    if (N % 2 == 0) return 2;
    if (is_prime(N)) return N;

    auto f = [&](long long x) -> long long {
        return (__int128_t(x) * x + 1) % N;
    };
    long long step = 0;
    while (true) {
        ++step;
        long long x = step, y = f(x);
        while (true) {
            long long p = gcd(y - x + N, N);
            if (p == 0 || p == N) break;
            if (p != 1) return p;
            x = f(x);
            y = f(f(y));
        }
    }
}

vector<long long> prime_factorize(long long N) {
    if (N == 1) return {};
    long long p = pollard(N);
    if (p == N) return {p};
    vector<long long> left = prime_factorize(p);
    vector<long long> right = prime_factorize(N / p);
    left.insert(left.end(), right.begin(), right.end());
    sort(left.begin(), left.end());
    return left;
}

unordered_map<long long, long long> prime_factor(long long n) {
    vector<long long> v = prime_factorize(n);
    unordered_map<long long, long long> res;
    for (auto c : v) {
        while (n % c == 0) {
            res[c]++;
            n /= c;
        }
    }
    return res;
}


//約数列挙O(n^(1/4)*log n)
vector<long long> divisor(long long n) {
    unordered_map<long long, long long> m = prime_factor(n);
    vector<long long> res;
    res.push_back(1);
    for (auto &[key, value] : m) {
        int k = res.size();
        for (int i = 0; i < k; i++) {
            int s = 1;
            for (int j = 1; j <= value; j++) {
                s *= key;
                res.push_back(res[i] * s);
            }
        }
    }
    return res;
}
