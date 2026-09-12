/*
https://drken1215.hatenablog.com/entry/2018/06/08/210000 を参考に

前提:pが大きくMAX_Nがそこまで大きくない素数&p>n

Binomial(MOD,MAX_N=1):コンストラクタ。O(MAX_N)
mod_comb(n,k),(n,k):nCk(mod p)。O(1)
mod_perm(n,k):nPk(mod p)。O(1)

コンストラクタの引数でMAX_Nを与えなくても、mod_comb等の関数で必要になったら、自動計算する。
だが、あらかじめ与えておいた方がやや早い。
*/

struct Binomial {
    int p;
    int MAX;
    vector<long long> fac, finv, inv;

    // テーブルを作る前処理
    Binomial(int p_, int n = 1) : p(p_), MAX(1), fac(2), finv(2), inv(2) {
        fac[0] = fac[1] = 1;
        finv[0] = finv[1] = 1;
        inv[1] = 1;
        if (n != 1) build(n);
    }

    void build(int new_max) {
        MAX++;
        fac.resize(new_max + 1);
        inv.resize(new_max + 1);
        finv.resize(new_max + 1);
        for (; MAX <= new_max; MAX++) {
            fac[MAX] = fac[MAX - 1] * MAX % p;
            inv[MAX] = p - inv[p % MAX] * (p / MAX) % p;
            finv[MAX] = finv[MAX - 1] * inv[MAX] % p;
        }
        MAX--;
    }

    // nCk
    long long mod_comb(int n, int k) {
        if (n < k) return 0;
        if (n < 0 || k < 0) return 0;
        if (n > MAX) build(n);
        return fac[n] * (finv[k] * finv[n - k] % p) % p;
    }
    long long operator()(int n, int k) {
        return mod_comb(n, k);
    }

    // nPk
    long long mod_perm(int n, int k) {
        if (n < k) return 0;
        if (n < 0 || k < 0) return 0;
        if (n > MAX) build(n);
        return fac[n] * finv[n - k] % p;
    }
} C(mod);
