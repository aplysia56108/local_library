/*
パスカルの三角形を使用
nが小さい時。MODが素数でなくても使える。

Binomial(n,p=-1):コンストラクタ。pに入れなければ、MODで割らない。
mod_comb(n,k):nCkmodpを返す
*/

struct Binomial {
    long long p;
    long long n;
    vector<vector<long long>> Com;

    Binomial(int MAX_N, int p = -1)
        : p(p), n(MAX_N + 1), Com(MAX_N + 1, vector<long long>(MAX_N + 1)) {
        Com[0][0] = 1;
        for (int i = 1; i < n; i++) {
            Com[i][0] = 1;
            for (int j = 1; j < n; j++) {
                if (p != -1) {
                    Com[i][j] = (Com[i - 1][j - 1] + Com[i - 1][j]) % p;
                } else {
                    Com[i][j] = (Com[i - 1][j - 1] + Com[i - 1][j]);
                }
            }
        }
    }

    long long mod_comb(int m, int k) {
        return Com[m][k];
    }

    long long operator()(int n, int k) {
        return mod_comb(n, k);
    }
};
