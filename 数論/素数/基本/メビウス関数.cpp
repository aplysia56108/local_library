/*
蟻本p267を参考に

μ(n)=
・nが１でない平方数で割り切れる時0
・そうでない時、素因数の個数をk個とすると(-1)^k
*/

// nの約数におけるメビウス関数の値のmapを返す。O(√n)
map<int, int> moebius(int n) {
    map<int, int> res;
    vector<int> primes;

    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            primes.push_back(i);
            while (n % i == 0) n /= i;
        }
    }
    if (n != 1) primes.push_back(n);

    int m = primes.size();
    for (int i = 0; i < (1 << m); i++) {
        int mu = 1, d = 1;
        for (int j = 0; j < m; j++) {
            if (i >> j & 1) {
                mu *= -1;
                d *= primes[j];
            }
        }
        res[d] = mu;
    }
    return res;
}

// n以下のメビウス関数の値を返す。O(n log(log n))程度
vector<int> moebius2(int n) {
    vector<int> res(n + 1, 1);
    vector<bool> prime(n + 1, true);
    prime[1] = false;
    for (int i = 2; i <= n; i++) {
        if (!prime[i]) continue;
        res[i] *= -1;
        for (int j = 2 * i; j <= n; j += i) {
            prime[j] = false;
            res[j] *= -1;
            if (j % (i * i) == 0) res[j] = 0;
        }
    }
    return res;
}
