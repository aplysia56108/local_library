#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "data-structure/btree/components/bplustree-node.hpp"
#include "data-structure/btree/components/bplustree-iterator.hpp"
#include "data-structure/btree/components/bplustree-setter.hpp"

namespace Bplustree {

/** Shared positional storage. Traits provides T, kNodeCapacity, Node, Leaf and
 * Internal. Stable nodes hold count < NodeCap; full insertions split
 * immediately. Iterators expose const elements so writes cannot bypass metadata
 * maintenance. Structural mutations invalidate iterators. See README.md for
 * extension hooks.
 */
// Traitsが含む情報
// T:要素の型
// IsReversible:回転操作を許容するかどうか
// IsMergable:マージ操作を許容するかどうか
// Node, Leaf, Internal:それぞれ要素の型。Leaf, InternalはNodeを継承している。
// kNodeCapacity:Internalのmax容量
template <class Traits>
class Base {
 public:
  using traits_type = Traits;
  using value_type = typename Traits::T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using const_iterator = ::Iterator<Base, false>;
  using iterator = ::Iterator<Base, true>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using setter_type = Setter<Base>;

 protected:
  using T = value_type;
  using Node = typename Traits::Node;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  static constexpr size_type NodeCap = Traits::kNodeCapacity;
  static constexpr bool IsReversible = Traits::IsReversible;
  static constexpr bool IsMergable = Traits::IsMergable;
  static_assert(NodeCap >= 3);

  std::unique_ptr<Internal> root_;
  iterator bbegin_, end_;

  void init() {
    root_ = std::make_unique<Internal>();
    auto bbegin = std::make_unique<Leaf>();
    auto end = std::make_unique<Leaf>();
    bbegin_ = iterator{bbegin.get()};
    end_ = iterator{end.get()};
    root_->insert(bbegin, 0);
    root_->insert(end, 0);
  }
  void normalize_root() {
    while (root_->count == 1) {
      root_->materialize();
      auto child = std::move(root_->children[0]);
      root_ = std::move(child);
    }
  }

  void build(std::vector<T>& values) {
    if (values.empty()) return init();
    constexpr size_type MaxCount = NodeCap - 1;
    std::vector<std::unique_ptr<Node>> level;
    level.resize(values.size() + 2);
    {
      auto bbegin = std::make_unique<Leaf>();
      bbegin_ = iterator{bbegin.get()};
      level.push_back(std::move(bbegin));
    }
    for (size_type i = 0; i < values.size();) {
      auto leaf = std::make_unique<Leaf>(std::move(values[i]));
      level.push_back(std::move(leaf));
    }
    {
      auto end = std::make_unique<Leaf>();
      end_ = iterator{end.get()};
      level.push_back(std::move(end));
    }
    while (level.size() > 1) {
      std::vector<std::unique_ptr<Internal>> next;
      for (size_type i = 0; i < level.size();) {
        auto parent = std::make_unique<Internal>();
        while (parent->count < MaxCount && i < level.size())
          parent->children[parent->count++] = std::move(level[i++]);
        refresh(parent.get());
        next.push_back(std::move(parent));
      }
      level = std::move(next);
    }
    root_ = std::move(level[0]);
    root_->parent = nullptr;
  }

  using NodePair = std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>>;
  // Preserve both subtree heights; normalize only the resulting top-level
  // roots.
  static NodePair split_node(std::unique_ptr<Node> node, size_type index) {
    if (index == 0) return {nullptr, std::move(node)};
    if (index == node->size) return {std::move(node), nullptr};
    materialize(node.get());
    if (node->is_leaf()) {
      auto right = static_cast<Leaf*>(node.get())->split(index);
      return {std::move(node), std::move(right)};
    }
    auto* internal = static_cast<Internal*>(node.get());
    auto left = std::make_unique<Internal>();
    auto right = std::make_unique<Internal>();
    for (size_type i = 0; i < internal->count; ++i) {
      auto child = std::move(internal->children[i]);
      const size_type child_size = child->size;
      if (index >= child_size) {
        left->children[left->count++] = std::move(child);
        index -= child_size;
      } else if (index == 0) {
        right->children[right->count++] = std::move(child);
      } else {
        auto parts = split_node(std::move(child), index);
        left->children[left->count++] = std::move(parts.first);
        right->children[right->count++] = std::move(parts.second);
        index = 0;
      }
    }
    refresh(left.get());
    refresh(right.get());
    return {std::move(left), std::move(right)};
  }

