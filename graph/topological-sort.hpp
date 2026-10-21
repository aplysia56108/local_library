#pragma once

#include <vector>
#include <queue>

#include "graph/graph.hpp"

/**
 * @brief 有向グラフをトポロジカルソートする。
 *
 * @param graph 隣接リストで表された有向グラフ。
 * @return DAG なら `{true, order}`、閉路を含むなら `{false, order}`。
 * `order` には Kahn 法で処理できた頂点が格納される。
 *
 * ### 計算量
 * $O(V + E)$
 */
inline std::pair<bool, std::vector<long long>> topological_sort(
    const Graph<long long>& graph) {
  const long long n = graph.size();

  std::vector<long long> indegree(n);
  for (const auto& edges : graph.G) {
    for (const auto& edge : edges) {
      ++indegree[edge.to];
    }
  }

  std::queue<long long> que;
  for (long long v = 0; v < n; ++v) {
    if (indegree[v] == 0) que.push(v);
  }

  std::vector<long long> order;
  order.reserve(n);

  while (!que.empty()) {
    const long long v = que.front();
    que.pop();

    order.push_back(v);

    for (const auto& edge : graph[v]) {
      if (--indegree[edge.to] == 0) {
        que.push(edge.to);
      }
    }
  }

  return {static_cast<long long>(order.size()) == n, std::move(order)};
}