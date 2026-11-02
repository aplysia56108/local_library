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

template <class Tree>
concept Has_Monoid_e = requires {
  { Tree::Monoid::e() } -> std::convertible_to<typename Tree::valuetype>;
};
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
// template <class Traits>
class Base {
 public:
  // using traits_type = Traits;
  using value_type = typename Traits::T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using const_iterator = Iterator<Base, true, false>;
  using iterator = Iterator<Base, false, false>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using setter_type = Setter<Base>;

 protected:
  using T = value_type;
  // using Node = typename Traits::Node;
  // using Leaf = typename Traits::Leaf;
  // using Internal = typename Traits::Internal;
  static constexpr size_type NodeCap = Traits::kNodeCapacity;
  static constexpr bool IsReversible = Traits::IsReversible;
  static constexpr bool IsMergable = Traits::IsMergable;
  static_assert(NodeCap >= 3);

  std::unique_ptr<Internal> root_;
  iterator bbegin_, end_;

  const T init_value = T{};

  void init() {
    auto bbegin = std::make_unique<Leaf>(init_value);
    auto end = std::make_unique<Leaf>(init_value);
    bbegin_ = iterator{bbegin.get()};
    end_ = iterator{end.get()};
    root_ = make_new_root(std::move(bbegin), std::move(end));
  }
  static void cut_root(std::unique_ptr<Internal>& root) {
    while (root->count == 1 && root->level) {
      root->materialize();
      auto child = std::move(root->children[0]);
      root.reset(static_cast<Internal*>(child.release()));
    }
  }

  void build(std::vector<T>& values) {
    if (values.empty()) return init();
    constexpr size_type MaxCount = NodeCap - 1;
    std::vector<std::unique_ptr<Node>> level;
    level.reserve(values.size() + 2);
    {
      auto bbegin = std::make_unique<Leaf>(init_value);
      bbegin_ = iterator{bbegin.get()};
      level.push_back(std::move(bbegin));
    }
    for (size_type i = 0; i < values.size(); ++i) {
      auto leaf = std::make_unique<Leaf>(std::move(values[i]));
      level.push_back(std::move(leaf));
    }
    {
      auto end = std::make_unique<Leaf>(init_value);
      end_ = iterator{end.get()};
      level.push_back(std::move(end));
    }
    while (level.size() > 1) {
      std::vector<std::unique_ptr<Internal>> next;
      for (size_type i = 0; i < level.size();) {
        auto parent = std::make_unique<Internal>();
        while (parent->count < MaxCount && i < level.size())
          parent->children[parent->count++] = std::move(level[i++]);
        parent->refresh();
        next.push_back(std::move(parent));
      }
      level.clear();
      level.resize(next.size());
      for (auto& child : next) level.push_back(std::move(child));
    }
    root_.reset(static_cast<Internal*>(level[0].release()));
    root_->parent = nullptr;
  }

  static std::unique_ptr<Internal> make_new_root(std::unique_ptr<Node> left,
                                                 std::unique_ptr<Node> right,
                                                 const std::size_t level = 0) {
    auto parent = std::make_unique<Internal>(level);
    parent->insert(std::move(left), 0);
    parent->insert(std::move(right), 1);
    parent->refresh();
    return parent;
  }

  static void refresh_recursive(Internal* node) {
    node->refresh();
    if (node->parent) refresh_recursive(node->parent);
  }

  static std::unique_ptr<Internal> insert_recursive(
      Internal* parent, std::size_t index,
      std::unique_ptr<Node> child = nullptr) {
    if (!parent) return nullptr;

    if (child) {
      if (parent->insert(std::move(child), index)) {
        if (parent->parent)
          return insert_recursive(parent->parent, parent->position + 1,
                                  parent->split());
        return parent->split();
      }
    }
    refresh_recursive(parent);
    return nullptr;
  };

  static void integrate_path(Internal* node) {
    if (node->parent) integrate_path(node->parent);
    node->integrate();
  }
  static void integrate_path(Leaf* leaf) {
    integrate_path(leaf->parent);
  }
  static void insert(Internal* parent, std::size_t index,
                     std::unique_ptr<Node> child,
                     std::unique_ptr<Internal>& root) {
    auto right = insert_recursive(parent, index, std::move(child));
    if (right) {
      auto level = right->level + 1;
      root = make_new_root(std::move(root), std::move(right), level);
    }
  }

  static void erase_recursive(Internal* parent, std::size_t index) {
    if (parent->erase(index))
      return erase_recursive(parent->parent, parent->position);
    if (parent->parent) refresh_recursive(parent->parent);
  }

  static void erase(Internal* parent, std::size_t index,
                    std::unique_ptr<Internal>& root) {
    erase_recursive(parent, index);
    cut_root(root);
  }

