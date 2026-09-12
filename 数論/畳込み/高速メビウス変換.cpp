/*
https://misteer.hatenablog.com/entry/zeta-moebius
を参考に

ゼータ変換の逆

下位集合版
f(S)=∑T⊆S(−1)^(|S|−|T|)g(T)
{1, 10, 100, 1000}→{1, 9, 99, 891}

上位集合版
{1, 10, 100, 1000}→{891, -990, -900, 1000}
*/
template <typename T>
vector<T> fmt(vector<T> &a) {
    vector<T> dp = a;
    int n_ = a.size() - 1;
    int N = 0;
    int s = 1;
    while (n_ > 0) {
        n_ >>= 1;
        N++;
        s <<= 1;
    }
    dp.resize(s);
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < (1 << N); i++) {
            //下位集合
            if (i & (1 << j)) {
                dp[i] -= dp[i ^ (1 << j)];
            }
            //上位集合
            // if (!(i & (1 << j))) {
            //     dp[i] -= dp[i ^ (1 << j)];
            // }
        }
    }
    // dp.resize(a.size());
    return dp;
}