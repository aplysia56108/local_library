#pragma once

#include <bits/stdc++.h>
#include <atcoder/modint>

#ifndef NODE_CAP
#define NODE_CAP 8
#endif

#if defined(BENCH_OP_MAX_RIGHT) || defined(BENCH_OP_MIN_LEFT)
#define BENCH_BOUNDARY_SEARCH
#endif

#if defined(BENCH_TREE_SET)
#include "data-structure/btree/bplustree-set.hpp"
using BenchmarkTree =
    Bplustree_Set<long long, SetType::Set, std::less<long long>, NODE_CAP>;
using BenchmarkValue = long long;
#elif defined(BENCH_TREE_VECTOR)
#include "data-structure/btree/bplustree-vector.hpp"
using BenchmarkTree = Bplustree_Vector<long long, NODE_CAP>;
using BenchmarkValue = long long;
#elif defined(BENCH_TREE_SEGTREE)
#include "data-structure/btree/bplustree-segtree.hpp"
using BenchmarkMint = atcoder::modint998244353;
#if defined(BENCH_BOUNDARY_SEARCH)
// A nonnegative sum gives a monotone predicate for max_right / min_left.
using BenchmarkValue = long long;
using BenchmarkTree = Bplustree_Segtree<SumMonoid, NODE_CAP>;
#else
using BenchmarkValue = std::pair<BenchmarkMint, BenchmarkMint>;
using BenchmarkTree = Bplustree_Segtree<AffineMonoid<BenchmarkMint>, NODE_CAP>;
#endif
#elif defined(BENCH_TREE_LAZY)
#include "data-structure/btree/bplustree-lazysegtree.hpp"
using BenchmarkMint = atcoder::modint998244353;
#if defined(BENCH_BOUNDARY_SEARCH)
using BenchmarkValue = long long;
using BenchmarkTree = Bplustree_LazySegtree<
    SumMonoid,
    Action<long long, long long, AddSumMapping, AddComposition>,
    true, NODE_CAP>;
#else
using BenchmarkValue = BenchmarkMint;
using BenchmarkTree = AffineSumLazySegtree<BenchmarkMint, NODE_CAP>;
#endif
#else
#error "Unknown benchmark tree"
#endif

namespace benchmark_case {

using ll = long long;
struct Query {
  ll x = 0, y = 0, a = 0, b = 0;
};

inline BenchmarkValue read_value() {
  ll a;
  std::cin >> a;
#if defined(BENCH_TREE_SEGTREE) && !defined(BENCH_BOUNDARY_SEARCH)
  ll b;
  std::cin >> b;
  return {BenchmarkMint(a), BenchmarkMint(b)};
#elif defined(BENCH_TREE_LAZY) && !defined(BENCH_BOUNDARY_SEARCH)
  return BenchmarkMint(a);
#else
  return a;
#endif
}

inline BenchmarkValue inserted_value(const Query& q) {
#if defined(BENCH_TREE_SEGTREE) && !defined(BENCH_BOUNDARY_SEARCH)
  return {BenchmarkMint(q.a), BenchmarkMint(q.b)};
#elif defined(BENCH_TREE_LAZY) && !defined(BENCH_BOUNDARY_SEARCH)
  return BenchmarkMint(q.y);
#else
  return q.y;
#endif
}

inline void print(const BenchmarkValue& value) {
#if defined(BENCH_TREE_SEGTREE) && !defined(BENCH_BOUNDARY_SEARCH)
  std::cout << value.first.val() << ' ' << value.second.val() << '\n';
#elif defined(BENCH_TREE_LAZY) && !defined(BENCH_BOUNDARY_SEARCH)
  std::cout << value.val() << '\n';
#else
  std::cout << value << '\n';
#endif
}
}  // namespace benchmark_case

/**
 * @brief 固定された入力の B+tree 操作のみを計測する。
 * @details
 * 入力読み込み・初期構築・結果の出力は計測外。
 * 結果は標準出力、計測値は標準エラー出力へ書く。
 */
