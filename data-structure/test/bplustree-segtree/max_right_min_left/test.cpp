#include <atcoder/all>
using namespace atcoder;
#include "utility/io.hpp"

#ifdef LOCAL
#include <debug>
#else
#define debug(...) ((void)0)
#endif

#include <bits/stdc++.h>

using namespace std;
using namespace std::string_literals;

#include "utility/all"

#define int long long

// ==========================================
// Mod Select
// ==========================================
#define mod998244353
// #define mod1000000007
// #define mod_dynamic

#ifdef mod998244353
inline long long mod = 998244353;
using mint = modint998244353;
#endif

#ifdef mod1000000007
inline long long mod = 1000000007;
using mint = modint1000000007;
#endif

#ifdef mod_dynamic
using mint = modint;
#endif

using pm = std::pair<mint, mint>;
using vm = std::vector<mint>;
using vvm = std::vector<vector<mint>>;
using vvvm = std::vector<vector<vector<mint>>>;
inline Binomial<mint> C;
inline ModPowTable<mint> mtwo(2), mten(10);

inline void yes() {
  cout << "Yes" << endl;
  // cout << "Alice" << endl;
  // cout << "Takahashi" << endl;
}
inline void no() {
  cout << "No" << endl;
  // cout << "Bob" << endl;
  // cout << "Aoki" << endl;
}
inline void yesno(bool x) {
  if (x)
    yes();
  else
    no();
}
inline void err() {
  cout << -1 << endl;
}

inline const long long dx[] = {1, 0, -1, 0, 1, -1, 1, -1};
inline const long long dy[] = {0, 1, 0, -1, -1, 1, 1, -1};

// 8方向 時計回り (右 -> 右下 -> 下 -> 左下 -> 左 -> 左上 -> 上 -> 右上)
inline const long long dx8r[] = {1, 1, 0, -1, -1, -1, 0, 1};
inline const long long dy8r[] = {0, -1, -1, -1, 0, 1, 1, 1};

// 8方向 反時計回り (右 -> 右上 -> 上 -> 左上 -> 左 -> 左下 -> 下 -> 右下)
inline const long long dx8l[] = {1, 1, 0, -1, -1, -1, 0, 1};
inline const long long dy8l[] = {0, 1, 1, 1, 0, -1, -1, -1};

inline const long long dx4r[] = {1, 0, -1, 0};
inline const long long dy4r[] = {0, -1, 0, 1};

inline const long long dx4l[] = {1, 0, -1, 0};
inline const long long dy4l[] = {0, 1, 0, -1};

inline const long long inf = (1LL << 30) + (1LL << 60) - 2;
inline const double eps = 1e-9;
////////////////////////////////////////////////////////////////////////////////
// i番目参照可能なset
// #include "data-structure/btree/bplustree-set.hpp"
// 挿入、削除、区間反転可能なvector
// #include "data-structure/btree/bplustree-vector.hpp"
// 挿入、削除、区間反転可能なseg木
// #include "data-structure/btree/bplustree-segtree.hpp"
////////////////////////////////////////////////////////////////////////////////
void solve();

signed main() {
  int testcase = 1;
  // cin >> testcase;
  for (int i = 0; i < testcase; i++) {
    solve();
  }
}

#include "data-structure/btree/bplustree-segtree.hpp"

using T = AffineMonoid<int>;

void non_commutative_binary_search_test() {
  constexpr long long M = 50;
  constexpr long long Q = 5000;

  std::vector<pi> t(M);

  // a >= 1, b >= 1 にして、区間を広げるほど f(0) が単調増加するようにする
  rep(i, M) {
    t[i] = {rnd(2) + 1, rnd(5) + 1};
  }

  // 非可換であることを一応確認
  assert(T::op(pi{2, 1}, pi{3, 4}) != T::op(pi{3, 4}, pi{2, 1}));

  Bplustree_Segtree<T, 3> seg(all(t));

  rep(_, Q) {
    std::size_t left = rnd(M + 1);
    long long p = rnd(1LL << 52) + 1;

    auto f = [p](const pi& x) {
      return x.second < p;
    };

    std::size_t res = seg.max_right(left, f);

    pi current = T::e();
    std::size_t expected = left;

    for (std::size_t j = left; j < M; ++j) {
      pi next = T::op(current, t[j]);
      if (!f(next)) break;

      current = next;
      expected = j + 1;
    }

    assert(res == expected);

    int l = rnd(M);
    int r = rnd(l, M + 1);
    seg.reverse(l, r);
    reverse(t.begin() + l, t.begin() + r);
  }

  rep(_, Q) {
    std::size_t right = rnd(M + 1);
    long long p = rnd(1LL << 52) + 1;

    auto f = [p](const pi& x) {
      return x.second < p;
    };

    std::size_t res = seg.min_left(right, f);

    pi current = T::e();
    std::size_t expected = right;

    for (std::size_t j = right; j > 0; --j) {
      pi next = T::op(t[j - 1], current);
      if (!f(next)) break;

      current = next;
      expected = j - 1;
    }

    assert(res == expected);
    int l = rnd(M);
    int r = rnd(l, M + 1);
    seg.reverse(l, r);
    reverse(t.begin() + l, t.begin() + r);
  }

  rep(i, M) {
    assert(seg[i] == t[i]);
  }

  cerr << "All query Succeeded!" << endl;
}

void solve() {
  int M = 1000;
  int Q = 5000;

  vi t(1000);

  rep(i, M) t[i] = rnd(100);

  BSegtreeWithoutE<int,
                   [](auto a, auto b) {
                     return max(a, b);
                   },
                   5>
      seg(all(t));
  rep(i, Q) {
    int left = rnd(M + 1);
    int p = rnd(101);
    // left以上で初めてp以上の値が現れる位置
    auto res = seg.upper_bound(left, [p](auto x) {
      return x < p;
    });

    std::size_t expected = M;
    rep(j, left, M) {
      if (t[j] >= p) {
        expected = j;
        break;
      }
    }
    assert(res == expected);
  }
  rep(i, Q) {
    int right = rnd(M + 1);
    int p = rnd(101);
    // right未満で初めてp以上の値が現れる位置+1
    auto res = seg.lower_bound(right, [p](auto x) {
      return x < p;
    });

    std::size_t expected = -1;

    rrep(j, right) {
      if (t[j] >= p) {
        expected = j;
        break;
      }
    }
    assert(res == expected + 1);
  }

  cerr << "All query Succeeded!" << std::endl;

  non_commutative_binary_search_test();
}