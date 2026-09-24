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
void solve();

signed main() {
  int testcase = 1;
  // cin >> testcase;
  for (int i = 0; i < testcase; i++) {
    solve();
  }
}

// 区間は20%の確率で端を含む、空区間、全区間のいずれかとする。

// (a, b)=>ax + bの畳み込み
// cnt<Nの場合、選択可能
// 0 : i a b i番目に(a, b)を挿入

// 常に選択可能
// 1 : l, r [l, r)を区間反転。
// 2 : i a b 1点更新。
// 3 : l, r 区間取得。

// cnt==Nの場合、選択可能
// 4 : i i番目を削除
// 5 : l, r [l, r)を削除。区間幅は100以下

void solve() {
  int Q = 5000;
  int N = 1000;

  vvi querys(Q);
  int now_cnt = 0;

  // 通常の区間を生成する。
  // 20%の確率で
  //   - 左端または右端を含む
  //   - 空区間
  //   - 全区間
  // のいずれかを生成する。
  auto gen_range = [&](int n) -> pair<int, int> {
    if (n == 0) return {0, 0};

    if (rnd(5) == 0) {
      int type = rnd(3);

      // 端を含む区間
      if (type == 0) {
        if (rnd(2)) {
          int r = rnd(0, n + 1);
          return {0, r};
        } else {
          int l = rnd(0, n + 1);
          return {l, n};
        }
      }

      // 空区間
      if (type == 1) {
        int i = rnd(0, n + 1);
        return {i, i};
      }

      // 全区間
      return {0, n};
    }

    // 特殊ケース以外では適当な非空区間
    int l = rnd(0, n);
    int r = rnd(l + 1, n + 1);
    return {l, r};
  };

  // 幅100以下の削除区間。
  auto gen_erase_range = [&](int n) -> pair<int, int> {
    if (n == 0) return {0, 0};

    // type 5 では「幅100以下」の制約を優先する。
    if (rnd(5) == 0) {
      int type = rnd(3);

      // 端を含む区間
      if (type == 0) {
        int len = rnd(0, min<int>(100, n) + 1);
        if (rnd(2))
          return {0, len};
        else
          return {n - len, n};
      }

      // 空区間
      if (type == 1) {
        int i = rnd(0, n + 1);
        return {i, i};
      }

      // 全区間。ただし幅100以下の場合のみ。
      if (n <= 100) return {0, n};
    }

    int l = rnd(0, n + 1);
    int len = rnd(0, min<int>(100, n - l) + 1);
    return {l, l + len};
  };

  rep(qi, Q) {
    // 空の場合は一点更新できないので、まず要素を追加する。
    if (now_cnt == 0) {
      int a = rnd(1, 101);
      int b = rnd(0, 101);

      querys[qi] = {0, 0, a, b};
      ++now_cnt;
      continue;
    }

    int t;

    if (now_cnt < N) {
      // 0, 1, 2, 3
      t = rnd(0, 4);
    } else {
      // 1, 2, 3, 4, 5
      t = rnd(1, 6);
    }

    if (t == 0) {
      int i = rnd(0, now_cnt + 1);
      int a = rnd(1, 101);
      int b = rnd(0, 101);

      querys[qi] = {0, i, a, b};
      ++now_cnt;
    }

    elif (t == 1) {
      auto [l, r] = gen_range(now_cnt);
      querys[qi] = {1, l, r};
    }

    elif (t == 2) {
      int i = rnd(0, now_cnt);
      int a = rnd(1, 101);
      int b = rnd(0, 101);

      querys[qi] = {2, i, a, b};
    }

    elif (t == 3) {
      auto [l, r] = gen_range(now_cnt);
      querys[qi] = {3, l, r};
    }

    elif (t == 4) {
      int i = rnd(0, now_cnt);

      querys[qi] = {4, i};
      --now_cnt;
    }

    else {
      auto [l, r] = gen_erase_range(now_cnt);

      querys[qi] = {5, l, r};
      now_cnt -= r - l;
    }
  }
  out(Q);
  out(querys);
}