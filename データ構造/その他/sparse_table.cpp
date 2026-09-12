/*
https://ei1333.github.io/luzhiled/snippets/structure/sparse-table.html
を参考に

区間の最小値を求める

SparceTalbe(v):配列vで初期化　O(N log N),第二引数をfalseに指定するとmaxクエリ
query(l,r):[l,r)の最小値を返す　O(1)
*/

template <typename T, bool MIN = true>
struct SparseTable {
    vector<vector<T>> st;
    vector<int> lookup;
    int n;
    const T INF = inf;

    SparseTable(const vector<T> &v) : n(v.size()) {
        int b = 0;
        while ((1 << b) <= (int)v.size()) ++b;
        st.assign(b, vector<T>(1 << b));
        for (int i = 0; i < (int)v.size(); i++) {
            st[0][i] = v[i];
        }
        for (int i = 1; i < b; i++) {
            for (int j = 0; j + (1 << i) <= (1 << b); j++) {
                if (MIN)
                    st[i][j] = min(st[i - 1][j], st[i - 1][j + (1 << (i - 1))]);
                else
                    st[i][j] = max(st[i - 1][j], st[i - 1][j + (1 << (i - 1))]);
            }
        }
        lookup.resize(v.size() + 1);
        for (int i = 2; i < (int)lookup.size(); i++) {
            lookup[i] = lookup[i >> 1] + 1;
        }
    }
    T e() {
        if (MIN) {
            return INF;
        } else {
            return -INF;
        }
    }

    inline T query(int l, int r) {
        if (l >= r) return e();
        if (l >= n) return e();
        if (r <= 0) return e();
        int b = lookup[r - l];
        if (MIN) {
            return min(st[b][l], st[b][r - (1 << b)]);
        } else {
            return max(st[b][l], st[b][r - (1 << b)]);
        }
    }
};
