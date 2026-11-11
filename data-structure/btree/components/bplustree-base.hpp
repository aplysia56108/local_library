#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>
#include <iostream>

#include "data-structure/btree/components/bplustree-node.hpp"
#include "data-structure/btree/components/bplustree-iterator.hpp"
#include "data-structure/btree/components/bplustree-setter.hpp"

#include "data-structure/btree/components/bplustree-node-pools.hpp"

namespace Bplustree {

// Traitsが含む情報
// T:要素の型
// Node, Leaf, Internal:それぞれ要素の型。Leaf, InternalはNodeを継承している。
// kNodeCapacity:Internalのmax容量
template <class Traits>
class Base {
 public:
  using traits_type = Traits;
  using value_type = typename Traits::T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using const_iterator = Iterator<Traits, Base, true, false>;
  using iterator = Iterator<Traits, Base, false, false>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using setter_type = Setter<Base>;
  using Pools = NodePools<Traits>;
  using id_type = typename Pools::id_type;
  using leaf_pool_type = ObjectPool<typename Traits::Leaf>;
  using internal_pool_type = ObjectPool<typename Traits::Internal>;
  static constexpr id_type null_id = Pools::null_id;

 protected:
  using T = value_type;
  using Node = typename Traits::Node;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  static constexpr size_type NodeCap = Traits::kNodeCapacity;
  static_assert(NodeCap >= 3);

  // root_ が指す ID 木はこの Base が所有する。各 ID は同じ Traits の Pool
  // で共有する。 番兵も Leaf Pool 上にあり、Iterator は所有権を持たない。
  id_type root_ = null_id;
  iterator bbegin_, end_;

  static Leaf& get_leaf(const id_type& id) {
    return Pools::leaves()[id];
  }
  static Internal& get_node(const id_type& id) {
    return Pools::internals()[id];
  }
  static T init_value() {
    if constexpr (requires { Traits::identity(); })
      return Traits::identity();
    else
      return T{};
  };

  /** @brief 前後に番兵 Leaf を1つずつ持つ空の木を構築する。 */
  void init() {
    const id_type bbegin = Pools::make_leaf(init_value());
    const id_type end = Pools::make_leaf(init_value());
    const id_type new_root = make_new_root(bbegin, end);
    Pools::recycle(root_);
    root_ = new_root;
    bbegin_ = iterator{bbegin};
    end_ = iterator{end};
  }

  /** @brief 子が1つだけの根を縮め、古い Internal を回収する。 */
  static void cut_root(id_type& root) {
    for (auto& node = get_node(root); node.count == 1 && node.level;) {
      node.materialize();
      const id_type child = node.children[0];
      node.children[0] = null_id;
      node.count = 0;
      Pools::recycle(root);
      root = child;
      get_node(root).parent = null_id;
    }
  }

  void build(std::vector<T>& values) {
    if (values.empty()) return init();
    constexpr size_type MaxCount = NodeCap - 1;

    std::vector<id_type> nodes;
    nodes.reserve(values.size() + 2);
    const id_type bbegin = Pools::make_leaf(init_value());
    nodes.push_back(bbegin);
    for (const auto& value : values) nodes.push_back(Pools::make_leaf(value));
    const id_type end = Pools::make_leaf(init_value());
    nodes.push_back(end);

    pos_type current_level = 0;
    while (nodes.size() > 1) {
      std::vector<id_type> next;
      next.reserve((nodes.size() + MaxCount - 1) / MaxCount);
      for (size_type i = 0; i < nodes.size();) {
        const id_type parent_id = Pools::make_internal(current_level);
        auto& parent = get_node(parent_id);
        while (parent.count < MaxCount && i < nodes.size())
          parent.children[parent.count++] = nodes[i++];
        parent.refresh();
        next.push_back(parent_id);
      }
      nodes = std::move(next);
      ++current_level;
    }
    const id_type new_root = nodes[0];
    Pools::recycle(root_);
    root_ = new_root;
    bbegin_ = iterator{bbegin};
    end_ = iterator{end};
  }

  static id_type make_new_root(id_type left, id_type right,
                               pos_type level = 0) {
    const id_type parent_id = Pools::make_internal(level);
    auto& parent = get_node(parent_id);
    parent.insert(left, 0);
    parent.insert(right, 1);
    parent.refresh();
    return parent_id;
  }

  static void materialize_path(id_type node_id) {
    auto& node = get_node(node_id);
    if (node.parent != null_id) materialize_path(node.parent);
    node.materialize();
  }

  static void refresh_recursive(id_type node_id) {
    auto& node = get_node(node_id);
    node.refresh();
    if (node.parent != null_id) refresh_recursive(node.parent);
  }

