/*
https://ei1333.github.io/luzhiled/snippets/dp/slide-min.html を参考に

slide_min(v,k):配列vの幅kの連続区間の最小値をvectorで返す。
*/

template <typename T>
vector<T> slide_min(const vector<T> &v, int k) {
    deque<int> deq;
    vector<T> ret;
    for (int i = 0; i < v.size(); i++) {
        while (!deq.empty() && v[deq.back()] >= v[i]) {
            deq.pop_back();
        }
        deq.push_back(i);
        if (i - k + 1 >= 0) {
            ret.emplace_back(v[deq.front()]);
            if (deq.front() == i - k + 1) deq.pop_front();
        }
    }
    return ret;
}
