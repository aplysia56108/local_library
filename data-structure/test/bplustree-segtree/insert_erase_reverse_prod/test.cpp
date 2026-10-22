#ifdef LOCAL
#include <debug>
#else
#define debug(...) ((void)0)
#endif

#include <bits/stdc++.h>

#include <atcoder/all>

using namespace std;
using namespace atcoder;
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

using pm = pair<mint, mint>;
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
#include "data-structure/btree/bplustree-segtree.hpp"
////////////////////////////////////////////////////////////////////////////////
void solve();

signed main() {
  int testcase = 1;
  // cin >> testcase;
  for (int i = 0; i < testcase; i++) {
    solve();
  }
}

struct M {
  using value_type = pm;
  static pm op(pm a, pm b) {
    return {a.first * b.first, a.second + a.first * b.second};
  }
  static pm e() {
    return {1, 0};
  }
};
void solve() {
  int q;
  cin >> q;

  using BSeg = Bplustree_Segtree<M, 3>;
  vec<pm> v;
  BSeg seg;

  auto comp = [&](vec<pm>& v, BSeg& seg) -> void {
    assert(v.size() == seg.size());
    rep(i, sz(v)) {
      assert(v[i] == seg[i]);
    }
  };

  auto naive_prod = [&](int l, int r) {
    pm res = M::e();
    rep(i, l, r) {
      res = M::op(res, v[i]);
    }
    return res;
  };

  // 0 : i a b i番目に(a, b)を挿入
  // 1 : l, r [l, r)を区間反転。
  // 2 : i a b 1点更新。
  // 3 : l, r 区間取得。
  // 4 : i i番目を削除
  // 5 : l, r [l, r)を削除。区間幅は100以下

  rep(i_, q) {
    int t;
    cin >> t;
    if (t == 0) {
      int i, a, b;
      cin >> i >> a >> b;
      cerr << i_ << ' ' << i << ' ' << a << ' ' << b << endl;

      v.insert(v.begin() + i, {a, b});
      seg.insert(i, {a, b});
    }
    elif (t == 1) {
      int l, r;
      cin >> l >> r;
      cerr << i_ << ' ' << t << ' ' << l << ' ' << r << endl;

      reverse(v.begin() + l, v.begin() + r);
      seg.reverse(l, r);
    }
    elif (t == 2) {
      int i, a, b;
      cin >> i >> a >> b;
      cerr << i_ << ' ' << i << ' ' << a << ' ' << b << endl;

      v[i] = {a, b};
      seg.set(i, {a, b});
    }
    elif (t == 3) {
      int l, r;
      cin >> l >> r;
      cerr << i_ << ' ' << t << ' ' << l << ' ' << r << endl;

      auto res = seg.prod(l, r);
      out(seg.prod(l, r));

      pm comp = M::e();
      rep(i, l, r) comp = M::op(comp, v[i]);
      assert(res == comp);
    }
    elif (t == 4) {
      int i;
      cin >> i;

      cerr << i_ << ' ' << i << endl;

      v.erase(v.begin() + i);
      seg.erase(i);
    }
    else {
      int l, r;
      cin >> l >> r;
      cerr << i_ << ' ' << t << ' ' << l << ' ' << r << endl;

      v.erase(v.begin() + l, v.begin() + r);
      seg.erase(l, r);
    }

    // comp() より先にやる
    {
      auto expected = naive_prod(0, sz(v));
      auto actual = seg.prod(0, seg.size());

      if (actual != expected) {
        cerr << "Broken at query " << i_ << endl;
        debug(actual, expected);
        assert(false);
      }
    }

    comp(v, seg);
    cerr << "Query " << i_ << " is OK." << endl;
  }

  cout << "All query passed." << endl;
}