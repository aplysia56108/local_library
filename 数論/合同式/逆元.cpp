/*
mod_invese(a,m):ax≡1(mod m)となるxを返す
*/

//拡張ユークリッドの互除法
// ax+by=gcd(a,b)となるような整数解(x,y)
//返り値はgcd(a,b)
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

//逆元を求める。ax≡1(mod m)となるx
template <typename T>
T mod_inverse(T a, T m) {
    T x, y;
    extgcd(a, m, x, y);
    return (m + x % m) % m;
}