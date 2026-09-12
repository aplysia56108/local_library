#include <queue>
#include <vector>
#include <algorithm>

#include "graph/graph.hpp"

/**
 * @brief ダイクストラ法（単一始点 / 多始点 最短経路アルゴリズム）
 * @tparam T 辺の重み（コスト）の型 (デフォルト: long long)
 */
template <typename T = long long>
struct Dijkstra {
  /// 未到達判定用の無限大（int へキャストされてもオーバーフローしない定数）
  static constexpr T INF = (1LL << 30) + (1LL << 60) - 2;

  const Graph<T>& g;        ///< 対象グラフの参照
  std::vector<T> dist;      ///< 各頂点への最短距離
  std::vector<int> parent;  // 経路復元用 (直前の頂点、存在しない場合は-1)
  std::vector<edge<T>> parent_edge;  // 経路復元用 (直前に通ったedge)

  /**
   * @brief 1始点ダイクストラの構築
   * @param g 対象のグラフ
   * @param start 始点の頂点番号 (0-indexed)
   */
  Dijkstra(const Graph<T>& g, int start)
      : g(g), dist(g.size(), INF), parent(g.size(), -1), parent_edge(g.size()) {
    build(std::vector<int>{start});
  }

  /**
   * @brief 多始点ダイクストラの構築
   * @param g 対象のグラフ
   * @param starts 始点となる頂点番号のリスト
   */
  Dijkstra(const Graph<T>& g, const std::vector<int>& starts)
      : g(g), dist(g.size(), INF), parent(g.size(), -1), parent_edge(g.size()) {
    build(starts);
  }

  /**
   * @brief 指定頂点までの最短コストを取得する
   * @param to 目的地の頂点番号
   * @return T 最短コスト（到達不能な場合は INF）
   */
  T get_cost(int to) const {
    return dist[to];
  }
  /**
   * @brief 指定頂点までの最短コストを取得する
   * @param to 目的地の頂点番号
   * @return T 最短コスト（到達不能な場合は INF）
   */
  T operator[](int to) const {
    return dist[to];
  }

  /**
   * @brief 指定頂点へ到達可能かどうかを判定する
   * @param to 目的地の頂点番号
   * @return true 到達可能 / false 到達不能
   */
  bool is_reachable(int to) const {
    return dist[to] != INF;
  }

  /**
   * @brief 始点から指定頂点までの最短経路（頂点列）を復元する
   * @param to 目的地の頂点番号
   * @return std::vector<int> 頂点番号のリスト (到達不能な場合は空配列)
   */
  std::vector<int> get_path(int to) const {
    if (!is_reachable(to)) return {};
    std::vector<int> path;
    for (int v = to; v != -1; v = parent[v]) {
      path.push_back(v);
    }
    std::reverse(path.begin(), path.end());
    return path;
  }

  /**
   * @brief 始点から指定頂点までの最短経路（辺構造体列）を復元する
   * @param to 目的地の頂点番号
   * @return std::vector<edge<T>> 通過した辺のリスト (到達不能な場合は空配列)
   */
  std::vector<edge<T>> get_edge_path(int to) const {
    if (!is_reachable(to)) return {};
    std::vector<edge<T>> path;
    for (int v = to; parent[v] != -1; v = parent[v]) {
      path.push_back(parent_edge[v]);
    }
    std::reverse(path.begin(), path.end());
    return path;
  }

 private:
  void build(const std::vector<int>& starts) {
    using P = std::pair<T, int>;  // {cost, v}
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;

    for (int s : starts) {
      dist[s] = 0;
      pq.emplace(0, s);
    }

    while (!pq.empty()) {
      auto [d, v] = pq.top();
      pq.pop();

      if (d > dist[v]) continue;

      for (const auto& e : g[v]) {
        if (dist[e.to] > dist[v] + e.cost) {
          dist[e.to] = dist[v] + e.cost;
          parent[e.to] = v;
          parent_edge[e.to] = e;
          pq.emplace(dist[e.to], e.to);
        }
      }
    }
  }
};