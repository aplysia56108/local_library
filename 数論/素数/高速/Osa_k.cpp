/*
O(n loglog n)でエラトステネスの篩をした後、
O(log n)で素因数分解できる。

Osa_k(n):
    コンストラクタ。
    n以下の自然数をエラトステネスの篩する。O(n log lon n)
    v[i]にiの素因数を一つ書き込む
prime_factorization(n):
    nの素因数分解をvector<pi>で返す。O(log n)
    nをv[n]で割るのを繰り返す。

divisor(n):
    約数列挙。O(約数の数)
is_prime(n):
    nが素数かどうかbool
*/

struct Osa_k {
    vector<int> v;
    vector<int> prime;

    Osa_k(int n) {
        v.resize(n + 1, -1);
        v[1] = 1;
        for (int i = 2; i <= n; i++) {
            if (v[i] != -1) continue;
            prime.push_back(i);
            for (int j = i; j <= n; j += i) v[j] = i;
        }
    }

    vector<pair<int, int>> prime_factorization(int n) {
        vector<pair<int, int>> ret;
        while (n > 1) {
            if (ret.size() == 0 || ret.back().first != v[n]) {
                ret.push_back({v[n], 1});
            } else {
                ret.back().second++;
            }
            n /= v[n];
        }
        return ret;
    }

    vector<int> divisor(int n) {
        vector<pair<int, int>> pf = prime_factorization(n);
        vector<int> res;
        res.pb(1);
        for (auto &[key, value] : pf) {
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

    bool is_prime(int n) {
        if (n <= 1) return false;
        return v[n] == n;
    }
};
