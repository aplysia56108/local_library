// https://atcoder.jp/contests/abc022/tasks/abc022_c

#ifdef LOCAL
#include <debug>
#else
#define debug(...) ((void)0)
#endif

#include <bits/stdc++.h>

#include <atcoder/all>

using namespace std;
using namespace atcoder;

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
#include "graph/dijkstra.hpp"
////////////////////////////////////////////////////////////////////////////////
void solve();

signed main() {
  int testcase = 1;
  // cin >> testcase;
  for (int i = 0; i < testcase; i++) {
    solve();
  }
}

void solve() {
  int n, m;
  cin >> n >> m;

  vi u(m), v(m), l(m);
  rep(i, m) {
    cin >> u[i] >> v[i] >> l[i];
  }

  u--, v--;

  int ans = inf;

  rep(i, m) {
    if (u[i] != 0 && v[i] != 0) continue;

    Graph g(n);
    rep(j, m) {
      if (j == i) continue;
      g.add_edge(u[j], v[j], l[j], 1);
    }
    Dijkstra d(g, 0);

    int to = 0;
    if (u[i] != 0) to = u[i];
    if (v[i] != 0) to = v[i];

    chmin(ans, d[to] + l[i]);
  }

  if (ans < inf)
    out(ans);
  else
    err();
}