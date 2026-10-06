#pragma once

#include <cctype>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <numeric>
#include <atcoder/segtree.hpp>

// 基本型
using ll = long long;
using ull = unsigned long long;
#ifdef __SIZEOF_INT128__
using i128 = __int128_t;
using LL = __int128_t;
using u128 = __uint128_t;
#endif
using db = double;
using ld = long double;

// ペア型
using pi = std::pair<long long, long long>;
using pip = std::pair<long long, std::pair<long long, long long>>;

// tuple型
using ti3 = std::tuple<long long, long long, long long>;
using ti4 = std::tuple<long long, long long, long long, long long>;
using ti5 = std::tuple<long long, long long, long long, long long, long long>;
using ti6 = std::tuple<long long, long long, long long, long long, long long,
                       long long>;

// 1次元 vector
using vi = std::vector<long long>;
using vd = std::vector<double>;
using vb = std::vector<bool>;
using vs = std::vector<std::string>;
using vc = std::vector<char>;
using vp = std::vector<std::pair<long long, long long>>;

// 2次元 vector
using vvi = std::vector<std::vector<long long>>;
using vvd = std::vector<std::vector<double>>;
using vvb = std::vector<std::vector<bool>>;
using vvs = std::vector<std::vector<std::string>>;
using vvc = std::vector<std::vector<char>>;
using vvp = std::vector<std::vector<std::pair<long long, long long>>>;

// 多次元 vector
using vvvi = std::vector<std::vector<std::vector<long long>>>;
using vvvvi = std::vector<std::vector<std::vector<std::vector<long long>>>>;

// テンプレートエイリアス
template <typename T>
using vec = std::vector<T>;
template <typename T>
using vv = std::vector<std::vector<T>>;
template <typename T>
using vvv = std::vector<std::vector<std::vector<T>>>;
template <typename T>
using vvvv = std::vector<std::vector<std::vector<std::vector<T>>>>;

// コンテナ・データ構造
template <typename T>
using pq = std::priority_queue<T>;
template <typename T>
using pqg = std::priority_queue<T, std::vector<T>, std::greater<T>>;
template <typename T>
using mset = std::multiset<T>;
template <typename T>
using uset = std::unordered_set<T>;
template <typename T, typename U>
using umap = std::unordered_map<T, U>;

inline const long long segtree_inf = (1LL << 30) + (1LL << 60) - 2;

using Min_Segtree = atcoder::segtree<long long,
                                     [](long long a, long long b) {
                                       return std::min(a, b);
                                     },
                                     []() -> long long {
                                       return segtree_inf;
                                     }>;
using Max_Segtree = atcoder::segtree<long long,
                                     [](long long a, long long b) {
                                       return std::max(a, b);
                                     },
                                     []() -> long long {
                                       return -segtree_inf;
                                     }>;
using Sum_Segtree = atcoder::segtree<long long,
                                     [](long long a, long long b) {
                                       return a + b;
                                     },
                                     []() -> long long {
                                       return 0;
                                     }>;
using Gcd_Segtree = atcoder::segtree<long long,
                                     [](long long a, long long b) {
                                       return std::gcd(a, b);
                                     },
                                     []() -> long long {
                                       return 0;
                                     }>;