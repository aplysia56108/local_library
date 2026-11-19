#pragma once

#include <cstddef>
#include <iterator>
#include <utility>

#include "data-structure/btree/components/bplustree-base.hpp"

/**
 * @brief 1要素1Leaf の B+tree による、位置指定の動的配列。
 * @tparam T 要素型。
 * @tparam NodeCapacity Internal の子 ID 配列容量。
 * @details
 * get / operator[] / iterator は読み取り専用。要素の更新は
 * set(index,value) または operator()(index) の Setter を利用する。
 * insert / erase / reverse / split / merge に対応し、実要素が削除されない限り
 * 既存 iterator の Leaf ID は維持される（clear / assign / rebuild は例外）。
 * bbegin と end はオブジェクト固有の固定番兵。
 *
 * ### 計算量
 * 位置操作・分割・結合は概ね O(NodeCapacity * 木の高さ)。
 * 全体 reverse は O(1)。
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

  /**
   * @brief index 以降の要素を分離して同じ Bplustree_Vector 型で返す。
   * @details 左右の木の番兵 ID は固定のままで、実要素 Leaf ID も維持する。
   */
  Bplustree_Vector split(size_type index) {
    Bplustree_Vector right;
    static_cast<Parent&>(right) = Parent::split(index);
    return right;
  }
  /** @brief position 以降を分離する（位置は自身の [begin(),end()]）。 */
  Bplustree_Vector split(iterator position) {
    Bplustree_Vector right;
    static_cast<Parent&>(right) = Parent::split(position);
    return right;
  }
  /** @brief right の全実要素を末尾へ連結し、right を空にする。 */
  
  void merge(Bplustree_Vector& right) {
    Parent::merge(static_cast<Parent&>(right));
  }
};