  static std::unique_ptr<Internal> devide(std::unique_ptr<Internal>& root,
                                          Leaf* leaf) {
    Internal* parent = leaf->parent;
    std::size_t position = leaf->position;
    auto right = std::make_unique<Internal>();
    while (parent) {
      auto next = parent->split(position);
      if (right->count) next->insert(std::move(right), 0);
      right = std::move(next);
      position = parent->position + 1;
      parent = parent->parent;
    }
    cut_root(root);
    integrate_path(root->locate<false>(root->size - 1));
    cut_root(right);
    integrate_path(right->locate<false>(0));
    return right;
  }

  static std::unique_ptr<Internal> unite(std::unique_ptr<Internal> left,
                                         std::unique_ptr<Internal> right) {
    if (right->size == 0) return left;
    if (left->size == 0) return right;
    std::size_t left_size = left->size;
    if (left->level <= right->level) {
      Internal* target = right.get();
      while (left->level + 1 < target->level) target = target->get_child(0);
      insert(target, 0, std::move(left), right);
      integrate_path(right->locate<false>(left_size));
      return right;
    }
    Internal* target = left.get();
    while (right->level + 1 < target->level)
      target = target->get_child(target->count - 1);
    insert(target, target->count, std::move(right), left);
    integrate_path(left->locate<false>(left_size));
    return left;
  };

  template <bool WithLazy = false>
  Leaf* locate(size_type index) const {
    ++index;
    assert(index < root_->size);
    return root_->locate<WithLazy>(index);
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
    return {locate(index)};
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
    return locate(index)->get();
  }
  const T& operator[](size_type index) const {
    return get(index);
  }
  const T& front() const {
    return get(1);
  }
  const T& back() const {
    return get(size() - 1);
  }
  setter_type operator()(size_type index) {
    return {locate<true>(index)};
  }

  void set(size_type index, const T& value) {
    locate<true>(index)->set(value);
  }

  iterator insert(size_type index, const T& value) {
    return insert(locate<true>(index), value);
  }
  iterator insert(iterator pos, const T& value) {
    Internal* parent = pos.leaf_->parent;
    std::size_t position = pos.leaf_->position;
    std::unique_ptr<Node> child = std::make_unique<Leaf>(value);
    Leaf* res = static_cast<Leaf*>(child.get());
    insert(parent, position, std::move(child), root_);

    return res;
  }

  void push_back(const T& value) {
    insert(end(), value);
  }

  void push_front(const T& value) {
    insert(begin(), value);
  }

  iterator erase(size_type index) {
    auto leaf = locate<false>(index);
    return erase(iterator{leaf});
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
    cut_root(root_);
    return res;
  }

  iterator erase(size_type first, size_type last) {
    assert(first <= last && last <= size());
    return erase(nth(first), nth(last));
  }
  iterator erase(iterator first, iterator last) {
    if (first == last) return last;
    auto right = devide(root_, last.leaf_);
    auto removed = devide(root_, first.leaf_);
    root_ = unite(std::move(root_), std::move(right));
    return last;
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
    return split(nth(index));
  }
  Base split(iterator pos) {
    Base right;
    if (pos == end()) return right;
    if (pos == begin()) {
      swap(*this, right);
      return right;
    }
    right.root_ = devide(root_, pos.leaf_);
    auto bbegin = std::make_unique<Leaf>(init_value);
    auto end = std::make_unique<Leaf>(init_value);
    right.end_ = end_;
    right.bbegin_ = iterator{bbegin.get()};
    end_ = iterator{end.get()};
    auto end_parent = root_->locate<false>(root_->size - 1)->parent;
    insert(end_parent, end_parent->count, std::move(end), root_);
    auto bbegin_parent = right.root_->locate<false>(0)->parent;
    insert(bbegin_parent, 0, std::move(bbegin), right.root_);
    return right;
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

    auto end_pos = root_->locate<false>(root_->size - 1);
    erase(end_pos->parent, end_pos->position, root_);
    auto bbegin_pos = right.root_->locate<false>(right.root_->size - 1);
    erase(bbegin_pos->parent, bbegin_pos->position, root_);

    root_ = unite(std::move(root_), std::move(right.root_));
    this->end_ = right.end_;
    right.init();
  }

  void reverse()
    requires(IsMergable)
  {
    root_->push_reverse();
    using std::swap;
    swap(bbegin_, end_);
  }
  void reverse(size_type first, size_type last)
    requires(IsMergable)
  {
    assert(first <= last && last <= size());
    if (last - first < 2) return;
    if (first == 0 && last == size()) return reverse();
    reverse(nth(first), nth(last));
  }
  void reverse(iterator first, iterator last)
    requires(IsMergable)
  {
    auto right = devide(root_, last.leaf_);
    auto middle = devide(root_, first.leaf_);
    middle->push_reverse();
    root_ = unite(std::move(root_), std::move(middle));
    root_ = unite(std::move(root_), std::move(right));
  }
};
}  // namespace Bplustree
