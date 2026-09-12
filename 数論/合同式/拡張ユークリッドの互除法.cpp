//拡張ユークリッドの互除法
// ax+by=gcd(a,b)となるような整数解(x,y)
pair<int, int> extgcd(int a, int b) {
    if (a == 0) return make_pair(0, 1);
    auto [x, y] = extgcd(b % a, a);
    return make_pair(y - (b / a) * x, x);
}