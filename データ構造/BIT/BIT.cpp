/*
https://ei1333.github.io/luzhiled/snippets/structure/binary-indexed-tree.html
を参考に

BIT<tymename T> bit(n):サイズnで初期化
sum(k):[0,k]の和
sum(s,k)[s,k]の和
add(k,x):kにxを加える。
*/

//	右閉区間にする場合は第二引数をfalseに
template <typename T = int, bool half_open_interval = true>
struct BIT {
    vector<T> data;

    BIT(int sz) {
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

    friend ostream &operator<<(ostream &os, BIT ft) {
        for (int i = 0; i < (int)ft.data.size(); i++) {
            os << ft.sum(i, i) << " ";
        }
        os << endl;
        return (os);
    }
};