  static std::unique_ptr<Internal> devide(iterator pos) {
    Internal* parent = pos.leaf_->parent;
    std::size_t position = pos.leaf_->position;
    auto right = parent->split(position);
    position = parent->position + 1;
    parent = parent->parent;

    while (parent) {
      auto next = parent->split(position);
      if (right->count) next->insert(std::move(right), 0);
      right = std::move(next);
      position = parent->position + 1;
      parent = parent->parent;
    }
    return std::move(right);
  }
  // Return a root and, on overflow, a same-height right sibling.
  static NodePair join_nodes(std::unique_ptr<Node> left,
                             std::unique_ptr<Node> right) {
    materialize(left.get());
    materialize(right.get());
    if (left->level == right->level) {
      if (left->count + right->count >= NodeCap)
        return {std::move(left), std::move(right)};
      if (left->is_leaf()) {
        auto* l = static_cast<Leaf*>(left.get());
        auto* r = static_cast<Leaf*>(right.get());
        for (size_type i = 0; i < r->count; ++i)
          l->values[l->count++] = std::move(r->values[i]);
      } else {
        auto* l = static_cast<Internal*>(left.get());
        auto* r = static_cast<Internal*>(right.get());
        for (size_type i = 0; i < r->count; ++i)
          l->children[l->count++] = std::move(r->children[i]);
      }
      refresh(left.get());
      return {std::move(left), nullptr};
    }
    const bool left_taller = left->level > right->level;
    auto parent = left_taller ? std::move(left) : std::move(right);
    auto* internal = static_cast<Internal*>(parent.get());
    const size_type index = left_taller ? internal->count - 1 : 0;
    auto child = std::move(internal->children[index]);
    auto parts = left_taller ? join_nodes(std::move(child), std::move(right))
                             : join_nodes(std::move(left), std::move(child));
    internal->children[index] = std::move(parts.first);
    std::unique_ptr<Node> overflow;
    if (parts.second) {
      if (internal->insert(std::move(parts.second), index + 1))
        overflow = internal->split();
    } else
      refresh(internal);
    return {std::move(parent), std::move(overflow)};
  }

 public:
  Base() {
    init();
  }
  template <std::input_iterator InputIt>
  Base(InputIt first, InputIt last) {
    assign(first, last);
  }
  explicit Base(size_type count) : Base(count, T{}) {}
  Base(size_type count, const T& value) {
    assign(count, value);
  }
  Base(std::initializer_list<T> values) : Base(values.begin(), values.end()) {}
  Base(const Base& other) : Base(other.begin(), other.end_()) {}
  Base(Base&& other) : Base() {
    swap(*this, other);
  }
  Base& operator=(const Base& other) {
    if (this != &other) {
      Base copy(other);
      swap(*this, copy);
    }
    return *this;
  }
  Base& operator=(Base&& other) noexcept {
    if (this != &other) swap(*this, other);
    return *this;
  }
  friend void swap(Base& left, Base& right) noexcept {
    using std::swap;
    swap(left.root_, right.root_);
    swap(left.bbegin_, right.bbegin_);
    swap(left.end_, right.end_);
  }
  template <std::input_iterator InputIt>
  void assign(InputIt first, InputIt last) {
    std::vector<T> values;
    for (; first != last; ++first) values.push_back(*first);
    build(values);
  }
  void assign(size_type count, const T& value = T{}) {
    std::vector<T> values(count, value);
    build(values);
  }
  size_type size() const noexcept {
    return root_->size - 2;
  }
  bool empty() const noexcept {
    return size() == 2;
  }
  iterator nth(size_type index) const {
    ++index;
    assert(index < root_->size);
    return {root_->locate<false>(index)};
  }
  iterator bbegin() const {
    return bbegin_;
  }
  iterator begin() const {
    return bbegin_ + 1;
  }
  iterator end() const {
    return end_;
  }
  const_iterator cbegin() const {
    return begin();
  }
  const_iterator cend() const {
    return end();
  }
  reverse_iterator rbegin() const {
    return reverse_iterator(end());
  }
  reverse_iterator rend() const {
    return reverse_iterator(begin());
  }
  const_reverse_iterator crbegin() const {
    return rbegin();
  }
  const_reverse_iterator crend() const {
    return rend();
  }
  const T& get(size_type index) const {
    ++index;
    assert(index < root_->size);
    return *nth(index);
  }
  const T& operator[](size_type index) const {
    return get(index);
  }
  const T& front() const {
    return get(1);
  }
  const T& back() const {
    return get(root_->size - 2);
  }
  setter_type operator()(size_type index) {
    assert(index < size());
    return {*this, index};
  }

  void set(size_type index, const T& value) {
    ++index;
    assert(index < root_->size);
    auto leaf = root_->locate<true>(index);
    iterator{leaf}.set(value);
  }

