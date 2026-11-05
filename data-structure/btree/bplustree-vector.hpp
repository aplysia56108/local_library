#pragma once

#include <cstddef>
#include <iterator>
#include <utility>

#include "data-structure/btree/components/bplustree-base.hpp"

/**
 * @brief 1要素1Leafの B+ tree を使う動的配列。
 * @tparam T 要素型。
 * @tparam NodeCapacity Internal の子配列容量。
 * @details
 * Base と共通の Node / Iterator を使う。要素の変更は set() または
 * operator() の Setter を使用する。イテレータの直接書き換えは行わない。
 *
 * 旧実装の「1 Leaf に複数要素」方式とは異なるため、メモリ使用量や
 * 定数倍の性能特性は変化する。
 */
template <class T = long long, std::size_t NodeCapacity = 64>
class Bplustree_Vector
    : public Bplustree::Base<Bplustree::Traits<T, NodeCapacity>> {
  using Parent = Bplustree::Base<Bplustree::Traits<T, NodeCapacity>>;

 public:
  using Parent::Parent;
  Bplustree_Vector() = default;
  using typename Parent::size_type;
  using typename Parent::iterator;
  using Parent::operator();
  using Parent::operator[];
  using Parent::assign;
  using Parent::resize;

  /** @brief 分割した右半分を同じ公開型で返す。 */
  Bplustree_Vector split(size_type index) {
    Bplustree_Vector right;
    static_cast<Parent&>(right) = Parent::split(index);
    return right;
  }
  Bplustree_Vector split(iterator position) {
    Bplustree_Vector right;
    static_cast<Parent&>(right) = Parent::split(position);
    return right;
  }
  /** @brief right の要素を末尾へ結合して right を空にする。 */
  void merge(Bplustree_Vector& right) {
    Parent::merge(static_cast<Parent&>(right));
  }
};
