#include <bits/stdc++.h>
#include <atcoder/all>
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
  out("Yes");
  // out("Alice");
  // out("Takahashi");
}
inline void no() {
  out("No");
  // out("Bob");
  // out("Aoki");
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

#include "graph/hld.hpp"
void solve() {
  int n;
  cin >> n;

  vi a(n - 1), b(n - 1), c(n - 1);
  rep(i, n - 1) cin >> a[i] >> b[i] >> c[i];
  a--, b--;

  Graph g(n);
  rep(i, n - 1) g.add_edge(a[i], b[i], c[i], 1);

  HLD hld(g);

  SumSegtree seg(n - 1);
  rep(i, n - 1) {
    seg(hld[a[i], b[i]]) = c[i];
  }

  int q;
  cin >> q;
  rep(i, q) {
    int t;
    cin >> t;

    if (t == 1) {
      int j, w;
      cin >> j >> w;
      j--;
      seg(hld[a[j], b[j]]) = w;
    } else {
      int u, v;
      cin >> u >> v;
      u--, v--;

      int ans = 0;
      for (auto [l, r, reversed] : hld.path_edges(u, v)) {
        ans += seg.prod(l, r);
      }
      out(ans);
    }
  }
}