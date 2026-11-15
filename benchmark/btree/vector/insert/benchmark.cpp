#include <bits/stdc++.h>

#include "data-structure/btree/bplustree-vector.hpp"

/**
 * @brief Bplustree_Vector のランダム位置への挿入を計測する。
 * @details
 * 標準入力からクエリをすべて読み込んでから計測を開始し、全要素を
 * 標準出力に1行ずつ出力する。計測値は標準エラー出力に書く。
 */
signed main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  long long q;
  if (!(std::cin >> q) || q < 0) return 1;

  std::vector<std::pair<long long, long long>> queries(q);
  for (auto& [index, value] : queries) {
    if (!(std::cin >> index >> value)) return 1;
  }

  Bplustree_Vector<long long, 8> tree;
  const auto start = std::chrono::steady_clock::now();
  for (auto [index, value] : queries) tree.insert(index, value);
  const auto finish = std::chrono::steady_clock::now();

  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           finish - start)
                           .count();
  std::cerr << "BENCHMARK_NS=" << elapsed << '\n';
  for (long long i = 0; i < q; ++i) std::cout << tree[i] << '\n';
}
