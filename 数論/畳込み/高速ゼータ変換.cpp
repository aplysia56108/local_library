/*
https://qiita.com/Euglenese/items/260f9ddf513f772d7e42
を参考に

下位集合版
g(S)=∑T⊆Sf(T)
{1, 10, 100, 1000}→{1, 11, 101, 1111}

整数 Nと A0,A1,A2,...A2^N−1が与えられる。
0≦i<2^Nを満たす整数 iについて、
j⊆iを満たす整数 jについてのAjの総和をそれぞれ求める。
（s,tに対して s AND t=tの時、t⊂sと表記する。）
O(N*2^N)

上位集合版
g(S)=∑T⊇Sf(T)
{1, 10, 100, 1000}→{1111, 1010, 1100, 1000 }
*/
template <typename T>
vector<T> fzt(vector<T> &a) {
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
                dp[i] += dp[i ^ (1 << j)];
            }
            //上位集合
            // if (!(i & (1 << j))) {
            //     dp[i] += dp[i ^ (1 << j)];
            // }
        }
    }
    dp.resize(a.size());
    return dp;
}