#pragma once

#include "data-structure/btree/components/bplustree-aggregate.hpp"
#include "common/monoid.hpp"

/**
 * @brief 1要素1Leaf の B+tree による動的セグメント木。
 * @tparam Monoid value_type、op(a,b)、e() を持つモノイド。
 * @tparam NodeCapacity Internal の子 ID 配列容量。
 * @details
 * 区間積 prod / reverse_prod、全体積 all_prod / all_reverse_prod、
 * 境界探索 max_right / min_left、位置指定の insert / erase、
 * split / merge / reverse に対応する。
 * 非可換モノイドでも逆方向積を別に保持するため区間反転に対応する。
 * get / operator[] / iterator は読み取り専用。set と
 * operator()(index) の Setter を通して更新する。
 *
 * ### 計算量
 * 区間積・境界探索・点更新は O(NodeCapacity * 木の高さ)
 * （モノイド演算・述語は O(1) を仮定）。
 * 全体積と全体 reverse は O(1)。
 */
template <class Monoid, std::size_t NodeCapacity = 64>
class Bplustree_Segtree
    : public Bplustree::AggregateTree<Bplustree_Segtree<Monoid, NodeCapacity>,
                                      Monoid, Bplustree::NoAction, false,
                                      NodeCapacity> {
  using Parent =
      Bplustree::AggregateTree<Bplustree_Segtree, Monoid, Bplustree::NoAction,
                               false, NodeCapacity>;

 public:
  using Parent::Parent;
};
using SumSegtree = Bplustree_Segtree<SumMonoid>;
using MinSegtree = Bplustree_Segtree<MinMonoid>;
using MaxSegtree = Bplustree_Segtree<MaxMonoid>;
using GcdSegtree = Bplustree_Segtree<GcdMonoid>;
template <class mint>
using AffineSegtree = Bplustree_Segtree<AffineMonoid<mint>>;

/** @brief 関数 Op（結合演算）と E（単位元生成）から構築する別名。 */
template <class T, auto Op, auto E, std::size_t NodeCapacity = 64>
using BSegtree = Bplustree_Segtree<Monoid<T, Op, E>, NodeCapacity>;

/**
 * @brief 単位元 E を指定しないモノイドの B+tree Segtree。
 * @details MonoidWithoutE によって内部で人工的な単位元を扱う。
 * 実要素の公開値型は T。
 */
template <class T, auto Op, std::size_t NodeCapacity = 64>
using BSegtreeWithoutE = Bplustree_Segtree<MonoidWithoutE<T, Op>, NodeCapacity>;
