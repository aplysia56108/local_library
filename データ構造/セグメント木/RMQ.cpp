/*
https://algo-logic.info/segment-tree/ を参考に

RMQ:[0,n-1]について、区間ごとの最小値を管理する構造体
    第二引数をfalseにしたらmaxクエリ
update(i,x): i 番目の要素を x に更新。O(log(n))
query(a,b): [a,b) での最小の要素を取得。O(log(n))
*/
template <typename T = long long, bool MIN = true>
class RMQ {
   private:
    const T INF = MIN == true ? inf : -inf;
    int n;  // 葉の数
    int n_;
    vector<T> dat;  // 完全二分木の配列

    T query_sub(int a, int b, int k, int l, int r) {
        if (r <= a || b <= l) {
            return INF;
        } else if (a <= l && r <= b) {
            return dat[k];
        } else {
            T vl = query_sub(a, b, k * 2 + 1, l, (l + r) / 2);
            T vr = query_sub(a, b, k * 2 + 2, (l + r) / 2, r);
            if (MIN)
                return min(vl, vr);
            else
                return max(vl, vr);
        }
    }

   public:
    RMQ(int n_) : n_(n_) {  // 葉の数は 2^x の形
        dat.resize(n_ * 4, INF);
        int x = 1;
        while (n_ > x) {
            x *= 2;
        }
        n = x;
    }
    void set(int i, T x) {
        assert(i >= 0 && i < n_);
        i += n - 1;
        dat[i] = x;
        while (i > 0) {
            i = (i - 1) / 2;  // parent
            if (MIN)
                dat[i] = min(dat[i * 2 + 1], dat[i * 2 + 2]);
            else
                dat[i] = max(dat[i * 2 + 1], dat[i * 2 + 2]);
        }
    }

    // the minimum element of [a,b)
    T prod(int a, int b) {
        return query_sub(a, b, 0, 0, n);
    }

    T all_prod() {
        return query_sub(0, n, 0, 0, n);
    }

    T operator[](int a) {
        return prod(a, a + 1);
    }

    friend ostream &operator<<(ostream &os, RMQ rmq) {
        for (int i = 0; i < (int)rmq.n; i++) {
            if (rmq[i] != rmq.INF)
                os << rmq[i] << " ";
            else
                os << "$ ";
        }
        os << endl;
        return (os);
    }
};
