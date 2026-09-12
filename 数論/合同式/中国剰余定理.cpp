/*
連立線形合同式(crt)
a[i]*x≡b[i] (mod m[i])
の解xはx≡b(mod m)　の形になる。

linear_congruence(a,b,m):そのペア(b,m)を返す
*/

// 拡張ユークリッドの互除法
//  ax+by=gcd(a,b)となるような整数解(x,y)
// 返り値はgcd(a,b)
template <typename T, typename U>
T extgcd(T a, T b, U& x, U& y) {
    T d = a;
    if (b != 0) {
        d = extgcd(b, a % b, y, x);
        y -= (a / b) * x;
    } else {
        x = 1;
        y = 0;
    }
    return d;
}

// 逆元を求める。ax≡1(mod m)となるx
template <typename T>
T mod_inverse(T a, T m) {
    T x, y;
    extgcd(a, m, x, y);
    return (m + x % m) % m;
}

template <typename T>
pair<T, T> linear_congruence(const vector<T>& A, const vector<T>& B,
                             const vector<T>& M) {
    // 最初は条件がないので全ての整数を意味するx≡0(mod 1)を解としておく。
    T x = 0, m = 1;

    for (int i = 0; i < (int)A.size(); i++) {
        T a = A[i] * m, b = B[i] - A[i] * x, d = gcd(M[i], a);
        if (b % d != 0) return make_pair(0, -1);  // 解なし
        T t = b / d * mod_inverse(a / d, M[i] / d) % (M[i] / d);
        x = x + m * t;
        m *= M[i] / d;
    }
    x = (x % m + m) % m;
    return make_pair(x, m);
}
