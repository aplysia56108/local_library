// この提出コードでテンプレートに書いたすべてのコードは私(aplysia)が自分で、あるいは生成AIの助けを借りて書いたものです。
// すべてのコードは以下のgithubリンクのどこかに存在します。
// https://github.com/aplysia56108/local_library

#include <bits/stdc++.h>

#ifdef LOCAL
#define private public
#define protected public
#endif

#include <atcoder/all>

#ifdef LOCAL
#undef private
#undef protected
#endif

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

#include "string/rolling_hash.hpp"

void solve() {
  string s;
  cin >> s;

  int n = sz(s);

  vi ans(2 * n - 1);

  auto reverse_s = s;
  reverse(all(reverse_s));

  RollingHash<char> h1(s);
  RollingHash<char> h2(reverse_s);
  debug(h1);
  debug(h2);
  rep(i, n) {
    int l = min(i + 1, n - i);
    ans[2 * i] = bin_search(0, l + 1,
                            [&](int m) -> bool {
                              return h1.prod(i, i + m) ==
                                     h2.prod(n - 1 - i, n - 1 - i + m);
                            }) *
                     2 -
                 1;
  }

  rep(i, n - 1) {
    int l = min(i + 1, n - i - 1);
    ans[2 * i + 1] = bin_search(0, l + 1,
                                [&](int m) -> bool {
                                  return h1.prod(i + 1, i + 1 + m) ==
                                         h2.prod(n - 1 - i, n - 1 - i + m);
                                }) *
                     2;
  }
  out(ans);
}