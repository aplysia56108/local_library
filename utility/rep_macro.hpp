#pragma once

// 汎用 overload マクロ
#define overload4(a, b, c, d, e, ...) e

// --- rep (正順ループ) ---
#define rep1(n) for (long long i = 0; i < (long long)(n); ++i)
#define rep2(i, n) for (long long i = 0; i < (long long)(n); ++i)
#define rep3(i, a, b) for (long long i = (a); i < (long long)(b); ++i)
#define rep4(i, a, b, c) for (long long i = (a); i < (long long)(b); i += (c))
#define rep(...) overload4(__VA_ARGS__, rep4, rep3, rep2, rep1)(__VA_ARGS__)

// --- rrep (逆順ループ) ---
#define rrep1(n) for (long long i = (long long)(n) - 1; i >= 0; --i)
#define rrep2(i, n) for (long long i = (long long)(n) - 1; i >= 0; --i)
#define rrep3(i, a, b) \
  for (long long i = (long long)(b) - 1; i >= (long long)(a); --i)
#define rrep4(i, a, b, c)                                                    \
  for (long long i = (a) + ((b) - (a) - 1) / (c) * (c); i >= (long long)(a); \
       i -= (c))
// clang-format off
#define rrep(...) overload4(__VA_ARGS__, rrep4, rrep3, rrep2, rrep1)(__VA_ARGS__)
// clang-format on

// --- fore (レンジベース for / 構造化束縛) ---
#define fore1(a) for (auto&& x : a)
#define fore2(x, a) for (auto&& x : a)
#define fore3(x, y, a) for (auto&& [x, y] : a)
#define fore4(x, y, z, a) for (auto&& [x, y, z] : a)
// clang-format off
#define fore(...) overload4(__VA_ARGS__, fore4, fore3, fore2, fore1)(__VA_ARGS__)
// clang-format on
