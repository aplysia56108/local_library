#pragma once

#include <bits/stdc++.h>
#include <atcoder/modint>

#ifndef NODE_CAP
#define NODE_CAP 8
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
using BenchmarkValue = std::pair<BenchmarkMint, BenchmarkMint>;
using BenchmarkTree = Bplustree_Segtree<AffineMonoid<BenchmarkMint>, NODE_CAP>;
#elif defined(BENCH_TREE_LAZY)
#include "data-structure/btree/bplustree-lazysegtree.hpp"
using BenchmarkMint = atcoder::modint998244353;
using BenchmarkValue = BenchmarkMint;
using BenchmarkTree = AffineSumLazySegtree<BenchmarkMint, NODE_CAP>;
#else
#error "Unknown benchmark tree"
#endif

namespace mixed_benchmark {
using ll = long long;

struct Query {
  char op = 0;
  ll x = 0, y = 0, a = 0, b = 0;
};

inline BenchmarkValue read_value() {
  ll a;
  std::cin >> a;
#if defined(BENCH_TREE_SEGTREE)
  ll b;
  std::cin >> b;
  return {BenchmarkMint(a), BenchmarkMint(b)};
#elif defined(BENCH_TREE_LAZY)
  return BenchmarkMint(a);
#else
  return a;
#endif
}

inline BenchmarkValue inserted_value(const Query& q) {
#if defined(BENCH_TREE_SEGTREE)
  return {BenchmarkMint(q.a), BenchmarkMint(q.b)};
#elif defined(BENCH_TREE_LAZY)
  return BenchmarkMint(q.y);
#else
  return q.y;
#endif
}

inline void print(const BenchmarkValue& value) {
#if defined(BENCH_TREE_SEGTREE)
  std::cout << value.first.val() << ' ' << value.second.val() << '\n';
#elif defined(BENCH_TREE_LAZY)
  std::cout << value.val() << '\n';
#else
  std::cout << value << '\n';
#endif
}

inline bool parse(Query& q) {
  if (!(std::cin >> q.op)) return false;
  switch (q.op) {
    case 'I':
#if defined(BENCH_TREE_SET)
      return static_cast<bool>(std::cin >> q.x);
#elif defined(BENCH_TREE_SEGTREE)
      return static_cast<bool>(std::cin >> q.x >> q.a >> q.b);
#else
      return static_cast<bool>(std::cin >> q.x >> q.y);
#endif
    case 'E':
    case 'S':
    case 'G':
    case 'L':
      return static_cast<bool>(std::cin >> q.x);
    case 'R':
    case 'D':
    case 'Q':
      return static_cast<bool>(std::cin >> q.x >> q.y);
    case 'A':
      return static_cast<bool>(std::cin >> q.x >> q.y >> q.a >> q.b);
    default:
      return false;
  }
}
}  // namespace mixed_benchmark

/**
 * @brief 動的B+treeに混合操作を加え、クエリ結果と最終状態を検証可能にする。
 * @details 入力、初期構築、最終出力は計測外。クエリ結果のみ事前確保配列へ格納する。
 * ### 計算量
 * 操作数Qに対し、計測区間は各B+tree操作の計算量の総和。
 */
signed main() {
  using namespace mixed_benchmark;
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
  for (auto& query : queries) {
    if (!parse(query)) return 1;
  }

  BenchmarkTree tree(initial.begin(), initial.end());
  std::vector<BenchmarkValue> answers(q);
  ll answer_count = 0;

  const auto start = std::chrono::steady_clock::now();
  for (const Query& query : queries) {
    switch (query.op) {
      case 'I':
#if defined(BENCH_TREE_SET)
        tree.insert(query.x);
#else
        tree.insert(query.x, inserted_value(query));
#endif
        break;
      case 'E':
        tree.erase(query.x);
        break;
#if !defined(BENCH_TREE_SET)
      case 'D':
        tree.erase(query.x, query.y);
        break;
      case 'R':
        tree.reverse(query.x, query.y);
        break;
      case 'S': {
        auto right = tree.split(query.x);
        tree.merge(right);
        break;
      }
      case 'G':
        answers[answer_count++] = tree[query.x];
        break;
#endif
#if defined(BENCH_TREE_SEGTREE) || defined(BENCH_TREE_LAZY)
      case 'Q':
        answers[answer_count++] = tree.prod(query.x, query.y);
        break;
#endif
#if defined(BENCH_TREE_LAZY)
      case 'A':
        tree.apply(query.x, query.y,
                   {BenchmarkMint(query.a), BenchmarkMint(query.b)});
        break;
#endif
#if defined(BENCH_TREE_SET)
      case 'L': {
        auto it = tree.lower_bound(query.x);
        answers[answer_count++] = it == tree.end() ? -1 : *it;
        break;
      }
#endif
      default:
        return 2;
    }
  }
  const auto end = std::chrono::steady_clock::now();
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  std::cerr << "BENCHMARK_NS=" << elapsed << '\n';

  for (ll i = 0; i < answer_count; ++i) print(answers[i]);
  for (const auto& value : tree) print(value);
}
