/*
Bell_Number(int n, int k, int p) :スターリング数を返す。
n個の区別できる要素を、k個の区別できない箱に入れる数（をmodで割る）。
0個もok
*/

int Bell_Number(int n, int k, int p) {
    vector<vector<long long> > S(n + 1, vector<long long>(k + 1));
    S[0][0] = 1;
    for (long long i = 1; i < n + 1; i++) {
        for (long long j = 1; j < k + 1; j++) {
            S[i][j] = (j * S[i - 1][j] + S[i - 1][j - 1]) % p;
        }
    }
    int ans = 0;
    for (int i = 1; i <= k; i++) {
        ans += S[n][i];
    }
    return ans;
}