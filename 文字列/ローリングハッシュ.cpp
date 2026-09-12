/*
https://github.com/drken1215/algorithm/blob/master/String/rolling_hash.cpp
を参考に

RollingHash(string S):コンストラクタ
get(l,r):[l,r)のハッシュ値
contain(RollingHash T):他の文字列(RollingHash T)を含むか
*/

// ローリングハッシュ
struct RollingHash {
    int n;
    static const int base1 = 1007, base2 = 2009;
    static const int mod1 = 1000000007, mod2 = 1000000009;
    vector<long long> hash1, hash2, power1, power2;

    // construct
    RollingHash(const string &S) {
        n = (int)S.size();
        hash1.assign(n + 1, 0), hash2.assign(n + 1, 0);
        power1.assign(n + 1, 1), power2.assign(n + 1, 1);
        for (int i = 0; i < n; ++i) {
            hash1[i + 1] = (hash1[i] * base1 + S[i]) % mod1;
            hash2[i + 1] = (hash2[i] * base2 + S[i]) % mod2;
            power1[i + 1] = (power1[i] * base1) % mod1;
            power2[i + 1] = (power2[i] * base2) % mod2;
        }
    }

    // get hash value of S[left:right]
    inline long long get(int l, int r) const {
        long long res1 = hash1[r] - hash1[l] * power1[r - l] % mod1;
        if (res1 < 0) res1 += mod1;
        long long res2 = hash2[r] - hash2[l] * power2[r - l] % mod2;
        if (res2 < 0) res2 += mod2;
        return res1 * mod2 + res2;
    }

    // get hash value of S
    inline long long get() const {
        return hash1.back() * mod2 + hash2.back();
    }

    //この文字列が他の文字列Tを含む
    bool contain(const RollingHash &T) {
        long long t = T.get();
        int s = T.n;
        for (int i = 0; i + s <= n; i++) {
            if (RollingHash(*this).get(i, i + s) == t) return true;
        }
        return false;
    }

    // get lcp of S[a:] and S[b:]
    inline int getLCP(int a, int b) const {
        int len = min((int)hash1.size() - a, (int)hash1.size() - b);
        int low = 0, high = len;
        while (high - low > 1) {
            int mid = (low + high) >> 1;
            if (get(a, a + mid) != get(b, b + mid))
                high = mid;
            else
                low = mid;
        }
        return low;
    }

    // get lcp of S[a:] and T[b:]
    inline int getLCP(const RollingHash &T, int a, int b) const {
        int len = min((int)hash1.size() - a, (int)hash1.size() - b);
        int low = 0, high = len;
        while (high - low > 1) {
            int mid = (low + high) >> 1;
            if (get(a, a + mid) != T.get(b, b + mid))
                high = mid;
            else
                low = mid;
        }
        return low;
    }
};