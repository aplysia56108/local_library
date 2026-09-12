/*
https://algo-logic.info/run-length/
を参考に
*/
// encode: ランレングス圧縮を行う

template <class T = char, class U = string>
vector<pair<T, int>> encode(const U &str) {
    int n = (int)str.size();
    vector<pair<T, int>> ret;
    for (int l = 0; l < n;) {
        int r = l + 1;
        for (; r < n && str[l] == str[r]; r++) {
        };
        ret.push_back({str[l], r - l});
        l = r;
    }
    return ret;
}

// decode: ランレングス圧縮の復元を行う
template <class T = char, class U = string>
U decode(const vector<pair<T, int>> &code) {
    U ret;
    for (auto p : code) {
        for (int i = 0; i < p.second; i++) {
            ret.push_back(p.first);
        }
    }
    return ret;
}
