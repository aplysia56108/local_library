#include "graph/hld.hpp"
#include "debug"
#include <cassert>
#include <iostream>
#include <queue>
#include <random>
#include <vector>

constexpr long long MOD = 998244353;

struct Affine {
  long long a;
  long long b;

  bool operator==(const Affine&) const = default;
};

/**
 * @brief left の後に right を適用した合成を返す
 *
 * left(x) = ax + b, right(x) = cx + d のとき
 * right(left(x)) を返す。
 */
Affine compose(const Affine& left, const Affine& right) {
  return {
      left.a * right.a % MOD,
      (right.a * left.b + right.b) % MOD,
  };
}

void test_hld_edge() {
  constexpr long long N = 1000;
  constexpr long long TREE_COUNT = 10;
  constexpr long long TREE_SIZE = N / TREE_COUNT;
  constexpr long long PATH_QUERY_COUNT = 1000;
  constexpr long long TEST_COUNT = 100;

  std::mt19937_64 rng(123456789);

  for (long long test = 0; test < TEST_COUNT; ++test) {
    Graph<> g(N);

    // 生成時の親。HLD の root の選び方と一致するよう、
    // 各成分の最小頂点を root にする。
    std::vector<long long> parent(N, -1);
    std::vector<long long> component(N);

    for (long long c = 0; c < TREE_COUNT; ++c) {
      const long long root = c * TREE_SIZE;

      component[root] = c;

      for (long long v = root + 1; v < root + TREE_SIZE; ++v) {
        const long long p = root + static_cast<long long>(rng() % (v - root));

        parent[v] = p;
        component[v] = c;
        g.add_edge(p, v, 1, 1);
      }
    }

    HLD hld(g);

    // ------------------------------------------------------------
    // edge_index / edge の確認
    // ------------------------------------------------------------

    for (long long v = 0; v < N; ++v) {
      if (parent[v] == -1) continue;

      const long long p = parent[v];
      const long long index = hld.edge_index(p, v);

      auto [x, y] = hld.edge(index);

      assert((x == p && y == v));
      assert(hld.edge_index(v, p) == index);
    }

    // ------------------------------------------------------------
    // subtree_edge_range の確認
    // ------------------------------------------------------------

    std::vector<long long> edge_value(N - TREE_COUNT);
    std::vector<long long> value_to_parent(N);

    for (long long v = 0; v < N; ++v) {
      if (parent[v] == -1) continue;

      const long long value = static_cast<long long>(rng() % 1000000);

      value_to_parent[v] = value;
      edge_value[hld.edge_index(parent[v], v)] = value;
    }

    auto dfs_subtree_sum = [&](auto&& self, long long v,
                               long long p) -> long long {
      long long result = 0;

      for (const auto& e : g[v]) {
        const long long to = e.to;
        if (to == p) continue;

        result += value_to_parent[to];
        result += self(self, to, v);
      }

      return result;
    };

    for (long long v = 0; v < N; ++v) {
      auto [l, r] = hld.subtree_edge_range(v);

      long long hld_sum = 0;
      for (long long i = l; i < r; ++i) {
        hld_sum += edge_value[i];
      }

      const long long naive_sum =
          dfs_subtree_sum(dfs_subtree_sum, v, parent[v]);

      if (hld_sum != naive_sum) {
        std::cerr << "subtree_edge_range failed\n";
        std::cerr << "test = " << test << '\n';
        std::cerr << "v = " << v << '\n';
        std::cerr << "[l, r) = [" << l << ", " << r << ")\n";
        std::cerr << "hld = " << hld_sum << '\n';
        std::cerr << "naive = " << naive_sum << '\n';
        assert(false);
      }
    }

    // ------------------------------------------------------------
    // path_edges の確認
    // ------------------------------------------------------------

    std::vector<Affine> edge_affine(N - TREE_COUNT);
    std::vector<Affine> affine_to_parent(N, {1, 0});

    for (long long v = 0; v < N; ++v) {
      if (parent[v] == -1) continue;

      Affine f = {
          static_cast<long long>(rng() % MOD),
          static_cast<long long>(rng() % MOD),
      };

      affine_to_parent[v] = f;
      edge_affine[hld.edge_index(parent[v], v)] = f;
    }

    for (long long query = 0; query < PATH_QUERY_COUNT; ++query) {
      const long long c = rng() % TREE_COUNT;
      const long long begin = c * TREE_SIZE;

      const long long u = begin + rng() % TREE_SIZE;
      const long long v = begin + rng() % TREE_SIZE;

      // ----- HLD -----

      Affine hld_result{1, 0};

      for (auto [l, r, reversed] : hld.path_edges(u, v)) {
        if (!reversed) {
          for (long long i = l; i < r; ++i) {
            hld_result = compose(hld_result, edge_affine[i]);
          }
        } else {
          for (long long i = r; i > l; --i) {
            hld_result = compose(hld_result, edge_affine[i - 1]);
          }
        }
      }

      // ----- 愚直に元グラフ上で u -> v のパスを復元 -----

      std::vector<long long> prev(N, -1);
      std::queue<long long> que;

      prev[u] = u;
      que.push(u);

      while (!que.empty()) {
        const long long x = que.front();
        que.pop();

        if (x == v) break;

        for (const auto& e : g[x]) {
          const long long to = e.to;
          if (prev[to] != -1) continue;

          prev[to] = x;
          que.push(to);
        }
      }

      std::vector<long long> path;
      for (long long x = v; x != u; x = prev[x]) {
        path.push_back(x);
      }
      path.push_back(u);

      std::reverse(path.begin(), path.end());

      Affine naive_result{1, 0};

      for (long long i = 0; i + 1 < static_cast<long long>(path.size()); ++i) {
        const long long x = path[i];
        const long long y = path[i + 1];

        // x-y のうち根から遠い方を取得。
        const long long child = parent[x] == y ? x : y;

        naive_result = compose(naive_result, affine_to_parent[child]);
      }

      if (!(hld_result == naive_result)) {
        std::cerr << "path_edges failed\n";
        std::cerr << "test = " << test << '\n';
        std::cerr << "query = " << query << '\n';
        std::cerr << "u = " << u << ", v = " << v << '\n';
        std::cerr << "hld = {" << hld_result.a << ", " << hld_result.b << "}\n";
        std::cerr << "naive = {" << naive_result.a << ", " << naive_result.b
                  << "}\n";

        std::cerr << "segments:\n";
        for (auto [l, r, reversed] : hld.path_edges(u, v)) {
          std::cerr << "  [" << l << ", " << r << ") "
                    << (reversed ? "reverse" : "forward") << '\n';
        }

        assert(false);
      }
    }
  }

  std::cerr << "All HLD edge tests passed.\n";
}

int main() {
  test_hld_edge();
}