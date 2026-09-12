/*
https://github.com/drken1215/algorithm/blob/master/DataStructure/mo.cpp
を参考に

過去問提出例：
https://atcoder.jp/contests/abc174/submissions/30910300
https://atcoder.jp/contests/abc242/submissions/30190084

Mo(n,vi l,vi r):コンストラクタ ([l,r],0-index)
build():ビルド

insertとeraseに処理を書く

答え出力
vi ans(q);
rep(i, q) {
    int k = mo.next();
    ans[k] = now;
}
cout << ans << el;
*/

struct Mo {
    vector<int> left, right, index;
    int d;
    int nl, nr, ptr;

    Mo(int n, vector<int> l, vector<int> r)
        : left(l),
          right(r),
          d(n / (sqrt(l.size()) + 1) + 1),
          nl(0),
          nr(-1),
          ptr(0) {
    }

    void build() {
        index.resize(left.size());
        iota(index.begin(), index.end(), 0);
        sort(index.begin(), index.end(), [&](int a, int b) {
            if (right[a] / d != right[b] / d) return right[a] < right[b];
            if (right[a] / d % 2 == 1) return left[a] > left[b];
            return left[a] < left[b];
        });
    }

    void insert(int id);
    void erase(int id);

    int next() {
        if (ptr == (int)index.size()) return -1;
        int id = index[ptr];
        while (nl < left[id]) erase(nl++);
        while (nl > left[id]) insert(--nl);
        while (nr < right[id]) insert(++nr);
        while (nr > right[id]) erase(nr--);
        return index[ptr++];
    }
};

void Mo::insert(int id) {
}

void Mo::erase(int id) {
}
