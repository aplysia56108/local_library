/*
stackを使って、最大長方形の面積を求めるやつ

stackは、高さが単調増加になるような形
sumはその面積の合計値

add(int a,T w):高さa、幅wの長方形を追加。sumが返る。

https://atcoder.jp/contests/arc115/submissions/35446010
など
*/

template <typename T>
struct Histogram {
    T sum;
    stack<pair<int, T>> st;
    Histogram() : sum(0) {
    }
    T add(int a, T w) {
        while (st.size() && st.top().first >= a) {
            auto p = st.top();
            st.pop();
            sum -= p.second * p.first;
            w += p.second;
        }
        sum += w * a;
        st.emplace(a, w);
        return sum;
    }
};

// 蟻本p299,ヒストグラム内の最大面積
// i番目の高さがh[i]の配列hを与え、そのヒストグラム内での最大面積を返す。
long long Largest_Rectangle_in_a_Histogram(vector<long long> &h) {
    int n = h.size();
    vi L(n), R(n);
    {
        stack<int> st;
        for (int i = 0; i < n; i++) {
            while (st.size() && h[st.top()] >= h[i]) st.pop();
            L[i] = (int)st.size() == 0 ? 0 : st.top();
            st.push(i);
        }
    }

    {
        stack<int> st;
        for (int i = n - 1; i >= 0; i--) {
            while (st.size() && h[st.top()] >= h[i]) st.pop();
            R[i] = (int)st.size() == 0 ? n : st.top();
            st.push(i);
        }
    }
    long long res = 0;
    for (int i = 0; i < n; i++) {
        chmax(res, h[i] * (R[i] - L[i]));
    }
    return res;
}
