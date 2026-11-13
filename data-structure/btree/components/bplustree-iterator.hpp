#pragma once

#include <iterator>
#include <cassert>
#include <compare>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>

#include "data-structure/btree/components/bplustree-node.hpp"

/**
 * @brief B+ tree 上の要素を指すイテレータ。
 * @details Leaf の Pool ID で要素と番兵を表す。
 *
 * C++20 の `std::random_access_iterator` コンセプトを満たす。
 * ただし木構造上、任意距離の移動やイテレータ差分は厳密には O(1) ではなく
 * O(NodeCapacity log_{NodeCapacity} N) となり得る。
 * 参照時は祖先から遅延更新を適用するため O(log_{NodeCapacity} N)。
 */
// Traitsが含む情報
// T:要素の型
// Node, Leaf, Internal:それぞれ要素の型。Leaf, InternalはNodeを継承している。
// kNodeCapacity:Internalのmax容量
namespace Bplustree {

template <class Traits, class Tree, bool IsConst, bool VisitAll = false>
class Iterator {
  template <class, class, bool, bool>
  friend class Iterator;
  friend Tree;

 protected:
  // using Traits = typename Tree::traits;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  using T = typename Traits::T;
  using Pools = NodePools<Traits>;
  using id_type = typename Pools::id_type;
  static constexpr id_type null_id = NodePools<Traits>::null_id;

  id_type leaf_id_ = null_id;

  static Leaf& get_leaf(const id_type& id) {
    return Pools::leaves()[id];
  }
  static Internal& get_node(const id_type& id) {
    return Pools::internals()[id];
  }
  Leaf& this_leaf() const {
    return get_leaf(leaf_id_);
  }

  static void apply_reverse_path(id_type node_id) {
    auto& node = get_node(node_id);
    if (node.parent != null_id) apply_reverse_path(node.parent);
    node.apply_reverse();
  }

  void apply_reverse_path() const {
    apply_reverse_path(this_leaf().parent);
  }

  static void materialize_path(id_type node_id) {
    auto& node = get_node(node_id);
    if (node.parent != null_id) materialize_path(node.parent);
    node.materialize();
  }

  void materialize_path() const {
    materialize_path(this_leaf().parent);
  }

  static void refresh_path(id_type node_id) {
    auto& node = get_node(node_id);
    node.after_sync();
    if (node.parent != null_id) refresh_path(node.parent);
  }

  void refresh_path() const {
    refresh_path(this_leaf().parent);
  }

  void advance(std::ptrdiff_t offset) {
    assert(leaf_id_ != null_id);
    // Reversal changes each Leaf's position. Materialize it before reading
    // the current position or computing the target index.
    auto& leaf = this_leaf();
    if constexpr (!VisitAll) apply_reverse_path();
    std::ptrdiff_t to_index =
        static_cast<std::ptrdiff_t>(leaf.position) + offset;
    id_type node_id = leaf.parent;

    while (true) {
      auto& node = get_node(node_id);
      if (0 <= to_index && to_index < static_cast<std::ptrdiff_t>(node.size)) {
        leaf_id_ = node.template locate<VisitAll>(to_index);
        return;
      }
      assert(node.parent != null_id);
      for (std::size_t i = 0; i < node.position; ++i) {
        to_index += get_node(node.parent).child_size(i);
      }
      node_id = node.parent;
    }
  }

 public:
  using iterator_concept = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = const T&;
  using pointer = const T*;

  Iterator() = default;
  explicit Iterator(id_type leaf_id) : leaf_id_(leaf_id) {}

  template <bool OtherConst, bool OtherVisitALL>
    requires(IsConst && !OtherConst)
  Iterator(const Iterator<Traits, Tree, OtherConst, OtherVisitALL>& other)
      : leaf_id_(other.leaf_id_) {}

  reference operator*() const {
    assert(leaf_id_ != null_id);
    if constexpr (!VisitAll) materialize_path();
    return this_leaf().get();
  }

  pointer operator->() const {
    return std::addressof(operator*());
  }

  void init() const
    requires VisitAll
  {
    materialize_path();
  };

  void set(const T& value) const
    requires(!IsConst)
  {
    materialize_path();
    this_leaf().set(value);
    refresh_path();
  }

  Iterator& operator++() {
    advance(1);
    return *this;
  }

  Iterator operator++(signed) {
    Iterator result = *this;
    ++*this;
    return result;
  }

  Iterator& operator--() {
    advance(-1);
    return *this;
  }

  Iterator operator--(signed) {
    Iterator result = *this;
    --*this;
    return result;
  }

  Iterator& operator+=(std::ptrdiff_t offset) {
    advance(offset);
    return *this;
  }

  Iterator& operator-=(std::ptrdiff_t offset) {
    return *this += -offset;
  }

  friend Iterator operator+(Iterator it, std::ptrdiff_t offset) {
    return it += offset;
  }

  friend Iterator operator+(std::ptrdiff_t offset, Iterator it) {
    return it += offset;
  }

  friend Iterator operator-(Iterator it, std::ptrdiff_t offset) {
    return it -= offset;
  }

  friend std::ptrdiff_t operator-(const Iterator& lhs, const Iterator& rhs) {
    lhs.apply_reverse_path();
    rhs.apply_reverse_path();

    id_type l_node_id = get_leaf(lhs.leaf_id_).parent;
    std::ptrdiff_t l_index = get_leaf(lhs.leaf_id_).position;
    id_type r_node_id = get_leaf(rhs.leaf_id_).parent;
    std::ptrdiff_t r_index = get_leaf(rhs.leaf_id_).position;

    while (l_node_id != r_node_id) {
      auto& l_node = get_node(l_node_id);
      auto& r_node = get_node(r_node_id);
      assert(l_node.parent != null_id && r_node.parent != null_id);
      for (std::size_t i = 0; i < l_node.position; ++i)
        l_index += get_node(l_node.parent).child_size(i);
      for (std::size_t i = 0; i < r_node.position; ++i)
        r_index += get_node(r_node.parent).child_size(i);

      l_node_id = l_node.parent;
      r_node_id = r_node.parent;
    }
    return l_index - r_index;
  }

  reference operator[](difference_type offset) const {
    return *(*this + offset);
  }

  friend std::strong_ordering operator<=>(const Iterator& lhs,
                                          const Iterator& rhs) {
    return (lhs - rhs) <=> 0;
  }

  friend bool operator==(const Iterator&, const Iterator&) = default;
};
}  // namespace Bplustree