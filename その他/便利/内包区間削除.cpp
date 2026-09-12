/*
https://kopricky.github.io/code/Misc/erase_outer_inner_interval.html
を参考に

区間がn個与えられたときにある別の区間を完全に含んでいるような区間をすべて削除(EraseOuterInterval),
ある別の区間に完全に含まれているような区間をすべて削除(EraseInnerInterval)
という操作を効率良く行うアルゴリズム.
ただし同じ区間が複数入力に存在する場合はその区間はちょうど1つ出力のvector
に含まれる. また出力される vector 内の区間はx座標,y座標の順に昇順でソートされる.
時間計算量:O(nlogn)
*/

// 外側の区間を削除
template <typename T>
vector<pair<T, T>> EraseOuterInterval(vector<pair<T, T>> vec) {
    sort(vec.begin(), vec.end(), [](const pair<T, T> &a, const pair<T, T> &b) {
        return (a.second == b.second) ? (a.first < b.first)
                                      : (a.second > b.second);
    });
    vector<pair<T, T>> res;
    for (const pair<T, T> &p : vec) {
        while (!res.empty()) {
            const pair<T, T> &q = res.back();
            if (p.first < q.first) break;
            res.pop_back();
        }
        res.push_back(p);
    }
    reverse(res.begin(), res.end());
    return res;
}

// 内側の区間を削除
template <typename T>
vector<pair<T, T>> EraseInnerInterval(vector<pair<T, T>> vec) {
    sort(vec.begin(), vec.end(), [](const pair<T, T> &a, const pair<T, T> &b) {
        return (a.second == b.second) ? (a.first > b.first)
                                      : (a.second < b.second);
    });
    vector<pair<T, T>> res;
    for (const pair<T, T> &p : vec) {
        while (!res.empty()) {
            const pair<T, T> &q = res.back();
            if (q.first < p.first) break;
            res.pop_back();
        }
        res.push_back(p);
    }
    return res;
}
