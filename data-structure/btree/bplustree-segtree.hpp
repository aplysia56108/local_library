#pragma once

#include "data-structure/btree/components/bplustree-aggregate.hpp"
#include "common/monoid.hpp"

/**
 * @brief 1要素1Leafの B+ tree 上に構築した動的セグメント木。
 * @tparam Monoid 結合則と単位元を提供するモノイド。
 * @tparam NodeCapacity Internal の子配列容量。
 * @details
 * 要素の更新は set() または operator() の Setter を使用する。
 * prod / reverse_prod / all_prod / all_reverse_prod、
 * 境界探索、挿入削除、split / merge、反転を利用できる。
 *
 * ### 計算量
 * 区間積・点更新・境界探索は O(NodeCapacity log_{NodeCapacity} N)。
 * all_prod / all_reverse_prod は O(1)。
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

/**
 * @brief `Op` と `E` を直接指定して構築する B+ tree セグメント木。
 */
template <class T, auto Op, auto E, std::size_t NodeCapacity = 64>
using BSegtree = Bplustree_Segtree<Monoid<T, Op, E>, NodeCapacity>;

/**
 * @brief 単位元を指定せずに構築する B+ tree セグメント木。
 * @details
 * 各要素を通常の `T` として扱いつつ、内部では人工的な単位元を使用する。
 */
template <class T, auto Op, std::size_t NodeCapacity = 64>
using BSegtreeWithoutE = Bplustree_Segtree<MonoidWithoutE<T, Op>, NodeCapacity>;
