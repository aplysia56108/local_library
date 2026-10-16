#pragma once

#include <algorithm>
#include <stack>
#include <utility>
#include <vector>
#include <cassert>

#include "graph/graph.hpp"

/**
 * @brief Heavy-Light Decomposition
 *
 * @details
 * 木を Heavy-Light Decomposition により分解する。
 *
 * 各頂点 `v` には HLD 上の頂点番号 `index(v)` が割り当てられ、
 * `v` の部分木は区間
 * `[index(v), index(v) + subtree_size(v))`
 * に対応する。
 *
 * 森に対して使用する場合は `build()` に各木の根を指定する。
 *
 * @tparam T 辺の重みの型
 */
template <typename T = long long>
class HLD {
 private:
  const Graph<T>& graph_;
  long long n_;

  std::vector<long long> index_;
  std::vector<long long> edge_index_;
  std::vector<long long> edge_vertex_;
  std::vector<long long> vertex_;
  std::vector<long long> parent_;
  std::vector<long long> depth_;
  std::vector<long long> subtree_size_;
  std::vector<long long> head_;
  std::vector<long long> heavy_;
  std::vector<long long> component_;

  void build_subtree(long long root) {
    std::stack<std::pair<long long, bool>> stack;
    stack.push({root, false});

    parent_[root] = -1;
    depth_[root] = 0;

    while (!stack.empty()) {
      auto [v, visited] = stack.top();
      stack.pop();

      if (!visited) {
        stack.push({v, true});

        for (const auto& e : graph_[v]) {
          const long long to = e.to;
          if (to == parent_[v]) continue;

          parent_[to] = v;
          depth_[to] = depth_[v] + 1;
          stack.push({to, false});
        }
      } else {
        long long max_size = 0;

        for (const auto& e : graph_[v]) {
          const long long to = e.to;
          if (to == parent_[v]) continue;

          subtree_size_[v] += subtree_size_[to];

          if (max_size < subtree_size_[to]) {
            max_size = subtree_size_[to];
            heavy_[v] = to;
          }
        }
      }
    }
  }

  void decompose(long long root, long long component, long long& position) {
    std::stack<std::pair<long long, long long>> stack;
    stack.push({root, root});

    while (!stack.empty()) {
      auto [start, path_head] = stack.top();
      stack.pop();

      for (long long v = start; v != -1; v = heavy_[v]) {
        component_[v] = component;
        head_[v] = path_head;
        index_[v] = position;
        vertex_[position] = v;
        ++position;

        for (const auto& e : graph_[v]) {
          const long long to = e.to;

          if (to == parent_[v] || to == heavy_[v]) continue;

          stack.push({to, to});
        }
      }
    }
  }

  /**
   * @brief HLD を構築する
   *
   * ### 計算量
   * $O(V + E)$
   */
  void build() {
    long long position = 0;
    long long component = 0;
    long long edge_position = 0;

    for (long long root = 0; root < n_; ++root) {
      if (parent_[root] != -2) continue;

      build_subtree(root);
      const long long begin = position;
      decompose(root, component, position);
      for (long long i = begin; i < position; ++i) {
        const long long v = vertex_[i];
        if (parent_[v] == -1) {
          edge_index_[v] = edge_position - 1;
        } else {
          edge_index_[v] = edge_position++;
          edge_vertex_[edge_index_[v]] = v;
        }
      }
      ++component;
    }
  }

 public:
  /**
   * @brief HLD を構築する
   *
   * @param graph 対象となる木または森
   */
  explicit HLD(const Graph<T>& graph)
      : graph_(graph),
        n_(graph.size()),
        index_(n_, -1),
        edge_index_(n_, -1),
        edge_vertex_(n_, -1),
        vertex_(n_, -1),
        parent_(n_, -2),
        depth_(n_),
        subtree_size_(n_, 1),
        head_(n_),
        heavy_(n_, -1),
        component_(n_, -1) {
    build();
  }

  /**
   * @brief 頂点 `v` の HLD 上の番号を取得する
   */
  long long index(long long v) const {
    return index_[v];
  }

  long long operator[](long long v) const {
    return index(v);
  }

  /**
   * @brief 辺 `u - v` の HLD 上の番号を取得する
   *
   * @details
   * 辺を、根から遠い側の頂点の HLD 上の位置に対応させる。
   * `u` と `v` は木上で隣接している必要がある。
   *
   * @param u 辺の一方の端点
   * @param v 辺のもう一方の端点
   * @return 辺に対応する HLD 上の番号
   */
  long long edge_index(long long u, long long v) const {
    if (v == parent_[u]) return edge_index_[u];
    assert(u == parent_[v]);
    return edge_index_[v];
  }

  long long operator[](long long u, long long v) const {
    return edge_index(u, v);
  }

  /**
   * @brief HLD 上の番号から元の頂点番号を取得する
   */
  long long vertex(long long index) const {
    return vertex_[index];
  }

