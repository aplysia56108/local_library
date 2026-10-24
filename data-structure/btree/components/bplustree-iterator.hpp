#pragma once

#include <iterator>
#include <cassert>
#include <compare>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>

/**
 * @brief B+ tree 上の要素を指すイテレータ。
 * @details 通常要素は `(leaf_, index_)` で表す。最左 Leaf の
 * `index_ == -1` は `bbegin()`、最右 Leaf の
 * `index_ == leaf_->count` は `end()` を表す。
 *
 * C++20 の `std::random_access_iterator` コンセプトを満たす。
 * ただし木構造上、任意距離の移動やイテレータ差分は厳密には O(1) ではなく
 * O(NodeCapacity log_{NodeCapacity} N) となり得る。
 * 参照時は祖先から遅延更新を適用するため O(log_{NodeCapacity} N)。
 */
// Traitsが含む情報
// T:要素の型
// IsReversible:回転操作を許容するかどうか
// IsMergable:マージ操作を許容するかどうか
// Node, Leaf, Internal:それぞれ要素の型。Leaf, InternalはNodeを継承している。
// kNodeCapacity:Internalのmax容量
template <class Tree, bool IsConst, bool VisitAll = false>
class Iterator {
  template <class, bool>
  friend class Iterator;
  friend Tree;

 protected:
  using Traits = typename Tree::traits;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  using T = typename Traits::T;
  static constexpr bool IsReversible = Traits::IsReversible;
  Leaf* leaf_ = nullptr;

  static void apply_lazy_path(Internal* node) {
    if (node->parent != nullptr) apply_lazy_path(node->parent);
    node->apply_lazy();
  }

  void apply_lazy_path() {
    apply_lazy_path(leaf_->parent);
  }

  static void apply_reverse_path(Internal* node) {
    if (node->parent != nullptr) apply_reverse_path(node->parent);
    node->reverse_lazy();
  }

  void apply_reverse_path() {
    apply_reverse_path(leaf_->parent);
  }

  static void refresh_path(Internal* node) {
    node->refresh();
    if (node->parent != nullptr) refresh_path(node->parent);
  }

  void refresh_path() {
    refresh_path(leaf_->parent);
  }

  void advance(std::ptrdiff_t offset) {
    assert(leaf_ != nullptr);
    std::ptrdiff_t to_index = const_cast<std::size_t>(leaf_->position) + offset;
    Internal* node = leaf_->parent;
    if constexpr (!VisitAll) apply_reverse_path();

    while (true) {
      if (0 <= to_index && to_index < static_cast<std::ptrdiff_t>(node->size)) {
        leaf_ = node->locate<VisitAll>(to_index);
        return;
      }
      if (node->parent == nullptr) {
        leaf_ = node->locate<VisitAll>(to_index);
        return;
      }
      for (std::size_t i = 0; i < node->position; ++i) {
        to_index += node->parent->child_size(i);
      }
      node = node->parent;
    }
  }

 public:
  using iterator_concept = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = std::conditional_t<IsConst, const T&, T&>;
  using pointer = std::conditional_t<IsConst, const T*, T*>;

  Iterator() = default;
  Iterator(Leaf* leaf) : leaf_(leaf) {}

  template <bool OtherConst, bool OtherVisitALL>
    requires(IsConst && !OtherConst)
  Iterator(const Iterator<Traits, OtherConst, OtherVisitALL>& other)
      : leaf_(other.leaf_) {}

  reference operator*() const {
    assert(leaf_ != nullptr);
    if constexpr (!VisitAll) apply_lazy_path();
    return leaf_->value;
  }

  pointer operator->() const {
    return std::addressof(operator*());
  }

  void set(T& value) const {
    apply_lazy_path();
    leaf_->value = value;
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
    if constexpr (!VisitAll) {
      lhs.apply_reverse_path();
      rhs.apply_reverse_path();
    }
    Internal* l_node = lhs.leaf_->parent;
    std::ptrdiff_t l_index = lhs.leaf_->position;
    Internal* r_node = rhs.leaf_->parent;
    std::ptrdiff_t r_index = rhs.leaf_->position;

    while (l_node != r_node) {
      assert(l_node->parent != nullptr && r_node->parent != nullptr);
      for (std::size_t i = 0; i < l_node->position; ++i)
        l_index += l_node->parent->children[i]->size;
      for (std::size_t i = 0; i < r_node->position; ++i)
        r_index += r_node->parent->children[i]->size;

      l_node = l_node->parent;
      r_node = r_node->parent;
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
