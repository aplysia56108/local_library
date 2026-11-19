#pragma once

#include "data-structure/btree/components/bplustree-aggregate.hpp"
#include "common/monoid.hpp"

/**
 * @brief 1要素1Leaf の B+tree による、反転にも対応した遅延セグメント木。
 * @tparam Monoid value_type、op、e を提供するモノイド。
 * @tparam Action action_type、mapping、composition を提供する作用。
 * @tparam ReverseInvariant 積が反転後も不変な場合に限り true。
 * @tparam NodeCapacity Internal の子 ID 配列容量（この型のデフォルトは15）。
 * @details
 * composition(f,g) は g の後に f を適用し、
 * mapping(f,x,size) は長さ size の区間積 x に作用する。
 * Internal は遅延作用と順・逆方向の積を管理し、Leaf は作用を受けた時点で
 * 値を直接更新する。作用の単位元や作用同士の等価比較は必須ではない。
 * get / operator[] / iterator は読み取り専用で、set / Setter で点更新する。
 * apply(l,r,f) / all_apply(f)、区間積・境界探索・反転・分割・結合に対応。
 *
 * ### 計算量
 * 区間作用・区間積・境界探索は O(NodeCapacity * 木の高さ)
 * （モノイド演算・作用・述語は O(1) を仮定）。
 * all_prod / all_reverse_prod と全体 reverse は O(1)。
 */
template <class Monoid, class Action, bool ReverseInvariant = false,
          std::size_t NodeCapacity = 15>
class Bplustree_LazySegtree
    : public Bplustree::AggregateTree<
          Bplustree_LazySegtree<Monoid, Action, ReverseInvariant, NodeCapacity>,
          Monoid, Action, ReverseInvariant, NodeCapacity> {
  using Parent = Bplustree::AggregateTree<Bplustree_LazySegtree, Monoid, Action,
                                          ReverseInvariant, NodeCapacity>;
 public:
  using Parent::Parent;
};
/**
 * @brief モノイドの値 T へ作用 F を適用するための関数オブジェクト束。
 * @tparam Mapping mapping(f,x,size) を実装する callable。
 * @tparam Composition composition(f,g) を実装する callable。
 * @details Composition(f,g) は g の後に f を実行する。
 * Mapping は長さ size の区間積を正しく写像する必要がある。
 */
template <class T, class F, auto Mapping, auto Composition>
struct Action {
  using action_type = F;

  static T mapping(const F& f, const T& x, std::size_t size) {
    return Mapping(f, x, size);
  }

  static F composition(const F& f, const F& g) {
    return Composition(f, g);
  }
};

/**
 * @brief 関数 Op / E と作用 Mapping / Composition を直接指定する LazySegtree。
 * @details この別名の NodeCapacity デフォルトは64。 
 */
template <class T, class F, auto Op, auto E, auto Mapping, auto Composition,
          bool ReverseInvariant = false, std::size_t NodeCapacity = 64>
using BLazySegtree =
    Bplustree_LazySegtree<Monoid<T, Op, E>, Action<T, F, Mapping, Composition>,
                          ReverseInvariant, NodeCapacity>;

/**
 * @brief 単位元 E を指定しない LazySegtree の別名。
 * @details MonoidWithoutE を使い、実要素は T として保持する。
 * この別名の NodeCapacity デフォルトは15。
 */
template <class T, class F, auto Op, auto Mapping, auto Composition,
          bool ReverseInvariant = false, std::size_t NodeCapacity = 15>
using BLazySegtreeWithoutE =
    Bplustree_LazySegtree<MonoidWithoutE<T, Op>,
                          Action<T, F, Mapping, Composition>, ReverseInvariant,
                          NodeCapacity>;

/** @brief 単点値（最小・最大値など）へ加算する写像。size==0 は変更しない。 */
inline constexpr auto AddMapping = [](const long long& f, const long long& x,
                                      std::size_t size) {
  return size == 0 ? x : x + f;
};

/** @brief 単点値（最小・最大値など）を置換する写像。size==0 は変更しない。 */
inline constexpr auto ChangeMapping = [](const long long& f, const long long& x,
                                         std::size_t size) {
  return size == 0 ? x : f;
};

/** @brief 区間和へ size * f を加える写像。 */
inline constexpr auto AddSumMapping = [](const long long& f, const long long& x,
                                         std::size_t size) {
  return x + f * static_cast<long long>(size);
};

/** @brief 区間和を size * f へ置き換える写像。 */
inline constexpr auto ChangeSumMapping =
    [](const long long& f, const long long&, std::size_t size) {
      return f * static_cast<long long>(size);
    };

/** @brief 2つの加算作用の合成。 */
inline constexpr auto AddComposition = [](const long long& f,
                                          const long long& g) {
  return f + g;
};

/** @brief 後から適用する置換作用 f が前の作用 g を上書きする。 */
inline constexpr auto ChangeComposition = [](const long long& f,
                                             const long long&) {
  return f;
};

using AddMinLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return std::min(a, b);
                 },
                 []() {
                   return library_inf;
                 },
                 AddMapping, AddComposition, true>;

using AddMaxLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return std::max(a, b);
                 },
                 []() {
                   return -library_inf;
                 },
                 AddMapping, AddComposition, true>;

using ChangeMinLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return std::min(a, b);
                 },
                 []() {
                   return library_inf;
                 },
                 ChangeMapping, ChangeComposition, true>;

using ChangeMaxLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return std::max(a, b);
                 },
                 []() {
                   return -library_inf;
                 },
                 ChangeMapping, ChangeComposition, true>;

using AddSumLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return a + b;
                 },
                 []() {
                   return 0LL;
                 },
                 AddSumMapping, AddComposition, true>;

using ChangeSumLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return a + b;
                 },
                 []() {
                   return 0LL;
                 },
                 ChangeSumMapping, ChangeComposition, true>;

template <class mint, std::size_t NodeCapacity = 15>
using AffineSumLazySegtree =
    BLazySegtree<mint, std::pair<mint, mint>,
                 [](auto a, auto b) {
                   return a + b;
                 },
                 []() {
                   return 0;
                 },
                 [](auto f, auto x, std::size_t size) {
                   return f.first * x + size * f.second;
                 },
                 [](auto f, auto g) -> std::pair<mint, mint> {
                   return {f.first * g.first, f.first * g.second + f.second};
                 },
                 true, NodeCapacity>;
