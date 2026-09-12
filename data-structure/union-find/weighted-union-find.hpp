#pragma once

#include <numeric>
#include <utility>
#include <vector>

/**
 * @brief 重み付き Union-Find の矛盾処理方針
 */
enum class WeightedUnionFindConflictPolicy {
  Ignore,  ///< 矛盾する制約を無視する
  Record   ///< 矛盾した連結成分を記録する
};

/**
 * @brief ポテンシャル差を管理する重み付き Union-Find
 * @tparam T
 * ポテンシャル差を表す型。加減算、単項マイナス、等値比較が可能であること。
 * @details
 * 各頂点 x にポテンシャル potential(x)
 * を考え、同じ連結成分に属する頂点間の差を管理する。 `merge(x, y, w)`
 * は次の制約を追加する。
 *
 *     potential(y) - potential(x) = w
 *
 * 例えば、おもり x の重さを potential(x) とすると、`merge(1, 2, 5)` は
 * 「おもり 2 はおもり 1 より 5 重い」という意味になる。
 *
 * 矛盾処理方針は構築時に Ignore または Record から選択する。
 *
 * - Ignore: 矛盾する制約を無視し、既存のポテンシャル差を維持する。
 * - Record: 矛盾した連結成分を記録し、その後の併合でも矛盾状態を引き継ぐ。
 *
 * Record モードで矛盾した連結成分では、ポテンシャル差は定義されない。
 * ただし、連結性は引き続き管理する。
 *
 * 経路圧縮と union by rank を使用する。
 *
 * ### 計算量
 *
 * 構築は O(N)、各操作は償却 O(α(N))。α は逆 Ackermann 関数。
 */
template <typename T = long long>
class WeightedUnionFind {
 public:
  /**
   * @brief 頂点数と矛盾処理方針を指定して構築する
   * @param node_size 頂点数
   * @param policy 矛盾処理方針。デフォルトは Ignore。
   * @details
   * 頂点番号は 0 以上 node_size 未満とする。
   * 初期状態ではすべての頂点が独立した連結成分に属し、矛盾は存在しない。
   *
   * ### 計算量
   *
   * O(N)
   */
  explicit WeightedUnionFind(int node_size,
                             WeightedUnionFindConflictPolicy policy =
                                 WeightedUnionFindConflictPolicy::Ignore)
      : policy_(policy),
        parent_(node_size),
        rank_(node_size, 0),
        diff_weight_(node_size, T{}),
        conflict_(node_size, false) {
    std::iota(parent_.begin(), parent_.end(), 0);
  }

  /**
   * @brief 頂点 x の属する連結成分の代表元を返す
   * @param x 頂点番号
   * @return 頂点 x の代表元
   * @details
   * 経路圧縮を行い、diff_weight_[x] を代表元からのポテンシャル差に更新する。
   *
   * ### 計算量
   *
   * 償却 O(α(N))
   */
  int leader(int x) {
    if (parent_[x] == x) {
      return x;
    }

    int p = parent_[x];
    parent_[x] = leader(p);
    diff_weight_[x] += diff_weight_[p];

    return parent_[x];
  }

  /**
   * @brief 代表元から頂点 x へのポテンシャル差を返す
   * @param x 頂点番号
   * @return potential(x) - potential(leader(x))
   * @details
   * Record モードで連結成分が矛盾している場合、返り値に意味はない。
   *
   * ### 計算量
   *
   * 償却 O(α(N))
   */
  T weight(int x) {
    leader(x);
    return diff_weight_[x];
  }

