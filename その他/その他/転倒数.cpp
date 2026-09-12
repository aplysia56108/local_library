/*
https://kopricky.github.io/code/Computation_Advanced/inversion_count.html
を参考に

int inv_count(v<T> a,v<T> b):aをbに変化させるのに必要な転倒数。不可能な場合-1
int inv_count(v<T> a):aを昇順に変えるのに必要な転倒数。
*/

//	右閉区間にする場合は第二引数をfalseに
template <typename T = int, bool half_open_interval = true>
struct BIT_for_inv_count {
    vector<T> data;

    BIT_for_inv_count(int sz) {
        data.assign(++sz + 1, 0);
    }

    T sum(int k) {
        T ret = 0;
        for (++k; k > 0; k -= k & -k) ret += data[k];
        return (ret);
    }

    T sum(int s, int k) {
        if (half_open_interval) k--;
        if (s > k) return 0;
        if (s == 0)
            return sum(k);
        else {
            return sum(k) - sum(s - 1);
        }
    }

    void add(int k, T x) {
        for (++k; k < (int)data.size(); k += k & -k) data[k] += x;
    }

    friend ostream &operator<<(ostream &os, BIT_for_inv_count ft) {
        for (int i = 0; i < (int)ft.data.size(); i++) {
            os << ft.sum(i, i) << " ";
        }
        os << endl;
        return (os);
    }
};

// AをBに変換するのに必要な転倒数
template <class T>
int inv_count(vector<T> a, vector<T> b) {
    if (a.size() != b.size()) return -1;
    int n = a.size();
    vector<T> c = a;
    for (int i = 0; i < n; i++) {
        c.push_back(b[i]);
    }
    sort(c.begin(), c.end());
    c.erase(unique(c.begin(), c.end()), c.end());
    for (int i = 0; i < n; i++) {
        a[i] = lower_bound(c.begin(), c.end(), a[i]) - c.begin();
        b[i] = lower_bound(c.begin(), c.end(), b[i]) - c.begin();
    }

    vector<int> a1 = a, b1 = b;
    sort(a1.begin(), a1.end());
    sort(b1.begin(), b1.end());
    if (a1 != b1) return -1;

    BIT_for_inv_count<int, 0> bt(n + 1);
    vector<vector<int>> v(n);
    for (int i = n - 1; i >= 0; i--) {
        v[a[i]].push_back(i);
    }
    int ans = 0;
    for (int i = 0; i < n; i++) {
        int k = v[b[i]].back();
        v[b[i]].pop_back();
        ans += i - bt.sum(k);
        bt.add(k, 1);
    }
    return ans;
}

// Aを昇順にするのに必要な転倒数
template <class T>
int inv_count(vector<T> A) {
    vector<T> B = A;
    sort(B.begin(), B.end());
    return inv_count(A, B);
}