signed main() {
  using namespace benchmark_case;
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  ll n, q;
  if (!(std::cin >> n >> q) || n < 0 || q < 0) return 1;
  std::vector<BenchmarkValue> initial;
  initial.reserve(n);
  for (ll i = 0; i < n; ++i) {
    initial.push_back(read_value());
    if (!std::cin) return 1;
  }
  std::vector<Query> queries(q);
#if !defined(BENCH_OP_ACCESS) && !defined(BENCH_OP_ITERATE)
  for (auto& x : queries) {
#if defined(BENCH_OP_INSERT)
#if defined(BENCH_TREE_SET)
    std::cin >> x.x;
#elif defined(BENCH_TREE_SEGTREE)
    std::cin >> x.x >> x.a >> x.b;
#else
    std::cin >> x.x >> x.y;
#endif
#elif defined(BENCH_OP_ERASE)
    std::cin >> x.x;
#elif defined(BENCH_OP_REVERSE) || defined(BENCH_OP_PROD)
    std::cin >> x.x >> x.y;
#elif defined(BENCH_OP_APPLY)
    std::cin >> x.x >> x.y >> x.a >> x.b;
#elif defined(BENCH_OP_RANGE_ERASE) || defined(BENCH_OP_MAX_RIGHT) || \
    defined(BENCH_OP_MIN_LEFT)
    std::cin >> x.x >> x.y;
#elif defined(BENCH_OP_SPLIT_MERGE) || defined(BENCH_OP_LOWER_BOUND)
    std::cin >> x.x;
#else
#error "Unknown benchmark operation"
#endif
    if (!std::cin) return 1;
  }
#endif

  BenchmarkTree tree(initial.begin(), initial.end());
#if defined(BENCH_OP_PROD) || defined(BENCH_OP_ACCESS) || \
    defined(BENCH_OP_ITERATE) || defined(BENCH_OP_LOWER_BOUND) || \
    defined(BENCH_BOUNDARY_SEARCH)
  std::vector<BenchmarkValue> answers(q);
#endif

  const auto start = std::chrono::steady_clock::now();
#if defined(BENCH_OP_INSERT)
  for (const auto& x : queries) {
#if defined(BENCH_TREE_SET)
    tree.insert(x.x);
#else
    tree.insert(x.x, inserted_value(x));
#endif
  }
#elif defined(BENCH_OP_ERASE)
  for (const auto& x : queries) tree.erase(x.x);
#elif defined(BENCH_OP_REVERSE)
  for (const auto& x : queries) tree.reverse(x.x, x.y);
#elif defined(BENCH_OP_PROD)
  for (ll i = 0; i < q; ++i)
    answers[i] = tree.prod(queries[i].x, queries[i].y);
#elif defined(BENCH_OP_APPLY)
  for (const auto& x : queries)
    tree.apply(x.x, x.y, {BenchmarkMint(x.a), BenchmarkMint(x.b)});
#elif defined(BENCH_OP_ACCESS)
  for (ll i = 0; i < q; ++i) answers[i] = tree[i];
#elif defined(BENCH_OP_RANGE_ERASE)
  for (const auto& x : queries) tree.erase(x.x, x.y);
#elif defined(BENCH_OP_SPLIT_MERGE)
  for (const auto& x : queries) {
    auto right = tree.split(x.x);
    tree.merge(right);
  }
#elif defined(BENCH_OP_LOWER_BOUND)
  for (ll i = 0; i < q; ++i) {
    auto it = tree.lower_bound(queries[i].x);
    answers[i] = it == tree.end() ? -1 : *it;
  }
#elif defined(BENCH_OP_MAX_RIGHT)
  for (ll i = 0; i < q; ++i) {
    const ll limit = queries[i].y;
    answers[i] = tree.max_right(queries[i].x,
                                [limit](const BenchmarkValue& sum) {
                                  return sum <= limit;
                                });
  }
#elif defined(BENCH_OP_MIN_LEFT)
  for (ll i = 0; i < q; ++i) {
    const ll limit = queries[i].y;
    answers[i] = tree.min_left(queries[i].x,
                               [limit](const BenchmarkValue& sum) {
                                 return sum <= limit;
                               });
  }
#elif defined(BENCH_OP_ITERATE)
  ll index = 0;
  for (const auto& value : tree) answers[index++] = value;
#else
#error "Unknown benchmark operation"
#endif
  const auto end = std::chrono::steady_clock::now();

  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  std::cerr << "BENCHMARK_NS=" << elapsed << '\n';
#if defined(BENCH_OP_PROD) || defined(BENCH_OP_ACCESS) || \
    defined(BENCH_OP_ITERATE) || defined(BENCH_OP_LOWER_BOUND) || \
    defined(BENCH_BOUNDARY_SEARCH)
  for (const auto& x : answers) print(x);
#else
  for (const auto& x : tree) print(x);
#endif
}
