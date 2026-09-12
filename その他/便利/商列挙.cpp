/*
https://ei1333.github.io/luzhiled/snippets/math/quotient-range.html
を参考に
floor(n)はO(√n)種類
{{x,y},z}のvectorを返す。x<=i<=yを満たすfloor(n/i)がzである。xの昇順で返す
O(√n)
*/
template <typename T>
vector<pair<pair<T, T>, T>> quotient_range(T N) {
    T M;
    vector<pair<pair<T, T>, T>> ret;
    for (M = 1; M * M <= N; M++) {
        ret.emplace_back(make_pair(M, M), N / M);
    }
    for (T i = M; i >= 1; i--) {
        T L = N / (i + 1) + 1;
        T R = N / i;
        if (L <= R && ret.back().first.second < L)
            ret.emplace_back(make_pair(L, R), N / L);
    }
    return ret;
}
