/*
https://atcoder.jp/contests/abc323/submissions/46349737より
characteristic_polynomial(M):det(xI-M)を求める。O(N^3)
eterminant_polynomial(M0,M1):det(M0+M1x)を求める。O(N^3)
*/

template <typename T>
std::vector<T> characteristic_polynomial(std::vector<std::vector<T>> M) {
    assert(M.empty() or M.size() == M[0].size());
    int n = M.size();
    // reduce M to upper Hessenberg form
    for (int j = 0; j < n - 2; j++) {
        for (int i = j + 2; i < n; i++) {
            if (M[i][j] != 0) {
                std::swap(M[j + 1], M[i]);
                for (int k = 0; k < n; k++) std::swap(M[k][j + 1], M[k][i]);
                break;
            }
        }
        if (M[j + 1][j] == 0) continue;
        auto inv = T(1) / M[j + 1][j];
        for (int i = j + 2; i < n; i++) {
            auto coef = M[i][j] * inv;
            for (int k = j; k < n; k++) M[i][k] -= coef * M[j + 1][k];
            for (int k = 0; k < n; k++) M[k][j + 1] += coef * M[k][i];
        }
    }

    // compute the characteristic polynomial of upper Hessenberg matrix M
    std::vector<std::vector<T>> p(n + 1);
    p[0] = {T(1)};
    for (int i = 0; i < n; i++) {
        p[i + 1].resize(i + 2);
        for (int j = 0; j <= i; j++) {
            p[i + 1][j + 1] += p[i][j];
            p[i + 1][j] -= p[i][j] * M[i][i];
        }
        T betas = 1;
        for (int j = i - 1; j >= 0; j--) {
            betas *= M[j + 1][j];
            T coef = -betas * M[j][i];
            for (int k = 0; k <= j; k++) p[i + 1][k] += coef * p[j][k];
        }
    }
    return p[n];
}

template <typename T>
std::vector<T> determinant_polynomial(std::vector<std::vector<T>> M0,
                                      std::vector<std::vector<T>> M1) {
    assert(M0.size() == M1.size());
    assert(M0.size() == M0[0].size());
    assert(M1.size() == M1[0].size());
    int n = M0.size(), off = 0;
    T prod = 1;

    for (int p = 0; p < n; p++) {
        int pivot = -1;
        for (int i = p; i < n; i++) {
            if (M1[i][p] != 0) {
                pivot = i;
                break;
            }
        }
        if (pivot == -1) {
            if (++off > n) return std::vector<T>(n + 1, 0);
            for (int i = 0; i < p; i++) {
                for (int k = 0; k < n; k++) M0[k][p] -= M1[i][p] * M0[k][i];
                M1[i][p] = 0;
            }
            for (int i = 0; i < n; i++) std::swap(M0[i][p], M1[i][p]);
            p--;
            continue;
        }
        if (pivot != p) {
            std::swap(M0[p], M0[pivot]);
            std::swap(M1[p], M1[pivot]);
            prod *= -1;
        }
        prod *= M1[p][p];
        auto inv = T(1) / M1[p][p];
        for (int j = 0; j < n; j++) {
            M0[p][j] *= inv;
            M1[p][j] *= inv;
        }
        for (int i = 0; i < n; i++) {
            if (i == p) continue;
            auto coef = M1[i][p];
            for (int j = 0; j < n; j++) {
                M0[i][j] -= M0[p][j] * coef;
                M1[i][j] -= M1[p][j] * coef;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            M0[i][j] *= -1;
        }
    }
    auto poly = characteristic_polynomial(M0);
    std::vector<T> res(n + 1, 0);
    for (int i = 0; i + off <= n; i++) res[i] = prod * poly[i + off];
    return res;
}
