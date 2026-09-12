/*
https://drken1215.hatenablog.com/entry/2021/08/01/025200
https://kazuma8128.hatenablog.com/entry/2018/05/31/144519
を参考に

解説動画↓
https://www.youtube.com/watch?v=9rnTrRxYgb0

高速アダマール変換（ウォルシュ変換):
長さ2^nの（vector）a,bに対し、
c[k]=Σ(i xor j==k) a[i]*b[j]
をO(n log n)で計算できる。

xorだけでなく、ANDやORでも可能
fwt<type,T>(vec a,vec b):アダマール変換されたものが返る。
*/

// type: xor->0 ,AND->1 ,OR->2
template <int type, class T>
vector<T> fwt(const vector<T> &a, const vector<T> &b) {
    int N = a.size();
    auto trans = [&](vector<T> &f, bool inv = false) {
        for (int i = 1; i < N; i <<= 1) {
            for (int j = 0; j < N; ++j) {
                if ((j & i) == 0) {
                    if (type == 0) {
                        // xor畳込み
                        if (!inv) {
                            if ((j & i) == 0) {
                                T x = f[j], y = f[j | i];
                                f[j] = x + y, f[j | i] = x - y;
                            }
                        } else {
                            if ((j & i) == 0) {
                                T x = f[j], y = f[j | i];
                                f[j] = (x + y) / 2, f[j | i] = (x - y) / 2;
                            }
                        }
                    }
                    elif (type == 1) {
                        // AND畳込み
                        if (!inv) {
                            if ((j & i) == 0) {
                                f[j] += f[j | i];
                            }
                        } else {
                            if ((j & i) == 0) {
                                f[j] -= f[j | i];
                            }
                        }
                    }
                    elif (type == 2) {
                        // OR畳込み
                        if (!inv) {
                            if ((j & i) == 0) {
                                f[j | i] += f[j];
                            }
                        } else {
                            if ((j & i) == 0) {
                                f[j | i] -= f[j];
                            }
                        }
                    }
                }
            }
        }
    };

    auto A = a, B = b;
    trans(A), trans(B);
    vector<T> C(N);
    for (int i = 0; i < N; ++i) C[i] = A[i] * B[i];
    trans(C, true);
    return C;
}
