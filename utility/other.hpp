#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

// ==========================================
// 1. 構文・エイリアスマクロ
// ==========================================

#define elif else if
#define ifnot(x) if (!(x))
#define all(obj) (obj).begin(), (obj).end()
#define rall(obj) (obj).rbegin(), (obj).rend()

// 立っているビット（1のビット）の位置 i を走査するループマクロ
// j_ &= j_ - 1 により最下位ビットを高速に消去
#define setbits(i, n)                                                   \
  for (long long j_ = (n), i = (j_ ? __builtin_ctzll(j_) : -1); j_ > 0; \
       j_ &= j_ - 1, i = (j_ ? __builtin_ctzll(j_) : -1))

// リターン省略記法 (yes/no/err 関数が別途定義されている前提)
#define ryes return yes();
#define rno return no();
#define rerr return err();
#define rout(x) return out(x);

// ==========================================
// 2. 高速 I/O 自動設定
// ==========================================

struct fast_ios {
  fast_ios() {
    std::cin.tie(nullptr);
    std::ios::sync_with_stdio(false);
    std::cout << std::fixed << std::setprecision(12);
  }
};

// C++17 で安全な inline グローバルインスタンス化
inline fast_ios fast_ios_;

// ==========================================
// 3. コンパイル最適化プラグラ
// ==========================================

#ifndef LOCAL
#pragma GCC optimize("-O3")
// #pragma GCC target("avx2")
// #pragma GCC optimize("unroll-loops")
#endif