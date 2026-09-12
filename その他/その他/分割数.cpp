/*
分割数（Division number）

Division_Number(n,m,p):（n個の互いに区別できない品物を、m個以下に分割する方法の総数。（pで割った余り))
dn[m][n]:n個の互いに区別できない品物を、m個以下に分割する方法の総数
4個の品物を3個以下に分割する方法:dn[3][4]=4(1+1+2=1+3=2+2=4)

nのm分割a_i（Σa_i=n）を考える。
すべてのiでa_i>0ならば{a_i-1}はn-mのm分割となる。
またa_i=0となるiが存在したら、これはnのm-1分割となる。
よって、dp[i][j]=dp[i-1][j]+dp[i][j-i]

2025/05/04 nの分割を列挙する関数を追加
*/

int Division_Number(int n, int m, int p) {
    vector<vector<int> > dn(m + 1, vector<int>(n + 1));
    dn[0][0] = 1;
    for (int i = 1; i <= m; i++) {
        for (int j = 0; j <= n; j++) {
            if (j - i >= 0) {
                dn[i][j] = (dn[i - 1][j] + dn[i][j - i]) % p;
            } else {
                dn[i][j] = dn[i - 1][j];
            }
        }
    }
    return dn[m][n];
}

// https://shigotoron.com/%E6%95%B4%E6%95%B0%E3%81%AE%E5%88%86%E5%89%B2%E3%82%92%E5%88%97%E6%8C%99-python/
// を参考に。
// nの分割を全て求める。(分割の個数は指定しない)
// nの分割数をf(n)として、
// O(n^2 f(n))には抑えられている。(実際はもっと軽いと考えられる)
// atcoderのコードテスト環境で、n = 60で357ms(2025/05/04に計測)
// verify: https://atcoder.jp/contests/abc404/submissions/65496914
vector<vector<int>> all_divisons(int n) {
    vvi res;
    auto f = [&](auto f, int remain, int max_summand, vi summands) {
        if(remain == 0) {
            res.push_back(summands);
            return;
        }

        int i = max_summand;
        while(i) {
            if(remain >= i) {
                summands.push_back(i);
                f(f, remain - i, i, summands);
                summands.pop_back();
            }
            i = i - 1;
        }
    };

    f(f, n, n, vi());
    return res;
}
