/*
https://kopricky.github.io/code/DataStructure_Basic/bit_1d_rangeadd_rangesum.html
を参考に
*/

template <typename T = long long>
class BIT_1D_RangeAdd_RangeSum {
    template <typename U>
    class BIT_ {
       private:
        int n;
        vector<U> bit;

       public:
        void add(int i, const U x) {
            i++;
            while (i < n) {
                bit[i] += x, i += i & -i;
            }
        }
        U sum(int i) const {
            i++;
            U s = 0;
            while (i > 0) {
                s += bit[i], i -= i & -i;
            }
            return s;
        }
        BIT_(const int sz) : n(sz + 1), bit(n, 0) {
        }
    };

   private:
    int n;
    BIT_<T> bitc, biti;

   public:
    // 初期値はすべて 0
    BIT_1D_RangeAdd_RangeSum(const int sz) : n(sz), bitc(sz), biti(sz) {
    }
    // [l, r) に val を加算
    void add(const int l, const int r, const T val) {
        bitc.add(l, val), bitc.add(r, -val);
        biti.add(l, -val * (l - 1)), biti.add(r, val * (r - 1));
    }
    // [0, x] の和
    T sum(const int x) const {
        return bitc.sum(x) * x + biti.sum(x);
    }
    // [l, r) の和
    T sum(const int l, const int r) const {
        return sum(r - 1) - sum(l - 1);
    }
    friend ostream &operator<<(ostream &os, BIT_1D_RangeAdd_RangeSum ft) {
        for (int i = 0; i < (int)ft.n; i++) {
            os << ft.sum(i, i + 1) << " ";
        }
        os << endl;
        return (os);
    }
};
