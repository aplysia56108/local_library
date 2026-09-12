/*
https://github.com/E869120/kyopro_educational_90/blob/main/sol/090-07b.cpp
を参考に
(C[0] + C[1] * x + ... * C[N-1] * x^(N-1)) * P(x) == 1 (mod x^L)を満たす、
L-1次以下の多項式 P(x) を計算量 O(N log N) で求める
制約: C[0]== 1でなければならない
説明：https://opt-cp.com/fps-fast-algorithms/

(下はニュートンを用いた別方法)
https://atcoder.jp/contests/typical90/submissions/30762425を参考にした
c[0]=1である必要はなさそう
*/

template <typename T>
vector<T> polynomial_inverse(vector<T> C, int L) {
    int N = C.size();
    vector<T> A = {1, 0};
    int level = 0;
    while ((1 << level) < L) {
        int CS = min(2 << level, N);
        vector<T> P = convolution(A, vector<T>(C.begin(), C.begin() + CS));
        vector<T> Q(2 << level);
        Q[0] = 1;
        for (int j = (1 << level); j < (2 << level); ++j) {
            Q[j] = -P[j];
        }
        A = convolution(A, Q);
        A.resize(4 << level);
        ++level;
    }
    A.resize(L);
    return A;
}

//ニュートン法を使ったやり方
// template <typename T>
// vector<T> polynomial_inverse(vector<T> a) {
//     int d = a.size();
//     vector<T> f = {1};
//     int accuracy = 1;
//     while (accuracy < d) {
//         accuracy *= 2;
//         f.resize(accuracy, 0);
//         vector<T> aa(a.begin(), a.begin() + min((int)a.size(), accuracy));
//         vector<T> f1(accuracy);
//         for (int i = 0; i < accuracy; i++) {
//             f1[i] = 2 * f[i];
//         }
//         vector<T> f2 = convolution(a, f);
//         f2.resize(accuracy, 0);
//         f2 = convolution(f2, f);
//         for (int i = 0; i < accuracy; i++) {
//             f[i] = f1[i] - f2[i];
//         }
//     }
//     f.resize(d);
//     return f;
// }