  /**
   * @brief ポテンシャル差の制約を追加する
   * @param x 頂点番号
   * @param y 頂点番号
   * @param w potential(y) - potential(x)
   * @return 制約に矛盾がなければ true、矛盾があれば false
   * @details
   * 次の制約を追加する。
   *
   *     potential(y) - potential(x) = w
   *
   * x と y が異なる連結成分に属する場合は、それらを併合する。
   * 同じ連結成分に属する場合は、既存のポテンシャル差と w
   * が一致するかを判定する。
   *
   * **Ignore モード**
   *
   * 新しい制約が既存の制約と矛盾する場合、その制約を無視して false を返す。
   * 既存のポテンシャル差や連結成分は変更されない。
   *
   * **Record モード**
   *
   * 新しい制約が矛盾する場合、その連結成分を矛盾状態にして false を返す。
   * すでに矛盾している連結成分を含む場合も false を返す。
   * 異なる連結成分であれば、矛盾状態に関係なく併合を行い、
   * 併合後の連結成分に矛盾状態を引き継ぐ。
   *
   * 矛盾した連結成分でも内部のポテンシャル差は管理し続けるが、
   * それらの値は制約全体を満たすポテンシャル差を表すものではない。
   *
   * いずれのモードでも、判定に伴う経路圧縮は行われる。
   *
   * ### 計算量
   *
   * 償却 O(α(N))
   */
  bool merge(int x, int y, T w) {
    int root_x = leader(x);
    int root_y = leader(y);

    // potential(root_y) - potential(root_x)
    w += diff_weight_[x];
    w -= diff_weight_[y];

    if (root_x == root_y) {
      if (conflict_[root_x]) {
        return false;
      }

      if (w == T{}) {
        return true;
      }

      if (policy_ == WeightedUnionFindConflictPolicy::Record) {
        conflict_[root_x] = true;
      }

      return false;
    }

    if (rank_[root_x] < rank_[root_y]) {
      std::swap(root_x, root_y);
      w = -w;
    }

    if (rank_[root_x] == rank_[root_y]) {
      ++rank_[root_x];
    }

    parent_[root_y] = root_x;
    diff_weight_[root_y] = w;

    if (policy_ == WeightedUnionFindConflictPolicy::Record) {
      conflict_[root_x] = conflict_[root_x] || conflict_[root_y];
    }

    return !conflict_[root_x];
  }

  /**
   * @brief 2 頂点が同じ連結成分に属するか判定する
   * @param x 頂点番号
   * @param y 頂点番号
   * @return 同じ連結成分に属する場合は true、そうでなければ false
   * @details
   * Record モードでは、矛盾した連結成分でも連結性を判定できる。
   *
   * ### 計算量
   *
   * 償却 O(α(N))
   */
  bool same(int x, int y) {
    return leader(x) == leader(y);
  }

  /**
   * @brief 頂点 x の属する連結成分が整合しているか判定する
   * @param x 頂点番号
   * @return 矛盾がなければ true、矛盾があれば false
   * @details
   * Ignore モードでは常に true を返す。
   *
   * Record モードでは、その連結成分に追加されたすべての制約を
   * 同時に満たすポテンシャルの割り当てが存在する場合に true を返す。
   * 一度矛盾した連結成分は、その後の併合でも矛盾状態を引き継ぐ。
   *
   * ### 計算量
   *
   * 償却 O(α(N))
   */
  bool consistent(int x) {
    return !conflict_[leader(x)];
  }

  /**
   * @brief 2 頂点間のポテンシャル差を返す
   * @param x 頂点番号
   * @param y 頂点番号
   * @return potential(y) - potential(x)
   * @pre x と y が同じ連結成分に属していること。
   * @pre Record モードでは、その連結成分が整合していること。
   * @details
   * 前提条件を満たさない場合、返り値に意味はない。
   * 必要に応じて `same(x, y)` と `consistent(x)` を確認すること。
   *
   * ### 計算量
   *
   * 償却 O(α(N))
   */
  T diff(int x, int y) {
    return weight(y) - weight(x);
  }

 private:
  /** @brief 矛盾処理方針 */
  WeightedUnionFindConflictPolicy policy_;

  /** @brief 各頂点の親。代表元では parent_[x] == x。 */
  std::vector<int> parent_;

  /** @brief union by rank に使用する各代表元の rank。 */
  std::vector<int> rank_;

  /**
   * @brief 親頂点からのポテンシャル差
   * @details
   * diff_weight_[x] は次の値を表す。
   *
   *     potential(x) - potential(parent_[x])
   *
   * 代表元の値は 0。leader(x) の実行後は経路圧縮により、
   * 代表元からのポテンシャル差となる。
   *
   * 矛盾した連結成分では、これらは内部管理用の値となる。
   */
  std::vector<T> diff_weight_;

  /**
   * @brief 各代表元の連結成分が矛盾しているか
   * @details
   * conflict_[leader(x)] が true の場合、その連結成分は矛盾している。
   * Ignore モードではすべて false のままとなる。
   */
  std::vector<bool> conflict_;
};