  static id_type insert_recursive(id_type parent_id, pos_type index,
                                  id_type child = null_id) {
    if (parent_id == null_id) return null_id;
    if (child != null_id) {
      auto& parent = get_node(parent_id);
      if (parent.insert(child, index)) {
        const id_type upper_id = parent.parent;
        const pos_type upper_pos = parent.position;
        const id_type right_id = parent.split();
        if (upper_id != null_id)
          return insert_recursive(upper_id, upper_pos + 1, right_id);
        return right_id;
      }
    }
    refresh_recursive(parent_id);
    return null_id;
  }

  template <bool Left>
  static std::vector<id_type> end_nodes(id_type root) {
    std::vector<id_type> nodes;
    nodes.reserve(get_node(root).level + 1);
    id_type current_id = root;
    while (true) {
      auto& current = get_node(current_id);
      current.apply_reverse();
      nodes.push_back(current_id);
      if (current.level == 0) break;
      if constexpr (Left)
        current_id = current.children[0];
      else
        current_id = current.children[current.count - 1];
    }
    return nodes;
  }

  static void cut_zero_node(id_type& root) {
    id_type node_id = root;
    for (bool left : {true, false}) {
      auto& node = get_node(node_id);
      while (true) {
        node.apply_reverse();
        if (node.size == 0 && node.parent != null_id) {
          erase(node.parent, node.position, root);
          break;
        }
        if (node.level == 0) break;
        node_id = node.children[left ? 0 : node.count - 1];
      }
    }
  }

  static void insert(id_type parent_id, pos_type index, id_type child,
                     id_type& root) {
    const id_type right_id = insert_recursive(parent_id, index, child);
    if (right_id != null_id) {
      const pos_type level = get_node(right_id).level + 1;
      root = make_new_root(root, right_id, level);
    }
  }

  static void erase_recursive(id_type parent_id, pos_type index) {
    auto& parent = get_node(parent_id);
    const id_type upper = parent.parent;
    const pos_type upper_pos = parent.position;
    if (parent.erase(index)) {
      if (upper != null_id) erase_recursive(upper, upper_pos);
      return;
    }
    if (upper != null_id) refresh_recursive(upper);
  }

  static void erase(id_type parent_id, pos_type index, id_type& root) {
    erase_recursive(parent_id, index);
    cut_root(root);
  }

  /**
   * @brief 指定 Leaf の直前で木を左右に分割し、右側の根 ID を返す。
   * @details 中間の空 Internal は、端の経路を ID でたどって除去・統合する。
   */
  static id_type devide(id_type& root, id_type leaf_id) {
    auto& leaf = get_leaf(leaf_id);
    materialize_path(leaf.parent);
    id_type parent_id = leaf.parent;
    pos_type position = leaf.position;
    id_type right = null_id;
    while (parent_id != null_id) {
      const id_type next = get_node(parent_id).split(position);
      if (right != null_id) {
        if (get_node(right).count)
          get_node(next).insert(right, 0);
        else
          Pools::recycle(right);
      }
      right = next;
      position = get_node(parent_id).position + 1;
      parent_id = get_node(parent_id).parent;
    }
    cut_root(root);
    cut_zero_node(root);
    for (id_type id : end_nodes<false>(root)) get_node(id).integrate();
    cut_root(right);
    cut_zero_node(right);
    for (id_type id : end_nodes<true>(right)) get_node(id).integrate();
    return right;
  }

  static id_type unite(id_type left_id, id_type right_id) {
    auto& left = get_node(left_id);
    auto& right = get_node(right_id);
    if (right.size == 0) {
      Pools::recycle(right_id);
      return left_id;
    }
    if (left.size == 0) {
      Pools::recycle(left_id);
      return right_id;
    }
    auto integrate_nodes = end_nodes<false>(left_id);
    for (id_type id : end_nodes<true>(right_id)) integrate_nodes.push_back(id);
    if (left.level == right.level) {
      id_type root = make_new_root(left_id, right_id, left.level + 1);
      for (id_type id : integrate_nodes) get_node(id).integrate();
      cut_root(root);
      return root;
    }
    if (left.level < right.level) {
      auto target_id = right_id;
      for (auto& target = get_node(target_id); left.level + 1 < target.level;) {
        target.materialize();
        target_id = target.children[0];
      }
      get_node(target_id).materialize();
      insert(target_id, 0, left_id, right_id);
      for (id_type id : integrate_nodes) get_node(id).integrate();
      return right_id;
    }
    auto& target_id = left_id;
    for (auto& target = get_node(target_id); right.level + 1 < target.level;) {
      target.materialize();
      target_id = target.children[0];
    }
    get_node(target_id).materialize();
    insert(target_id, get_node(target_id).count, right_id, left_id);
    for (id_type id : integrate_nodes) get_node(id).integrate();
    return left_id;
  }

