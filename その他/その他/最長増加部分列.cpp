/*
https://ei1333.github.io/luzhiled/snippets/dp/longest-increasing-subsequence.html
を参考に

Longest-Increasing-Subsequence(LIS)
LIS(vector<T> a,bool s):
s=trueの時、<となる部分列の長さ
s=falseの時、<=となる部分列の長さを返す。
*/

template <typename T>
size_t LIS(const vector<T> &a, bool strict) {
    vector<T> lis;
    for (auto &p : a) {
        typename vector<T>::iterator it;
        if (strict)
            it = lower_bound(begin(lis), end(lis), p);
        else
            it = upper_bound(begin(lis), end(lis), p);
        if (end(lis) == it)
            lis.emplace_back(p);
        else
            *it = p;
    }
    return lis.size();
}
