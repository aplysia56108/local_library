#pragma once

#include <algorithm>
#include <vector>

#include "graph/graph.hpp"

/**
 * @brief 有向グラフを強連結成分分解する
 *
 * @details
 * Kosaraju 法により強連結成分分解を行う。
 *
 * 各強連結成分にはトポロジカル順に `0, 1, ..., k-1` の番号が付けられる。
 * したがって、異なる強連結成分間に辺 `u -> v` が存在する場合、
 * `scc[u] < scc[v]` が成り立つ。
 *
 * @tparam T 辺の重みの型
 */
template <typename T = long long>
class SCC {
 private:
  const Graph<T>& graph_;
  std::vector<std::vector<long long>> reverse_graph_;
  std::vector<long long> component_;
  std::vector<long long> order_;
  std::vector<bool> used_;
  long long component_count_ = 0;

  void dfs(long long v) {
    if (used_[v]) return;
    used_[v] = true;

    for (const auto& e : graph_[v]) {
      dfs(e.to);
    }

    order_.push_back(v);
  }

  void reverse_dfs(long long v, long long component) {
    if (component_[v] != -1) return;

    component_[v] = component;

    for (long long to : reverse_graph_[v]) {
      reverse_dfs(to, component);
    }
  }

 public:
  /**
   * @brief 強連結成分分解を行う
   *
   * @param graph 対象となる有向グラフ
   *
   * ### 計算量
   * $O(V + E)$
   */
  explicit SCC(const Graph<T>& graph)
      : graph_(graph),
        reverse_graph_(graph.size()),
        component_(graph.size(), -1),
        used_(graph.size(), false) {
    const long long n = graph_.size();

    for (long long v = 0; v < n; ++v) {
      for (const auto& e : graph_[v]) {
        reverse_graph_[e.to].push_back(v);
      }
    }

    order_.reserve(n);

    for (long long v = 0; v < n; ++v) {
      dfs(v);
    }

    std::reverse(order_.begin(), order_.end());

    for (long long v : order_) {
      if (component_[v] != -1) continue;
      reverse_dfs(v, component_count_);
      ++component_count_;
    }
  }

  /**
   * @brief 頂点が属する強連結成分の番号を取得する
   *
   * @param v 頂点番号
   * @return 強連結成分の番号
   */
  long long operator[](long long v) const {
    return component_[v];
  }

  /**
   * @brief 強連結成分の個数を取得する
   *
   * @return 強連結成分の個数
   */
  long long size() const {
    return component_count_;
  }

  /**
   * @brief 強連結成分を縮約した DAG を構築する
   *
   * @details
   * 各強連結成分を1頂点に縮約したグラフを返す。
   *
   * `remove_multiple_edges` が `false` の場合、元のグラフに複数の辺が
   * 存在すると縮約後のグラフにも多重辺が残る。
   * `true` の場合、同じ強連結成分間の多重辺を削除する。
   *
   * @param remove_multiple_edges 多重辺を削除する場合は true
   * @return 強連結成分を縮約した DAG
   *
   * ### 計算量
   * `remove_multiple_edges == false` の場合 $O(V + E)$、
   * `true` の場合 $O(V + E \log E)$
   */
  Graph<long long> build(bool remove_multiple_edges = false) const {
    Graph<long long> result(component_count_);

    if (!remove_multiple_edges) {
      for (long long v = 0; v < graph_.size(); ++v) {
        for (const auto& e : graph_[v]) {
          const long long from = component_[v];
          const long long to = component_[e.to];

          if (from == to) continue;

          result.add_edge(from, to, 1, false);
        }
      }

      return result;
    }

    std::vector<std::vector<long long>> edges(component_count_);

    for (long long v = 0; v < graph_.size(); ++v) {
      for (const auto& e : graph_[v]) {
        const long long from = component_[v];
        const long long to = component_[e.to];

        if (from == to) continue;

        edges[from].push_back(to);
      }
    }

    for (long long from = 0; from < component_count_; ++from) {
      auto& destinations = edges[from];

      std::sort(destinations.begin(), destinations.end());
      destinations.erase(std::unique(destinations.begin(), destinations.end()),
                         destinations.end());

      for (long long to : destinations) {
        result.add_edge(from, to, 1, false);
      }
    }

    return result;
  }

  /**
   * @brief 各強連結成分に属する頂点を取得する
   *
   * @details
   * 戻り値の `result[i]` には、強連結成分 `i` に属する頂点が格納される。
   * 強連結成分の番号は縮約 DAG のトポロジカル順になっている。
   *
   * @return 各強連結成分に属する頂点の一覧
   *
   * ### 計算量
   * $O(V)$
   */
  std::vector<std::vector<long long>> groups() const {
    std::vector<std::vector<long long>> result(component_count_);

    for (long long v = 0; v < graph_.size(); ++v) {
      result[component_[v]].push_back(v);
    }

    return result;
  }
};