  template <bool WithLazy = false>
  id_type locate(size_type index) const {
    assert(index <= size());
    ++index;
    return get_node(root_).template locate<WithLazy>(index);
  }

  static iterator mutable_iterator(const const_iterator& it) {
    return iterator{it.leaf_id_};
  }

 public:
  Base() {
    init();
  }
  ~Base() {
    Pools::recycle(root_);
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
    return get_node(root_).size - 2;
  }
  bool empty() const noexcept {
    return size() == 0;
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
    return *nth(index);
  }
  const T& operator[](size_type index) const {
    return get(index);
  }
  const T& front() const {
    return get(0);
  }
  const T& back() const {
    return get(size() - 1);
  }
  setter_type operator()(size_type index) {
    return {locate(index)};
  }

  void set(size_type index, const T& value) {
    nth(index).set(value);
  }

  iterator insert(size_type index, const T& value) {
    return insert(locate<true>(index), value);
  }
  iterator insert(iterator pos, const T& value) {
    auto& leaf = get_leaf(pos.leaf_id_);
    materialize_path(leaf.parent);
    const id_type parent = leaf.parent;
    const pos_type position = leaf.position;
    const id_type child = Pools::make_leaf(value);
    insert(parent, position, child, root_);
    return iterator{child};
  }

  void push_back(const T& value) {
    insert(end(), value);
  }

  void push_front(const T& value) {
    insert(begin(), value);
  }

  iterator erase(size_type index) {
    auto leaf = locate(index);
    return erase(iterator{leaf});
  }

  iterator erase(iterator pos) {
    iterator res = pos + 1;
    auto& leaf = get_leaf(pos.leaf_id_);
    materialize_path(leaf.parent);
    id_type parent = leaf.parent;
    pos_type position = leaf.position;
    erase(parent, position, root_);
    return res;
  }

  iterator erase(size_type first, size_type last) {
    assert(first <= last && last <= size());
    return erase(nth(first), nth(last));
  }
  iterator erase(iterator first, iterator last) {
    if (first == last) return last;
    const id_type right = devide(root_, last.leaf_id_);
    const id_type removed = devide(root_, first.leaf_id_);
    root_ = unite(root_, right);
    Pools::recycle(removed);
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
  /**
   * @brief index 番目の実要素の直前で分割し、右側の木を返す。
   * @details 位置は番兵を除いた
   * 0-indexed。左右の木は分割後もそれぞれ番兵を持つ。 分割境界の既存 Leaf
   * は移動させ、要素をコピーしない。
   * @return 右側の木。自身には左側を残す。
   */
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
    const id_type old_right_root = right.root_;
    right.root_ = devide(root_, pos.leaf_id_);
    Pools::recycle(old_right_root);
    const id_type bbegin = Pools::make_leaf(init_value());
    const id_type end = Pools::make_leaf(init_value());
    right.end_ = end_;
    right.bbegin_ = iterator{bbegin};
    end_ = iterator{end};
    const id_type end_leaf = get_node(root_).locate(get_node(root_).size - 1);
    const id_type end_parent = get_leaf(end_leaf).parent;
    insert(end_parent, get_node(end_parent).count, end, root_);
    const id_type bbegin_leaf = get_node(right.root_).locate(0);
    const id_type bbegin_parent = get_leaf(bbegin_leaf).parent;
    insert(bbegin_parent, 0, bbegin, right.root_);
    return right;
  }

  void merge(Base& right) {
    assert(this != &right);
    if (right.empty()) return;
    if (empty()) {
      swap(*this, right);
      return;
    }
    erase(get_leaf(end_.leaf_id_).parent, get_leaf(end_.leaf_id_).position,
          root_);
    erase(get_leaf(right.bbegin_.leaf_id_).parent,
          get_leaf(right.bbegin_.leaf_id_).position, right.root_);
    root_ = unite(root_, right.root_);
    end_ = right.end_;
    right.root_ = null_id;
    right.init();
  }

  void reverse() {
    get_node(root_).push_reverse();
    using std::swap;
    swap(bbegin_, end_);
  }
  void reverse(size_type first, size_type last) {
    assert(first <= last && last <= size());
    if (last - first < 2) return;
    if (first == 0 && last == size()) return reverse();
    reverse(nth(first), nth(last));
  }
  void reverse(iterator first, iterator last) {
    if (first == last) return;
    auto right = devide(root_, last.leaf_id_);
    auto middle = devide(root_, first.leaf_id_);
    get_node(middle).push_reverse();
    root_ = unite(root_, middle);
    root_ = unite(root_, right);
  }
  void debug_node_stats() const {
    std::cerr << "leaf slots in use: " << Pools::leaves().size() << '\n';
    std::cerr << "internal slots in use: " << Pools::internals().size() << '\n';
  }
};
}  // namespace Bplustree
