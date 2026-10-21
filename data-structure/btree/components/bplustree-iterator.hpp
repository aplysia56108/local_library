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
template <class Node, class Leaf, class Internal, class T, bool IsConst>
class Iterator {
  template <class, class, class, class, bool> friend class Iterator;
 protected:
  Leaf* leaf_ = nullptr;
  // bbegin(beginの一つ前を指すiterator)を想定してptrdiff_tを採用
  std::ptrdiff_t index_ = 0;

  static void apply_lazy_path(Node* node) {
    if (node->parent != nullptr) apply_lazy_path(node->parent);
    node->apply_lazy();
  }

  void advance(std::ptrdiff_t offset) {
    assert(leaf_ != nullptr);
    std::ptrdiff_t to_index = index_ + offset;

    Node* node = static_cast<Node*>(leaf_);
    while (true) {
      if (0 <= to_index && to_index < static_cast<std::ptrdiff_t>(node->size)) {
        std::tie(leaf_, index_) =
            node->locate(to_index);
        return;
      }
      if (node->parent == nullptr) {
        std::tie(leaf_, index_) = node->locate(to_index);
        return;
      }
      for (std::size_t i = 0; i < node->position; ++i) {
        to_index += node->parent->children[i]->size;
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
  Iterator(Leaf* node, std::ptrdiff_t index) : leaf_(node), index_(index) {}

  template <bool OtherConst>
    requires(IsConst && !OtherConst)
  Iterator(const Iterator<Node, Leaf, Internal, T, OtherConst>& other)
      : leaf_(other.leaf_), index_(other.index_) {}

  reference operator*() const {
    assert(leaf_ != nullptr);
    assert(0 <= index_ && static_cast<std::size_t>(index_) < leaf_->count);
    apply_lazy_path(leaf_);
    return leaf_->values[index_];
  }

  pointer operator->() const {
    return std::addressof(operator*());
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
    if (lhs.leaf_ == rhs.leaf_) return lhs.index_ - rhs.index_;
    assert(lhs.leaf_ != nullptr && rhs.leaf_ != nullptr);
    Node* l_node = static_cast<Node*>(lhs.leaf_);
    std::ptrdiff_t l_index = lhs.index_;
    Node* r_node = static_cast<Node*>(rhs.leaf_);
    std::ptrdiff_t r_index = rhs.index_;
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
