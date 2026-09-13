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

////////////////////////////////////////////////////////////////////////////////
// https://judge.yosupo.jp/problem/predecessor_problem
/*
 * Let `S` be a set consisting of integers from `0` to `N - 1`.
 * Process the following `Q` queries:
 *
 * - `0 k`: If `k` is not in `S`, insert `k` into `S`. Otherwise, do nothing.
 * - `1 k`: If `k` is in `S`, remove `k` from `S`. Otherwise, do nothing.
 * - `2 k`: If `k` is in `S`, output `1`. Otherwise, output `0`.
 * - `3 k`: Output the smallest element in `S` that is greater than or equal to
 * `k`. If no such element exists, output `-1`.
 * - `4 k`: Output the largest element in `S` that is less than or equal to `k`.
 *   If no such element exists, output `-1`.
 */

#include "data-structure/set/btree-set.hpp"

void solve();

signed main() {
  int testcase = 1;
  // cin >> testcase;
  for (int i = 0; i < testcase; i++) {
    solve();
  }
}

void solve() {
  int n, q;
  cin >> n >> q;

  string s;
  cin >> s;

  vi a;
  rep(i, n) if (s[i] == '1') a.push_back(i);

  Btree_Set<int> st(all(a));

  rep(i, q) {
    int t, x;
    cin >> t >> x;
    if (t == 0) {
      st.insert(x);
    }
    elif (t == 1) {
      st.erase(x);
    }
    elif (t == 2) {
      out(st.contains(x));
    }
    elif (t == 3) {
      auto it = st.lower_bound(x);
      if (it == st.end())
        err();
      else
        out(*it);
    }
    else {
      auto it = st.upper_bound(x);
      if (it == st.begin())
        err();
      else
        out(*(--it));
    }
  }
}