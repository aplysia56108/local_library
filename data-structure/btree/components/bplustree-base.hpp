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
template <class Traits, bool IsReversible = false>
class Base {
 public:
  using traits_type = Traits;
  using value_type = typename Traits::T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  static constexpr bool is_reversible = IsReversible;

 protected:
  using T = value_type;
  using Node = typename Traits::Node;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  static constexpr size_type NodeCap = Traits::kNodeCapacity;
  static_assert(NodeCap >= 3);
  std::unique_ptr<Node> root_;

  static void materialize(Node* node) {
    node->apply_reverse();
    node->apply_lazy();
  }
  static void refresh(Leaf* leaf) {
    leaf->size = leaf->count;
    leaf->after_sync();
  }
  static void refresh(Internal* internal) {
    internal->sync();
    internal->after_sync();
  }
  static void refresh(Node* node) {
    if (node->is_leaf())
      refresh(static_cast<Leaf*>(node));
    else
      refresh(static_cast<Internal*>(node));
  }
  static void refresh_path(Node* node) {
    for (; node != nullptr; node = node->parent) refresh(node);
  }
  static void detach_root(Node* node) {
    node->parent = nullptr;
    node->position = 0;
  }
  void init() {
    root_ = std::make_unique<Leaf>();
    refresh(root_.get());
  }
  void normalize_root() {
    while (!root_->is_leaf() && root_->count == 1) {
      materialize(root_.get());
      auto* internal = static_cast<Internal*>(root_.get());
      auto child = std::move(internal->children[0]);
      root_ = std::move(child);
    }
    if (root_->size == 0) init();
    detach_root(root_.get());
  }

  void build(std::vector<T>& values) {
    if (values.empty()) return init();
    constexpr size_type MaxCount = NodeCap - 1;
    std::vector<std::unique_ptr<Node>> level;
    for (size_type i = 0; i < values.size();) {
      auto leaf = std::make_unique<Leaf>();
      while (leaf->count < MaxCount && i < values.size())
        leaf->values[leaf->count++] = std::move(values[i++]);
      refresh(leaf.get());
      level.push_back(std::move(leaf));
    }
    while (level.size() > 1) {
      std::vector<std::unique_ptr<Node>> next;
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
    detach_root(root_.get());
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
  using const_iterator = ::Iterator<Node, Leaf, Internal, T, true>;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using setter_type = Setter<Base, T>;

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
  Base(const Base& other) : Base(other.begin(), other.end()) {}
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
    return root_->size;
  }
  bool empty() const noexcept {
    return size() == 0;
  }
  iterator nth(size_type index) const {
    assert(index <= size());
    auto [leaf, offset] = root_->locate(index);
    return {leaf, offset};
  }
  iterator bbegin() const {
    auto [leaf, offset] = root_->locate(-1);
    return {leaf, offset};
  }
  iterator begin() const {
    return nth(0);
  }
  iterator end() const {
    return nth(size());
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
    assert(index < size());
    return *nth(index);
  }
  const T& operator[](size_type index) const {
    return get(index);
  }
  const T& front() const {
    return get(0);
  }
  const T& back() const {
    assert(!empty());
    return get(size() - 1);
  }
  setter_type operator()(size_type index) {
    assert(index < size());
    return {*this, index};
  }
  void set(size_type index, const T& value) {
    assert(index < size());
    auto [leaf, offset] = root_->locate_with_lazy(index);
    leaf->values[offset] = value;
    refresh_path(leaf);
  }
  iterator insert(size_type index, const T& value) {
    assert(index <= size());
    auto [leaf, offset] = root_->locate_with_lazy(index);
    std::unique_ptr<Node> right;
    if (leaf->insert(value, offset)) right = leaf->split();
    Node* child = leaf;
    for (Internal* parent = child->parent; parent != nullptr;
         parent = child->parent) {
      if (right) {
        if (parent->insert(std::move(right), child->position + 1))
          right = parent->split();
      } else
        refresh(parent);
      child = parent;
    }
    if (right) {
      auto parent = std::make_unique<Internal>();
      parent->insert(std::move(root_), 0);
      parent->insert(std::move(right), 1);
      root_ = std::move(parent);
    }
    return nth(index);
  }
  iterator insert(iterator pos, const T& value) {
    return insert(pos - begin(), value);
  }
  void push_back(const T& value) {
    insert(size(), value);
  }
  void push_front(const T& value) {
    insert(0, value);
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
    return erase(pos - begin());
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
    detach_root(root_.get());
    detach_root(right.root_.get());
    normalize_root();
    right.normalize_root();
    return right;
  }
  Base split(iterator pos) {
    return split(pos - begin());
  }
  void merge(Base& right) {
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
    detach_root(root_.get());
    normalize_root();
    right.init();
  }
  void reverse()
    requires(IsReversible)
  {
    root_->push_reverse();
  }
  void reverse(size_type first, size_type last)
    requires(IsReversible)
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
    requires(IsReversible)
  {
    reverse(first - begin(), last - begin());
  }
};
}  // namespace Bplustree

template <class Traits, bool IsReversible = false>
using Bplustree_Base = Bplustree::Base<Traits, IsReversible>;
