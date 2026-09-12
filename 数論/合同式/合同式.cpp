/*
ax≡b (mod　m)となる最小のxを返す。(存在しない場合は-1)
Atcoder_Libraryのinv_modを使用
*/

long long modular(long long a, long long b, long long m) {
    if (b == 0) return 0;
    long long g = gcd(a, m);
    if (b % g != 0) return -1;
    a /= g;
    b /= g;
    m /= g;
    long long res = (__int128_t)inv_mod(a, m) * b % m;
    return res;
}
