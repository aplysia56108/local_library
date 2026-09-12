/*
https://atcoder.jp/contests/wtf19-open/submissions/4380752
を参考に
nCr mod2
リュカの定理より
*/
int nCrbit(long long n, long long r) {
    if (n < 0 || r < 0) return 0;
    if (n < r) return 0;
    if ((n | r) != n) return 0;
    return 1;
}
