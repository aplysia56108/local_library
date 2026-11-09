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

#include "data-structure/object-pool/object_pool.hpp"

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

  friend typename Traits::Node;
  friend typename Traits::Leaf;
  friend typename Traits::Internal;
  friend Iterator<Traits, Base, true, true>;
  friend Iterator<Traits, Base, true, false>;
  friend Iterator<Traits, Base, false, true>;
  friend Iterator<Traits, Base, false, false>;

 protected:
  using T = value_type;
  using Node = typename Traits::Node;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  static constexpr size_type NodeCap = Traits::kNodeCapacity;
  static_assert(NodeCap >= 3);
  inline static leaf_pool_type& leaf_pool_ = Pools::leaves();
  inline static internal_pool_type& internal_pool_ = Pools::internals();

  // root_ が指す ID 木はこの Base が所有する。各 ID は同じ Traits の Pool で共有する。
  // 番兵も Leaf Pool 上にあり、Iterator は所有権を持たない。
  id_type root_ = null_id;
  iterator bbegin_, end_;

  static T init_value() {
    if constexpr (requires { Traits::identity(); })
      return Traits::identity();
    else
      return T{};
  };

  /** @brief 前後に番兵 Leaf を1つずつ持つ空の木を構築する。 */
  void init() {
    Pools::reserve_buffer(2, 1);
    const id_type bbegin = Pools::make_leaf(init_value());
    const id_type end = Pools::make_leaf(init_value());
    const id_type new_root = make_new_root(bbegin, end);
    Pools::recycle(root_, false);
    root_ = new_root;
    bbegin_ = iterator{bbegin};
    end_ = iterator{end};
  }

  /** @brief 子が1つだけの根を縮め、古い Internal を回収する。 */
  static void cut_root(id_type& root) {
    while (internal_pool_[root].count == 1 && internal_pool_[root].level) {
      internal_pool_[root].materialize();
      const id_type child = internal_pool_[root].children[0];
      internal_pool_[root].children[0] = null_id;
      internal_pool_[root].count = 0;
      internal_pool_.erase(root);
      root = child;
      internal_pool_[root].parent = null_id;
    }
  }

  void build(std::vector<T>& values) {
    if (values.empty()) return init();
    constexpr size_type MaxCount = NodeCap - 1;
    size_type n = values.size() + 2;
    size_type internal_required = 0;
    while (n > 1) {
      n = (n + MaxCount - 1) / MaxCount;
      internal_required += n;
    }
    Pools::reserve_buffer(values.size() + 2, internal_required);
    std::vector<id_type> level;
    level.reserve(values.size() + 2);
    const id_type bbegin = Pools::make_leaf(init_value());
    level.push_back(bbegin);
    for (const auto& value : values) level.push_back(Pools::make_leaf(value));
    const id_type end = Pools::make_leaf(init_value());
    level.push_back(end);

    pos_type current_level = 0;
    while (level.size() > 1) {
      std::vector<id_type> next;
      next.reserve((level.size() + MaxCount - 1) / MaxCount);
      for (size_type i = 0; i < level.size();) {
        const id_type parent_id = Pools::make_internal(current_level);
        auto& parent = internal_pool_[parent_id];
        while (parent.count < MaxCount && i < level.size())
          parent.children[parent.count++] = level[i++];
        parent.refresh();
        next.push_back(parent_id);
      }
      level = std::move(next);
      ++current_level;
    }
    const id_type new_root = level[0];
    internal_pool_[new_root].parent = null_id;
    Pools::recycle(root_, false);
    root_ = new_root;
    bbegin_ = iterator{bbegin};
    end_ = iterator{end};
  }

  static id_type make_new_root(id_type left, id_type right,
                               pos_type level = 0) {
    const id_type parent_id = Pools::make_internal(level);
    auto& parent = internal_pool_[parent_id];
    parent.insert(left, 0);
    parent.insert(right, 1);
    parent.refresh();
    parent.parent = null_id;
    return parent_id;
  }

  static void materialize_path(id_type node_id) {
    if (internal_pool_[node_id].parent != null_id)
      materialize_path(internal_pool_[node_id].parent);
    internal_pool_[node_id].materialize();
  }

  static void refresh_recursive(id_type node_id) {
    internal_pool_[node_id].refresh();
    if (internal_pool_[node_id].parent != null_id)
      refresh_recursive(internal_pool_[node_id].parent);
  }

  static id_type insert_recursive(id_type parent_id, pos_type index,
                                  id_type child = null_id) {
    if (parent_id == null_id) return null_id;
    if (child != null_id) {
      if (internal_pool_[parent_id].insert(child, index)) {
        const id_type upper = internal_pool_[parent_id].parent;
        const pos_type upper_pos = internal_pool_[parent_id].position;
        const id_type right_id = internal_pool_[parent_id].split();
        if (upper != null_id)
          return insert_recursive(upper, upper_pos + 1, right_id);
        return right_id;
      }
    }
    refresh_recursive(parent_id);
    return null_id;
  }

  template <bool Left>
  static std::vector<id_type> end_nodes(id_type root) {
    std::vector<id_type> nodes;
    nodes.reserve(internal_pool_[root].level + 1);
    id_type current = root;
    while (true) {
      internal_pool_[current].apply_reverse();
      nodes.push_back(current);
      if (internal_pool_[current].level == 0) break;
      current = Left ? internal_pool_[current].children[0]
                     : internal_pool_[current].children[
                           internal_pool_[current].count - 1];
    }
    return nodes;
  }

  static void cut_zero_node(id_type& root) {
    for (bool left : {true, false}) {
      id_type current = root;
      while (true) {
        internal_pool_[current].apply_reverse();
        const auto& node = internal_pool_[current];
        if (node.size == 0 && node.parent != null_id) {
          erase(node.parent, node.position, root);
          break;
        }
        if (node.level == 0) break;
        current = left ? node.children[0] : node.children[node.count - 1];
      }
    }
  }

  static void insert(id_type parent_id, pos_type index, id_type child,
                     id_type& root) {
    const id_type right_id = insert_recursive(parent_id, index, child);
    if (right_id != null_id) {
      const pos_type level = internal_pool_[right_id].level + 1;
      root = make_new_root(root, right_id, level);
    }
  }

  static void erase_recursive(id_type parent_id, pos_type index) {
    const id_type upper = internal_pool_[parent_id].parent;
    const pos_type upper_pos = internal_pool_[parent_id].position;
    if (internal_pool_[parent_id].erase(index)) {
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
    materialize_path(leaf_pool_[leaf_id].parent);
    id_type parent_id = leaf_pool_[leaf_id].parent;
    pos_type position = leaf_pool_[leaf_id].position;
    id_type right = null_id;
    while (parent_id != null_id) {
      const id_type next = internal_pool_[parent_id].split(position);
      if (right != null_id && internal_pool_[right].count)
        internal_pool_[next].insert(right, 0);
      right = next;
      position = internal_pool_[parent_id].position + 1;
      parent_id = internal_pool_[parent_id].parent;
    }
    cut_root(root);
    cut_zero_node(root);
    for (id_type id : end_nodes<false>(root)) internal_pool_[id].integrate();
    cut_root(right);
    cut_zero_node(right);
    for (id_type id : end_nodes<true>(right)) internal_pool_[id].integrate();
    return right;
  }

  static id_type unite(id_type left, id_type right) {
    if (internal_pool_[right].size == 0) {
      Pools::recycle(right, false);
      return left;
    }
    if (internal_pool_[left].size == 0) {
      Pools::recycle(left, false);
      return right;
    }
    auto integrate_nodes = end_nodes<false>(left);
    for (id_type id : end_nodes<true>(right)) integrate_nodes.push_back(id);
    if (internal_pool_[left].level == internal_pool_[right].level) {
      const pos_type level = internal_pool_[left].level + 1;
      id_type root = make_new_root(left, right, level);
      for (id_type id : integrate_nodes) internal_pool_[id].integrate();
      cut_root(root);
      return root;
    }
    if (internal_pool_[left].level < internal_pool_[right].level) {
      id_type target = right;
      while (internal_pool_[left].level + 1 < internal_pool_[target].level) {
        internal_pool_[target].materialize();
        target = internal_pool_[target].children[0];
      }
      internal_pool_[target].materialize();
      insert(target, 0, left, right);
      for (id_type id : integrate_nodes) internal_pool_[id].integrate();
      return right;
    }
    id_type target = left;
    while (internal_pool_[right].level + 1 < internal_pool_[target].level) {
      internal_pool_[target].materialize();
      target = internal_pool_[target].children[internal_pool_[target].count - 1];
    }
    internal_pool_[target].materialize();
    insert(target, internal_pool_[target].count, right, left);
    for (id_type id : integrate_nodes) internal_pool_[id].integrate();
    return left;
  }

  template <bool WithLazy = false>
  id_type locate(size_type index) const {
    ++index;
    assert(index < internal_pool_[root_].size);
    return internal_pool_[root_].template locate<WithLazy>(index);
  }

  static iterator mutable_iterator(const const_iterator& it) {
    return iterator{it.leaf_id_};
  }

 public:
  Base() { init(); }
  ~Base() { Pools::recycle(root_, false); }
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
    return internal_pool_[root_].size - 2;
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
    Pools::reserve_buffer(1, internal_pool_[root_].level + 3);
    materialize_path(leaf_pool_[pos.leaf_id_].parent);
    const id_type parent = leaf_pool_[pos.leaf_id_].parent;
    const pos_type position = leaf_pool_[pos.leaf_id_].position;
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
    materialize_path(leaf_pool_[pos.leaf_id_].parent);
    id_type parent = leaf_pool_[pos.leaf_id_].parent;
    pos_type position = leaf_pool_[pos.leaf_id_].position;
    bool erase_flg = true;
    while (parent != null_id) {
      if (erase_flg) {
        erase_flg = internal_pool_[parent].erase(position);
        position = internal_pool_[parent].position;
      } else {
        internal_pool_[parent].refresh();
      }
      parent = internal_pool_[parent].parent;
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
    Pools::reserve_buffer(0, 2 * (internal_pool_[root_].level + 3));
    const id_type right = devide(root_, last.leaf_id_);
    const id_type removed = devide(root_, first.leaf_id_);
    root_ = unite(root_, right);
    Pools::recycle(removed, false);
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
    Pools::reserve_buffer(2, 2 * (internal_pool_[root_].level + 3));
    const id_type old_right_root = right.root_;
    right.root_ = devide(root_, pos.leaf_id_);
    Pools::recycle(old_right_root, false);
    const id_type bbegin = Pools::make_leaf(init_value());
    const id_type end = Pools::make_leaf(init_value());
    right.end_ = end_;
    right.bbegin_ = iterator{bbegin};
    end_ = iterator{end};
    const id_type end_leaf = internal_pool_[root_].locate(internal_pool_[root_].size - 1);
    const id_type end_parent = leaf_pool_[end_leaf].parent;
    insert(end_parent, internal_pool_[end_parent].count, end, root_);
    const id_type bbegin_leaf = internal_pool_[right.root_].locate(0);
    const id_type bbegin_parent = leaf_pool_[bbegin_leaf].parent;
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
    Pools::reserve_buffer(0, 2 * (internal_pool_[root_].level +
                                  internal_pool_[right.root_].level + 3));
    const id_type end_leaf = internal_pool_[root_].locate(internal_pool_[root_].size - 1);
    erase(leaf_pool_[end_leaf].parent, leaf_pool_[end_leaf].position, root_);
    const id_type bbegin_leaf = internal_pool_[right.root_].locate(0);
    erase(leaf_pool_[bbegin_leaf].parent, leaf_pool_[bbegin_leaf].position, right.root_);
    root_ = unite(root_, right.root_);
    end_ = right.end_;
    right.root_ = null_id;
    right.init();
  }

  void reverse() {
    internal_pool_[root_].push_reverse();
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
    Pools::reserve_buffer(0, 2 * (internal_pool_[root_].level + 3));
    auto right = devide(root_, last.leaf_id_);
    auto middle = devide(root_, first.leaf_id_);
    internal_pool_[middle].push_reverse();
    root_ = unite(root_, middle);
    root_ = unite(root_, right);
  }
  void debug_node_stats() const {
    std::cerr << "leaf slots in use: " << leaf_pool_.size() << '\n';
    std::cerr << "internal slots in use: " << internal_pool_.size() << '\n';
  }
};
}  // namespace Bplustree