  /**
   * @brief HLD 上の番号から元のedgeの両端の番号を取得する
   */
  std::pair<long long, long long> edge(long long index) const {
    const long long v = edge_vertex_[index];
    return {parent_[v], v};
  }

  /**
   * @brief 頂点 `v` の親を取得する
   */
  long long parent(long long v) const {
    return parent_[v];
  }

  /**
   * @brief 頂点 `v` の深さを取得する
   */
  long long depth(long long v) const {
    return depth_[v];
  }

  /**
   * @brief 頂点 `v` の部分木サイズを取得する
   */
  long long subtree_size(long long v) const {
    return subtree_size_[v];
  }

  /**
   * @brief 頂点 `v` が属する Heavy Path の先頭頂点を取得する
   */
  long long head(long long v) const {
    return head_[v];
  }

  /**
   * @brief 頂点 `v` が属する連結成分の番号を取得する
   */
  long long component(long long v) const {
    return component_[v];
  }

  /**
   * @brief 頂点 `v` の部分木に対応する半開区間を取得する
   *
   * @return `[l, r)` を表す `{l, r}`
   */
  std::pair<long long, long long> subtree_range(long long v) const {
    return {index_[v], index_[v] + subtree_size_[v]};
  }

  /**
   * @brief 頂点 `v` の部分木に対応する半開区間を取得する
   *
   * @return `[l, r)` を表す `{l, r}`
   */
  std::pair<long long, long long> subtree_edge_range(long long v) const {
    return {edge_index_[v] + 1, edge_index_[v] + subtree_size_[v]};
  }

  struct PathSegment {
    long long l;
    long long r;
    bool reversed;
  };

  /**
   * @brief パス `u-v` 上の頂点列を HLD 上の区間に分解する
   *
   * @details
   * 戻り値の区間は、パスを `u` から `v` へ辿る順に並ぶ。
   *
   * 各区間は `[l, r)` で表される。
   * `reversed == false` の場合は `l, l+1, ..., r-1` の順、
   * `reversed == true` の場合は `r-1, r-2, ..., l` の順に読む。
   *
   * @param u パスの始点
   * @param v パスの終点
   * @return パスを構成する HLD 上の区間列
   *
   * ### 計算量
   * $O(\log V)$
   */
  std::vector<PathSegment> path_nodes(long long u, long long v) const {
    std::vector<PathSegment> left;
    std::vector<PathSegment> right;

    while (head_[u] != head_[v]) {
      if (depth_[head_[u]] > depth_[head_[v]]) {
        left.push_back({index_[head_[u]], index_[u] + 1, true});
        u = parent_[head_[u]];
      } else {
        right.push_back({index_[head_[v]], index_[v] + 1, false});
        v = parent_[head_[v]];
      }
    }

    if (depth_[u] > depth_[v]) {
      left.push_back({index_[v], index_[u] + 1, true});
    } else {
      right.push_back({index_[u], index_[v] + 1, false});
    }

    std::vector<PathSegment> result;
    result.reserve(left.size() + right.size());

    for (const auto& segment : left) {
      result.push_back(segment);
    }

    for (auto it = right.rbegin(); it != right.rend(); ++it) {
      result.push_back(*it);
    }

    return result;
  }

  /**
   * @brief パス `u-v` 上の辺列を HLD 上の区間に分解する
   *
   * @details
   * 辺 `u - v` を頂点 `edge_index(u, v)` の HLD 上の位置に対応させる。
   *
   * 戻り値の区間は、パスを `u` から `v` へ辿る順に並ぶ。
   *
   * 各区間は `[l, r)` で表される。
   * `reversed == false` の場合は `l, l+1, ..., r-1` の順、
   * `reversed == true` の場合は `r-1, r-2, ..., l` の順に読む。
   *
   * @param u パスの始点
   * @param v パスの終点
   * @return パスを構成する辺に対応する HLD 上の区間列
   *
   * ### 計算量
   * $O(\log V)$
   */
  std::vector<PathSegment> path_edges(long long u, long long v) const {
    std::vector<PathSegment> left;
    std::vector<PathSegment> right;

    while (head_[u] != head_[v]) {
      if (depth_[head_[u]] > depth_[head_[v]]) {
        left.push_back({edge_index_[head_[u]], edge_index_[u] + 1, true});
        u = parent_[head_[u]];
      } else {
        right.push_back({edge_index_[head_[v]], edge_index_[v] + 1, false});
        v = parent_[head_[v]];
      }
    }

    if (depth_[u] > depth_[v]) {
      if (index_[v] + 1 < index_[u] + 1) {
        left.push_back({edge_index_[v] + 1, edge_index_[u] + 1, true});
      }
    } else {
      if (index_[u] + 1 < index_[v] + 1) {
        right.push_back({edge_index_[u] + 1, edge_index_[v] + 1, false});
      }
    }

    std::vector<PathSegment> result;
    result.reserve(left.size() + right.size());

    for (const auto& segment : left) {
      result.push_back(segment);
    }

    for (auto it = right.rbegin(); it != right.rend(); ++it) {
      result.push_back(*it);
    }

    return result;
  }
};