#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ここにヘッダーの内容が含まれている前提
#include "graph/graph.hpp"
#include "utility/all"

void test_graph_basic() {
  Graph<long long> g(3);
  g.add_edge(0, 1, 10, true);   // 無向辺 (0 <-> 1)
  g.add_edge(1, 2, 20, false);  // 有向辺 (1 -> 2)

  assert(g.size() == 3);
  assert(!g.empty());
  assert(g.m == 2);

  // operator[] のテスト
  assert(g[0].size() == 1);
  assert(g[0][0].to == 1 && g[0][0].cost == 10);

  assert(g[1].size() == 2);  // 0 からの無向辺 + 2 への有向辺
  assert(g[2].size() == 0);

  std::cout << "grid basic tests passed successfully!" << std::endl;
}

void test_read_graph() {
  // 1. 無向グラフの入力シミュレーション (1-indexed)
  // 3 頂点 2 辺: (1-2), (2-3)
  std::string input_unweighted = "1 2\n2 3\n";
  std::stringstream ss_unweighted(input_unweighted);

  // std::cin の参照先を一時的に stringstream に差し替え
  auto cin_buf = std::cin.rdbuf();
  std::cin.rdbuf(ss_unweighted.rdbuf());

  Graph<long long> g1 = read_graph(3, 2, true);

  // cin の参照先を元に戻す
  std::cin.rdbuf(cin_buf);

  // 検証 (0-indexed に変換されているか)
  assert(g1.size() == 3);
  assert(g1.m == 2);
  assert(g1[0].size() == 1 && g1[0][0].to == 1);  // 0 -> 1
  assert(g1[1].size() == 2);                      // 1 -> 0, 1 -> 2
  assert(g1[2].size() == 1 && g1[2][0].to == 1);  // 2 -> 1

  // 2. 重み付き有向グラフの入力シミュレーション (1-indexed)
  // 3 頂点 2 辺: (1->2, cost=10), (2->3, cost=20)
  std::string input_weighted = "1 2 10\n2 3 20\n";
  std::stringstream ss_weighted(input_weighted);

  std::cin.rdbuf(ss_weighted.rdbuf());

  Graph<long long> g2 = read_weighted_graph<long long>(3, 2, false);

  std::cin.rdbuf(cin_buf);

  // 検証
  assert(g2.size() == 3);
  assert(g2.m == 2);
  assert(g2[0].size() == 1 && g2[0][0].to == 1 && g2[0][0].cost == 10);
  assert(g2[1].size() == 1 && g2[1][0].to == 2 && g2[1][0].cost == 20);
  assert(g2[2].empty());  // 有向グラフなので 2 から出る辺はなし

  std::cout << "read graph tests passed successfully!" << std::endl;
}

void test_grid_utility() {
  int r = 2, c = 3, w = 5;
  long long id = grid_to_id(r, c, w);
  assert(id == 13);  // 2 * 5 + 3 = 13

  auto [res_r, res_c] = id_to_grid(id, w);
  assert(res_r == 2 && res_c == 3);

  std::vector<std::string> grid = {".S.", ".#.", "..T"};

  long long start_id = find_in_grid(grid, 'S');
  long long target_id = find_in_grid(grid, 'T');
  long long wall_id = find_in_grid(grid, '#');
  long long none_id = find_in_grid(grid, 'X');

  assert(start_id == grid_to_id(0, 1, 3));   // 1
  assert(target_id == grid_to_id(2, 2, 3));  // 8
  assert(wall_id == grid_to_id(1, 1, 3));    // 4
  assert(none_id == -1);

  // グリッドグラフ構築のテスト
  Graph<long long> g_grid = read_grid(grid, '#');
  assert(g_grid.size() == 9);

  // 壁 (1, 1) から出ている辺は 0 本
  assert(g_grid[wall_id].empty());

  // スタート 'S' (0, 1) からは 上(範囲外)・下(壁) に行けないため、左(0, 0) と
  // 右(0, 2) のみ
  assert(g_grid[start_id].size() == 2);

  std::cout << "grid utility tests passed successfully!" << std::endl;
}

int main() {
  test_graph_basic();
  test_grid_utility();
  test_read_graph();
  std::cout << "All tests passed successfully!" << std::endl;
  return 0;
}