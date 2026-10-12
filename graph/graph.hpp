#include <iostream>
#include <string>
#include <vector>

/**
 * @brief グラフの辺を表す構造体
 * @tparam T 辺の重み（コスト）の型 (デフォルト: long long)
 */
template <typename T = long long>
struct edge {
  long long from;  ///< 遷移元の頂点番号 (0-indexed)
  long long to;    ///< 遷移先の頂点番号 (0-indexed)
  T cost;          ///< 辺の重み（コスト）
  long long id;    ///< 辺のID (0-indexed)
  edge() : from(-1), to(-1), cost(1), id(-1) {}
  edge(long long from, long long to, T cost = 1, long long id = -1)
      : from(from), to(to), cost(cost), id(id) {}
  bool operator<(const edge& other) const {
    if (cost != other.cost) return cost < other.cost;
    if (to != other.to) return to < other.to;
    return from < other.from;
  }
  bool operator>(const edge& other) const {
    if (cost != other.cost) return cost > other.cost;
    if (to != other.to) return to > other.to;
    return from > other.from;
  }
};

/**
 * @brief 汎用グラフ構造体（隣接リスト形式）
 * @tparam T 辺の重み（コスト）の型 (デフォルト: long long)
 */
template <typename T = long long>
struct Graph {
  std::vector<std::vector<edge<T>>> G;  ///< 隣接リスト
  long long n;                          ///< 頂点数
  long long m;                          ///< 辺数（無向辺は1本としてカウント）
  explicit Graph(long long n) : G(n), n(n), m(0) {}

  /**
   * @brief 辺を追加する
   * @param u 遷移元頂点
   * @param v 遷移先頂点
   * @param cost 辺のコスト (デフォルト: 1)
   * @param is_bidirected 無向辺（両方向）の場合は true (デフォルト: true)
   * @param id 辺ID (-1 の場合は内部で自動割り振りの m を使用)
   */
  void add_edge(long long u, long long v, T cost = 1, bool is_bidirected = true,
                long long id = -1) {
    if (id < 0) id = m;
    G[u].push_back({u, v, cost, id});
    if (is_bidirected) {
      G[v].push_back({v, u, cost, id});
    }
    m++;
  }

  /// 添字アクセス（const 版）: g[v] で頂点 v の出辺リストを取得
  const std::vector<edge<T>>& operator[](long long v) const {
    return G[v];
  }

  /// 添字アクセス: g[v] で頂点 v の出辺リストを取得
  std::vector<edge<T>>& operator[](long long v) {
    return G[v];
  }

  /// 頂点数を取得
  long long size() const {
    return n;
  }

  /// グラフが空（頂点数 0）かどうかを判定
  bool empty() const {
    return n == 0;
  }
};

// クラス外で演算子をオーバーロードする
template <typename T>
inline std::ostream& operator<<(std::ostream& os, const edge<T>& e) {
  return os << "{" << e.from << "->" << e.to << ", cost:" << e.cost
            << ", id:" << e.id << "}";
}

// --------------------------------------------------
// 入力補助ユーティリティ
// --------------------------------------------------

/**
 * @brief 2次元座標 (r, c) を 1次元の頂点 ID に変換する
 * @param r 行インデックス (0-indexed)
 * @param c 列インデックス (0-indexed)
 * @param w グリッドの幅 (列数)
 * @return long long 1次元頂点 ID (r * w + c)
 */
inline long long grid_to_id(long long r, long long c, long long w) {
  return static_cast<long long>(r) * w + c;
}

/**
 * @brief 1次元の頂点 ID を 2次元座標 (r, c) に逆変換する
 * @param id 1次元頂点 ID
 * @param w グリッドの幅 (列数)
 * @return std::pair<long long, long long> {r, c} のペア
 */
inline std::pair<long long, long long> id_to_grid(long long id, long long w) {
  return {id / w, id % w};
}

/**
 * @brief 重みなしグラフの標準入力を受け取る (1-indexed 入力を自動で 0-indexed
 * に変換)
 * @details 入力形式:
 *   a_1 b_1
 *   a_2 b_2
 *   ...
 *   a_m b_m
 * @param n 頂点数
 * @param m 辺数 (-1 の場合は n - 1 本の木構造として処理)
 * @param is_bidirected 無向グラフの場合は true, 有向グラフの場合は false
 * @return Graph<long long> 構築されたグラフ
 */
inline Graph<long long> read_graph(long long n, long long m = -1,
                                   bool is_bidirected = true) {
  if (m == -1) m = n - 1;
  Graph<long long> g(n);
  for (long long i = 0; i < m; i++) {
    long long u, v;
    std::cin >> u >> v;
    u--;
    v--;
    g.add_edge(u, v, 1, is_bidirected);
  }
  return g;
}

/**
 * @brief 重み付きグラフの標準入力を受け取る (1-indexed 入力を自動で 0-indexed
 * に変換)
 * @details 入力形式:
 *   a_1 b_1 c_1
 *   a_2 b_2 c_2
 *   ...
 *   a_m b_m c_m
 * @tparam T 辺のコストの型 (デフォルト: long long)
 * @param n 頂点数
 * @param m 辺数 (-1 の場合は n - 1 本の木構造として処理)
 * @param is_bidirected 無向グラフの場合は true, 有向グラフの場合は false
 * @return Graph<T> 構築されたグラフ
 */
template <typename T = long long>
inline Graph<T> read_weighted_graph(long long n, long long m = -1,
                                    bool is_bidirected = true) {
  if (m == -1) m = n - 1;
  Graph<T> g(n);
  for (long long i = 0; i < m; i++) {
    long long u, v;
    T w;
    std::cin >> u >> v >> w;
    u--;
    v--;
    g.add_edge(u, v, w, is_bidirected);
  }
  return g;
}

/**
 * @brief グリッド文字列から 4 方向移動の有向グラフを構築する (頂点 ID = r * w +
 * c)
 * @param grid グリッド文字列配列 (例: {"...#...", "..S....", "#####.."})
 * @param wall 通行不能を表す文字 (デフォルト: '#')
 * @return Graph<long long> 頂点数 H*W のグラフ
 */
inline Graph<long long> read_grid(const std::vector<std::string>& grid,
                                  char wall = '#') {
  long long h = grid.size();
  long long w = grid[0].size();
  Graph<long long> g(h * w);
  long long dx[] = {1, 0, -1, 0};
  long long dy[] = {0, 1, 0, -1};

  for (long long r = 0; r < h; r++) {
    for (long long c = 0; c < w; c++) {
      if (grid[r][c] == wall) continue;
      for (long long d = 0; d < 4; d++) {
        long long nr = r + dx[d];
        long long nc = c + dy[d];
        if (0 <= nr && nr < h && 0 <= nc && nc < w) {
          if (grid[nr][nc] != wall) {
            g.add_edge(grid_to_id(r, c, w), grid_to_id(nr, nc, w), 1, false);
          }
        }
      }
    }
  }
  return g;
}

/**
 * @brief グリッド内から指定した文字 (S, T 等) の座標 (r, c) を検索する
 * @param grid グリッド文字列配列
 * @param target 検索対象の文字 (デフォルト: 'S')
 * @return std::pair<long long, long long> 発見した場合は {r, c},
 * 存在しない場合は {-1, -1}
 */
inline long long find_in_grid(const std::vector<std::string>& grid,
                              char target = 'S') {
  long long h = grid.size();
  long long w = grid[0].size();
  for (long long r = 0; r < h; r++) {
    for (long long c = 0; c < w; c++) {
      if (grid[r][c] == target) return grid_to_id(r, c, w);
    }
  }

  return -1;
}
