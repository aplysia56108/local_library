// エラトステネスの篩
vector<long long> prime;  // i番目の素数
vector<bool> is_prime;

// n以下の素数の数を返す。O(n log(log n))
template <typename T>
int sieve(T n) {
    is_prime.resize(n + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (T i = 2; i <= n; i++) {
        if (is_prime[i]) {
            prime.push_back(i);
            for (T j = 2 * i; j <= n; j += i) is_prime[j] = false;
        }
    }
    return prime.size();
}