  iterator insert(size_type index, const T& value) {
    ++index;
    assert(index < root_->size);
    auto leaf = root_->locate<true>(index);
    return insert(iterator{leaf}, value);
  }
  iterator insert(iterator pos, const T& value) {
    Internal* parent = pos.leaf_->parent;
    std::size_t position = pos.leaf_->position;
    std::unique_ptr<Node> child = std::make_unique<Leaf>(value);
    Leaf* res = child.get();
    while (parent) {
      if (child) {
        if (parent->insert(std::move(child), position)) {
          child = parent->split();
          position = parent->position + 1;
        }
      } else {
        parent->refresh();
      }
      parent = parent->parent;
    }
    if (child) {
      auto parent = std::make_unique<Internal>();
      parent->insert(std::move(root_), 0);
      parent->insert(std::move(child), 1);
      root_ = std::move(parent);
    }
    return res;
  }
  void push_back(const T& value) {
    insert(end(), value);
  }
  void push_front(const T& value) {
    insert(begin(), value);
  }
  iterator erase(size_type index) {
    assert(index < size());
    auto [leaf, offset] = root_->locate_with_lazy(index);
    leaf->erase(offset);
    Node* child = leaf;
    while (child->parent != nullptr) {
      auto* parent = child->parent;
      if (child->count == 0)
        parent->erase(child->position);
      else
        refresh(parent);
      child = parent;
    }
    normalize_root();
    return nth(index);
  }
  iterator erase(iterator pos) {
    iterator res = pos + 1;
    Internal* parent = pos.leaf_->parent;
    std::size_t position = pos.leaf_->position;
    bool erase_flg = true;
    while (parent != nullptr) {
      if (erase_flg) {
        erase_flg = parent->erase(position);
        position = parent->position;
      } else {
        parent->refresh();
      }
      parent = parent->parent;
    }
    normalize_root();
    return res;
  }

  iterator erase(size_type first, size_type last) {
    assert(first <= last && last <= size());
    if (first == last) return nth(first);
    auto right = split(last);
    auto removed = split(first);
    merge(right);
    return nth(first);
  }
  iterator erase(iterator first, iterator last) {
    return erase(first - begin(), last - begin());
  }
  void pop_back() {
    assert(!empty());
    erase(size() - 1);
  }
  void pop_front() {
    erase(0);
  }
  void clear() {
    init();
  }
  void resize(size_type count, const T& value = T{}) {
    if (count < size()) erase(count, size());
    while (size() < count) push_back(value);
  }
  void rebuild() {
    assign(begin(), end());
  }
  iterator find(const T& value) const {
    return std::find(begin(), end(), value);
  }
  size_type count(const T& value) const {
    return std::count(begin(), end(), value);
  }
  bool contains(const T& value) const {
    return find(value) != end();
  }
  Base split(size_type index) {
    assert(index <= size());
    Base right;
    if (index == size()) return right;
    if (index == 0) {
      swap(*this, right);
      return right;
    }
    auto parts = split_node(std::move(root_), index);
    root_ = std::move(parts.first);
    right.root_ = std::move(parts.second);
    root_->parent = nullptr;
    right.root_->parent = nullptr;
    normalize_root();
    right.normalize_root();
    return right;
  }
  Base split(iterator pos) {
    return split(pos - begin());
  }
  void merge(Base& right)
    requires(IsMergable)
  {
    assert(this != &right);
    if (right.empty()) return;
    if (empty()) {
      swap(*this, right);
      return;
    }
    auto parts = join_nodes(std::move(root_), std::move(right.root_));
    root_ = std::move(parts.first);
    if (parts.second) {
      auto parent = std::make_unique<Internal>();
      parent->insert(std::move(root_), 0);
      parent->insert(std::move(parts.second), 1);
      root_ = std::move(parent);
    }
    root_->parent = nullptr;
    normalize_root();
    right.init();
  }
  void reverse()
    requires(IsMergable)
  {
    root_->push_reverse();
  }
  void reverse(size_type first, size_type last)
    requires(IsMergable)
  {
    assert(first <= last && last <= size());
    if (last - first < 2) return;
    if (first == 0 && last == size()) return reverse();
    auto right = split(last);
    auto middle = split(first);
    middle.reverse();
    merge(middle);
    merge(right);
  }
  void reverse(iterator first, iterator last)
    requires(IsMergable)
  {
    reverse(first - begin(), last - begin());
  }
};
}  // namespace Bplustree

template <class Traits>
using Bplustree_Base = Bplustree::Base<Traits>;
