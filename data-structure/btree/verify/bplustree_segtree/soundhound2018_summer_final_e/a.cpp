#include <bits/stdc++.h>

#define private public
#define protected public
#include <atcoder/all>
#undef private
#undef protected

using namespace atcoder;
#include "utility/io.hpp"

#ifdef LOCAL
#include <debug>
#else
#define debug(...) ((void)0)
#endif

using namespace std;
using namespace std::string_literals;

#include "utility/all"

#define int long long

// ==========================================
// Mod Select
// ==========================================
// #define mod998244353
#define mod1000000007
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
#include "prime/prime.hpp"
Prime prime_manager;
// i番目参照可能なset
// #include "data-structure/btree/bplustree-set.hpp"
// 挿入、削除、区間反転可能なvector
// #include "data-structure/btree/bplustree-vector.hpp"
// 挿入、削除、区間反転可能なseg木
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

// https://atcoder.jp/contests/soundhound2018-summer-final-open/submissions/80911406

void solve() {
  int n, m;
  cin >> n >> m;

  vs s(m);
  cin >> s;

  static vector<mint> mem(n + 10);
  mem[0] = 1;
  rep(i, 1, n + 10) mem[i] = mem[i - 1] * 1000000;
  //   mint i1000000{1000000};

  auto f = [](pair<mint, int> a, pair<mint, int> b) -> pair<mint, int> {
    // static mint i1000000{1000000};
    return {a.first + b.first * mem[a.second], a.second + b.second};
  };
  auto e = []() -> pair<mint, int> {
    return {0, 0};
  };

  vector<BSegtree<pair<mint, int>, f, e, 16>> v(m);

  rep(i, m) {
    // v[i].resize(n);
    vector<pair<mint, int>> t(n);
    rep(j, n) t[j] = pair<mint, int>{mint{s[i][j] - 'a' + 1}, 1};
    v[i].assign(t.begin(), t.end());
  }

  int q;
  cin >> q;

  rep(i, q) {
    int t, x, y, l, r;
    cin >> t >> x >> y >> l >> r;
    l--;
    x--;
    y--;

    if (t == 1) {
      auto xr = v[x].split(r);
      auto xm = v[x].split(l);
      auto yr = v[y].split(r);
      auto ym = v[y].split(l);
      v[x].merge(ym);
      v[x].merge(xr);
      v[y].merge(xm);
      v[y].merge(yr);
    } else {
      out(v[x].prod(l, r).first);
    }
  }
}