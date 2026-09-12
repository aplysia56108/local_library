/*
https://kopricky.github.io/code/DataStructure_Basic/update_interval.html
を参考に
Segment Tree で持てないような大きな範囲についての範囲の0,1(ON/OFF) を管理する.
1(ON) の範囲を set<pair<int, int>>で持つ.
また範囲[1,3),[3,5)などはちゃんと[1,5)にマージされる.
update(l,r,k):[l,r)をk(0,1)に更新。ならしO(log n)
query(x):xが0か1か
*/

template <typename T>
class UpdateInterval {
   private:
    using ptt = pair<T, T>;

   public:
    // 1の区間をsetで保持
    set<ptt> st;
    UpdateInterval(){};
    //[l,r)をk(0,1)に更新
    void update(T l, T r, int k) {
        T lb = l, rb = r;
        while (1) {
            auto it = st.lower_bound(ptt(l, numeric_limits<T>::min()));
            if (it == st.end()) {
                break;
            }
            ptt p = *it;
            if (p.first > r) {
                break;
            }
            st.erase(it);
            if (p.second > r) {
                rb = max(rb, p.second);
                if (!k) st.insert(ptt(r, p.second));
                break;
            }
        }
        auto it = st.lower_bound(ptt(l, numeric_limits<T>::min()));
        if (it != st.begin()) {
            --it;
            ptt p = *it;
            if (p.second >= l) {
                st.erase(it);
                if (!k) st.insert(ptt(p.first, l));
                lb = min(lb, p.first);
            }
            if (p.second > r) {
                if (!k) st.insert(ptt(r, p.second));
                rb = max(rb, p.second);
            }
        }
        if (k) st.insert(ptt(lb, rb));
    }
    bool query(T i) {
        auto it = st.upper_bound({i, inf});
        if (it == st.begin()) return false;
        it--;
        auto [l, r] = *it;
        if (l <= i && r > i) return true;
        return false;
    }
};
