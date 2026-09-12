/*
https://ei1333.github.io/luzhiled/snippets/graph/chromatic-number.html
を参考に

彩色数（隣接した辺の色が異なるように塗るのに必要な最小の色の数)を求める.O(2^n n)
隣接行列を渡す
*/
int chromatic_number(vector<vector<bool>> g) {
    int N = (int)g.size();
    vector<int> es(N);
    for (int i = 0; i < g.size(); i++) {
        for (int j = 0; j < g.size(); j++) {
            es[i] |= g[i][j] << j;
        }
    }
    int ret = N;
    for (int d : {7, 11, 21}) {
        int mod = 1e9 + d;
        vector<int> ind(1 << N), aux(1 << N, 1);
        ind[0] = 1;
        for (int S = 1; S < 1 << N; S++) {
            int u = __builtin_ctz(S);
            ind[S] = ind[S ^ (1 << u)] + ind[(S ^ (1 << u)) & ~es[u]];
        }
        for (int i = 1; i < ret; i++) {
            int64_t all = 0;
            for (int j = 0; j < 1 << N; j++) {
                int S = j ^ (j >> 1);
                aux[S] = (1LL * aux[S] * ind[S]) % mod;
                all += j & 1 ? aux[S] : mod - aux[S];
            }
            if (all % mod) ret = i;
        }
    }
    return ret;
}
