/*
前提：pがそこまで大きくない素数

Binomial:インスタンス生成。O(p)
mod_comb(n, k):nCk(mod p)。O(log_p(n))
mod_perm(n,k):nPk(mod p)。O(log_p(n))
*/

struct Binomial {
    vector<int> fact;
    int p;

    Binomial(int p_) : fact(p_), p(p_) {
        fact[0] = 1;
        for (int i = 1; i < p_; i++) {
            fact[i] =
                (int)(((long long)fact[i - 1] * (long long)i) % (long long)p_);
        }
    }

    // n!=a p^e とした時のa mod pを求める。O(log_p n)。ただしpは素数
    int mod_fact(int n, int &e) {
        e = 0;
        if (n == 0) return 1;

        // pの倍数の部分を計算
        int res = mod_fact(n / p, e);
        e += n / p;

        //(p-1)!≡-1なので(p-1)!^(n/p)はn/pの偶奇だけで決まる
        if (n / p % 2 != 0)
            return (int)((long long)res * (long long)((p - fact[n % p])) %
                         (long long)p);
        return (int)((long long)res * (long long)fact[n % p] % (long long)p);
    }

    //拡張ユークリッドの互除法
    // ax+by=gcd(a,b)となるような整数解(x,y)
    //返り値はgcd(a,b)
    int extgcd(int a, int b, int &x, int &y) {
        int d = a;
        if (b != 0) {
            d = extgcd(b, a % b, y, x);
            y -= (a / b) * x;
        } else {
            x = 1;
            y = 0;
        }
        return d;
    }

    //逆元を求める。ax≡1(mod m)となるx
    int mod_inverse(int a, int m) {
        int x, y;
        extgcd(a, m, x, y);
        return (m + x % m) % m;
    }

    // nCk mod pを求める。 O(log_p(n))
    int mod_comb(int n, int k) {
        if (n < 0 || k < 0 || n < k) return 0;
        int e1, e2, e3;
        int a1 = mod_fact(n, e1), a2 = mod_fact(k, e2),
            a3 = mod_fact(n - k, e3);
        if (e1 > e2 + e3) return 0;
        return (int)((long long)a1 *
                     mod_inverse((int)((long long)a2 * (long long)a3 % p), p) %
                     p);
    }
    long long operator()(int n, int k) {
        return mod_comb(n, k);
    }

    // nPk mod pを求める
    int mod_perm(int n, int k) {
        if (n < 0 || k < 0 || n < k) return 0;
        int e1, e3;
        int a1 = mod_fact(n, e1), a3 = mod_fact(n - k, e3);
        if (e1 > e3) return 0;
        return (int)((long long)a1 * mod_inverse(a3, p) % p);
    }
};
