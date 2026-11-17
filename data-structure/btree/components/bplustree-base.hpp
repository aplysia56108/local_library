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
  using const_iterator = Iterator<Traits, Base, true>;
  using iterator = Iterator<Traits, Base, false>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using setter_type = Setter<Base>;
  using Pools = NodePools<Traits>;
  using Epoch = IteratorEpoch<Traits>;
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

  template <bool IsConst>
  static id_type get_id(const Iterator<Traits, Base, IsConst>& it) {
    return it.leaf_id_;
  }

  /** @brief 前後に番兵 Leaf を1つずつ持つ空の木を構築する。 */
  void init() {
    const id_type bbegin = Pools::make_leaf(init_value());
    const id_type end = Pools::make_leaf(init_value());
    const id_type new_root = make_new_root(bbegin, end);
    Pools::recycle(root_);
    root_ = new_root;
    bbegin_ = iterator{bbegin};
    end_ = iterator{end};
    Epoch::invalidate_all();
  }

  /** @brief 子が1つだけの根を縮め、古い Internal を回収する。 */
  static void cut_root(id_type& root) {
    while (get_node(root).count == 1 && get_node(root).level) {
      auto& node = get_node(root);
      node.materialize();
      const id_type child = node.children[0];
      // 根の昇格は子の親を null にする特別な所有権移動。
      // 回収時に子まで解放されないよう、旧根の count を先に0にする。
      node.count = 0;
      node.size = 0;
      Pools::recycle(root);
      root = child;
      get_node(root).parent = null_id;
      get_node(root).position = 0;
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
    id_type previous_bottom = null_id;
    while (nodes.size() > 1) {
      std::vector<id_type> next;
      next.reserve((nodes.size() + MaxCount - 1) / MaxCount);
      for (size_type i = 0; i < nodes.size();) {
        const id_type parent_id = Pools::make_internal(current_level);
        auto& parent = get_node(parent_id);
        while (parent.count < MaxCount && i < nodes.size())
          parent.template move_child<true, false>(parent.count, nodes[i++], parent_id);
        parent.refresh();
        if (current_level == 0) {
          if (previous_bottom != null_id)
            Internal::connect(previous_bottom, 1, parent_id, 0);
          previous_bottom = parent_id;
        }
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
    Epoch::invalidate_all();
  }

  static id_type make_new_root(id_type left, id_type right,
                               pos_type level = 0) {
    const id_type parent_id = Pools::make_internal(level);
    auto& parent = get_node(parent_id);
    parent.template move_child<true, false>(0, left, parent_id);
    parent.template move_child<true, false>(1, right, parent_id);
    parent.refresh();
    return parent_id;
  }

  static void materialize_path(id_type node_id) {
    auto& node = get_node(node_id);
    if (node.parent != null_id) materialize_path(node.parent);
    node.materialize();
  }


  /** @brief 下位の構造変更に伴う要素数の増減を祖先へ伝える。 */
  static void add_size_delta(Internal& node, difference_type delta) {
    if (delta >= 0) {
      node.size += static_cast<size_type>(delta);
    } else {
      const size_type removed = static_cast<size_type>(-delta);
      assert(node.size >= removed);
      node.size -= removed;
    }
  }

  static difference_type size_delta(size_type before, size_type after) {
    return after >= before ? static_cast<difference_type>(after - before)
                           : -static_cast<difference_type>(before - after);
  }

  static void refresh_recursive(id_type node_id, difference_type delta = 0) {
    auto& node = get_node(node_id);
    add_size_delta(node, delta);
    node.refresh();
    if (node.parent != null_id) refresh_recursive(node.parent, delta);
  }

  static id_type insert_recursive(id_type parent_id, pos_type index,
                                  id_type child = null_id,
                                  difference_type delta = 0) {
    if (parent_id == null_id) return null_id;
    auto& parent = get_node(parent_id);
    const size_type old_size = parent.size;
    // 下位の分割により既存の左子のサイズが変化している分を先に補正する。
    add_size_delta(parent, delta);
    if (child != null_id) {
      if (parent.insert(child, index)) {
        const id_type upper_id = parent.parent;
        const pos_type upper_pos = parent.position;
        const id_type right_id = parent.split();
        if (upper_id != null_id) {
          // 左子の変化量と新しい右子の挿入量を足して祖先へ伝える。
          return insert_recursive(upper_id, upper_pos + 1, right_id,
                                  size_delta(old_size, get_node(parent_id).size));
        }
        return right_id;
      }
    } else {
      parent.refresh();
    }
    if (parent.parent != null_id)
      refresh_recursive(parent.parent, size_delta(old_size, parent.size));
    return null_id;
  }

  /** @brief 境界経路を取得する。子ノードを統合する前に lazy も伝搬する。 */
  template <bool Left>
  static std::vector<id_type> end_nodes(id_type root) {
    std::vector<id_type> nodes;
    nodes.reserve(get_node(root).level + 1);
    id_type current_id = root;
    while (true) {
      auto& current = get_node(current_id);
      current.materialize();
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
    for (bool left : {true, false}) {
      id_type node_id = root;

      while (true) {
        auto& node = get_node(node_id);
        node.materialize();
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


  static void erase_recursive(id_type parent_id, pos_type index,
                              difference_type delta = 0) {
    auto& parent = get_node(parent_id);
    const size_type old_size = parent.size;
    const id_type upper = parent.parent;
    const pos_type upper_pos = parent.position;
    // 空の子を回収するときも、下位で失った分のsizeを反映させる。
    add_size_delta(parent, delta);
    const bool empty = parent.erase(index);
    const difference_type change = size_delta(old_size, parent.size);
    if (empty) {
      if (upper != null_id) erase_recursive(upper, upper_pos, change);
    } else if (upper != null_id) {
      refresh_recursive(upper, change);
    }
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
    size_type removed_from_boundary = 0;
    while (parent_id != null_id) {
      // 下位で分離した右半分は、上位のsizeにはまだ含まれている。
      // syncせずに済むよう、上位をsplitする前に差し引く。
      auto& parent = get_node(parent_id);
      assert(parent.size >= removed_from_boundary);
      parent.size -= removed_from_boundary;
      const id_type next = parent.split(position);
      if (right != null_id) {
        if (get_node(right).count)
          get_node(next).insert(right, 0);
        else
          Pools::recycle(right);
      }
      right = next;
      removed_from_boundary = get_node(right).size;
      position = get_node(parent_id).position + 1;
      parent_id = get_node(parent_id).parent;
    }
    cut_root(root);
    cut_zero_node(root);
    for (id_type id : end_nodes<false>(root)) get_node(id).integrate();
    cut_root(right);
    cut_zero_node(right);
    for (id_type id : end_nodes<true>(right)) get_node(id).integrate();
    // The two resulting trees must have independent bottom-node paths.
    // Both boundaries have been materialized, so ports 1/0 are their
    // physical right/left ends respectively.
    const id_type left_tail = end_nodes<false>(root).back();
    const id_type right_head = end_nodes<true>(right).back();
    Internal::connect(left_tail, 1, null_id, 0);
    Internal::connect(right_head, 0, null_id, 0);
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
    const auto right_edge = end_nodes<true>(right_id);
    // Splice the two bottom-node paths before internal integration.
    Internal::connect(integrate_nodes.back(), 1, right_edge.back(), 0);
    for (id_type id : right_edge) integrate_nodes.push_back(id);
    if (left.level == right.level) {
      id_type root = make_new_root(left_id, right_id, left.level + 1);
      for (id_type id : integrate_nodes) get_node(id).integrate();
      cut_root(root);
      return root;
    }
    if (left.level < right.level) {
      auto target_id = right_id;
      while (left.level + 1 < get_node(target_id).level) {
        get_node(target_id).materialize();
        target_id = get_node(target_id).children[0];
      }
      get_node(target_id).materialize();
      insert(target_id, 0, left_id, right_id);
      for (id_type id : integrate_nodes) get_node(id).integrate();
      return right_id;
    }
    auto target_id = left_id;
    while (right.level + 1 < get_node(target_id).level) {
      auto& target = get_node(target_id);
      target.materialize();
      target_id = target.children[target.count - 1];
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

  /** @brief locateが保証した経路の検証状態をiteratorへ引き継ぐ。 */
  template <bool WithLazy = false>
  iterator locate_iterator(size_type index) const {
    return iterator{
        locate<WithLazy>(index),
        WithLazy ? iterator::Validation::Value
                 : iterator::Validation::Position};
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
    Epoch::invalidate_all();
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
    return locate_iterator(index);
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
    // 探索中に未伝搬のreverse/lazyを実体化し、そのLeafを直接参照する。
    // 一時Iteratorの生成と参照時の根への再走査を省略する。
    return get_leaf(locate<true>(index)).get();
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
    return {locate_iterator(index)};
  }

  void set(size_type index, const T& value) {
    nth(index).set(value);
  }

  iterator insert(size_type index, const T& value) {
    return insert(locate_iterator<true>(index), value);
  }
  iterator insert(iterator pos, const T& value) {
    auto& leaf = get_leaf(pos.leaf_id_);
    materialize_path(leaf.parent);
    const id_type parent = leaf.parent;
    const pos_type position = leaf.position;
    const id_type child = Pools::make_leaf(value);
    insert(parent, position, child, root_);
    Epoch::invalidate_all();
    return iterator{child};
  }

  void push_back(const T& value) {
    insert(end(), value);
  }

  void push_front(const T& value) {
    insert(begin(), value);
  }

  iterator erase(size_type index) {
    return erase(locate_iterator(index));
  }

  iterator erase(iterator pos) {
    iterator res = pos + 1;
    auto& leaf = get_leaf(pos.leaf_id_);
    materialize_path(leaf.parent);
    id_type parent = leaf.parent;
    pos_type position = leaf.position;
    erase(parent, position, root_);
    Epoch::invalidate_all();
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
    Epoch::invalidate_all();
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
    Epoch::invalidate_all();
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
    right.init();  // init()で共有epochを更新する。
  }

  void reverse() {
    get_node(root_).push_reverse();
    using std::swap;
    swap(bbegin_, end_);
    Epoch::invalidate_position();
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
    // 部分反転はsplit/mergeでLazy経路も再構成するため両方を無効化。
    Epoch::invalidate_all();
  }
  void debug_node_stats() const {
    std::cerr << "leaf slots in use: " << Pools::leaves().size() << '\n';
    std::cerr << "internal slots in use: " << Pools::internals().size() << '\n';
  }
};
}  // namespace Bplustree
