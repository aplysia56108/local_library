/*
euler_phi(n):オイラー関数の値を返す
euler_phi2(n):オイラー関数の値のテーブルをvectorで返す

オイラー関数：φ(m)=m＊Π(p_i-1)/p_i
(m=p_1^e_1*p_2^e_2・・・p_n^e_n)
これはm以下のmと互いに素な自然数の個数と等しい

mと互いに素なxについて
x^φ(m)≡1(mod m)が成り立つ。
（mが素数の時、フェルマーの小定理）
*/

//オイラー関数の値を求めるO(√n)
template <typename T>
T euler_phi(T n) {
    T res = n;
    for (T i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            res = res / i * (i - 1);
            for (; n % i == 0; n /= i)
                ;
        }
    }
    if (n != 1) res = res / n * (n - 1);
    return res;
}

//オイラー関数の値のテーブルを作成　O(n)程度
template <typename T>
vector<T> euler_phi2(T n) {
    vector<T> euler(n + 1);
    for (long long i = 0; i <= n; i++) euler[i] = i;
    for (long long i = 2; i <= n; i++) {
        if (euler[i] == i) {
            for (long long j = i; j <= n; j += i)
                euler[j] = euler[j] / i * (i - 1);
        }
    }
    return euler;
}