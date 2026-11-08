#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#include "data-structure/object-pool/object_pool.hpp"

namespace Bplustree {

using pos_type = std::uint8_t;

template <class Traits>
struct Node;
template <class Traits>
struct Leaf;
template <class Traits>
struct Internal;

/** @brief Leaf と Internal の ID 空間を分けて共有する Pool。 */
template <class Traits>
struct NodePools {
  using id_type = std::uint32_t;
  using L = typename Traits::Leaf;
  using I = typename Traits::Internal;
  static constexpr id_type null_id = ObjectPool<L>::null_id;

  static ObjectPool<L>& leaves() {
    static ObjectPool<L> pool;
    return pool;
  }
  static ObjectPool<I>& internals() {
    static ObjectPool<I> pool;
    return pool;
  }
  template <class... Args>
  static id_type make_leaf(Args&&... args) {
    auto id = leaves().emplace(std::forward<Args>(args)...);
    return id;
  }
  static id_type make_internal(pos_type level = 0) {
    auto id = internals().emplace(level);
    return id;
  }
  /** @brief 指定部分木の全スロットを再利用可能にする。O(部分木サイズ)。 */
  static void recycle(id_type id, bool is_leaf) {
    if (id == null_id) return;
    if (is_leaf) {
      leaves().erase(id);
    } else {
      auto& node = internals()[id];
      for (pos_type i = 0; i < node.count; ++i)
        recycle(node.children[i], node.level == 0);
      internals().erase(id);
    }
  }
};

template <class value_type, std::size_t Capacity = 32>
struct Traits {
  static_assert(3 <= Capacity && Capacity <= 255);
  using T = value_type;
  static constexpr pos_type kNodeCapacity = Capacity;
  static constexpr pos_type kNodeHalf = Capacity / 2;
  using Node = Bplustree::Node<Traits>;
  using Leaf = Bplustree::Leaf<Traits>;
  using Internal = Bplustree::Internal<Traits>;
};

/**
 * @brief 共通の親 ID と親内位置を保持する。
 * @details ID は各型の Pool 内で有効。自身の Pool ID は self_id に保持する。
 * ノードは ObjectPool の再確保で移動するため、Node* を保持しない。
 */
template <class Traits>
struct Node {
  using id_type = typename NodePools<Traits>::id_type;
  static constexpr id_type null_id = NodePools<Traits>::null_id;

  id_type parent = null_id;
  pos_type position = 0;
};

/** @brief 1要素1Leaf。安定した要素識別子は Leaf Pool 内の ID。 */
template <class Traits>
struct Leaf : Traits::Node {
  using T = typename Traits::T;
  using N = typename Traits::Node;

 private:
  T value;

 public:
  explicit Leaf(const T& v) : N(), value(v) {}
  void set(const T& v) {
    value = v;
  }
  const T& get() const {
    return value;
  }
  T& get() {
    return value;
  }
};

/**
 * @brief children は level==0 なら Leaf ID、それ以外は Internal ID。
 * @details 通常 count < NodeCap。挿入直後のみ count == NodeCap があり得る。
 * 所有権は Pool の ID 木にあり、子 ID の移動は実体を移動しない。
 */
template <class Traits>
struct Internal : Traits::Node {
  using pos_type = std::uint8_t;

  static constexpr pos_type NodeCap = Traits::kNodeCapacity;
  static constexpr pos_type HalfNodeCap = NodeCap / 2;
  using N = typename Traits::Node;
  using L = typename Traits::Leaf;
  using I = typename Traits::Internal;
  using Pools = NodePools<Traits>;
  using id_type = typename Pools::id_type;

  static constexpr id_type null_id = Pools::null_id;

  pos_type count = 0;
  std::size_t size = 0;
  pos_type level = 0;
  bool is_reversed = false;
  std::array<id_type, NodeCap> children{};

  explicit Internal(pos_type lvl = 0) : N(), level(lvl) {
    children.fill(null_id);
  }

  L& get_leaf(pos_type index) const {
    assert(level == 0 && index < count);
    return Pools::leaves()[children[index]];
  }
  I& get_child(pos_type index) const {
    assert(level > 0 && index < count);
    return Pools::internals()[children[index]];
  }
  std::size_t child_size(pos_type index) const {
    return level == 0 ? 1 : get_child(index).size;
  }
  pos_type child_count(pos_type index) const {
    return level == 0 ? 1 : get_child(index).count;
  }

  /** @brief count / size / parent / position を子 ID から再構築する。 */
  void sync(id_type self_id) {
    size = 0;
    count = 0;
    for (pos_type i = 0; i < NodeCap; ++i) {
      if (children[i] == null_id) break;
      if (level == 0) {
        auto& child = get_leaf(i);
        child.parent = self_id;
        child.position = i;
        ++size;
      } else {
        auto& child = get_child(i);
        child.parent = self_id;
        child.position = i;
        size += child.size;
      }
      ++count;
    }
  }
  void after_sync() {}
  void refresh(id_type self_id) {
    sync(self_id);
    static_cast<I*>(this)->after_sync(self_id);
  }
  void apply_reverse(id_type self_id) {
    if (!is_reversed) return;
    std::reverse(children.begin(), children.begin() + count);
    is_reversed = false;
    if (level)
      for (pos_type i = 0; i < count; ++i) get_child(i).push_reverse();
    refresh(self_id);
  }
  void push_reverse() {
    is_reversed = !is_reversed;
    static_cast<I*>(this)->push_reverse_impl();
  }
  void push_reverse_impl() {}
  void apply_lazy() {}
  void materialize(id_type self_id) {
    apply_reverse(self_id);
    static_cast<I*>(this)->apply_lazy();
  }

  /** @return 満杯になり直ちに split が必要なら true。 */
  bool insert(id_type child, pos_type index, id_type self_id) {
    assert(child != null_id && index <= count && count < NodeCap);
    for (pos_type i = count; i > index; --i) children[i] = children[i - 1];
    children[index] = child;
    ++count;
    if (count == NodeCap) return true;
    refresh(self_id);
    return false;
  }

  /** @return Pool に確保された右兄弟の Internal ID。 */
  id_type split(id_type self_id, pos_type left_count = HalfNodeCap) {
    assert(count <= NodeCap && left_count <= count);
    assert(left_count < NodeCap && count - left_count < NodeCap);
    const pos_type right_count = count - left_count;
    // emplace は vector を再確保し得る。this の情報を確保前に退避する。
    const id_type left_id = self_id;
    const pos_type current_level = level;
    id_type right_id = Pools::make_internal(current_level);
    auto& left = Pools::internals()[left_id];
    auto& right = Pools::internals()[right_id];
    for (pos_type i = 0; i < right_count; ++i) {
      right.children[i] = left.children[left_count + i];
      left.children[left_count + i] = null_id;
    }
    left.refresh(left_id);
    right.refresh(right_id);
    return right_id;
  }

  /** @return この Internal が空になった場合 true。 */
  bool erase(pos_type index, id_type self_id) {
    assert(count < NodeCap && index < count);
    Pools::recycle(children[index], level == 0);
    for (pos_type i = index + 1; i < count; ++i) children[i - 1] = children[i];
    children[count - 1] = null_id;
    refresh(self_id);
    return count == 0;
  }

  template <bool WithLazy = false>
  id_type locate(std::size_t index, id_type self_id) {
    apply_reverse(self_id);
    if constexpr (WithLazy) static_cast<I*>(this)->apply_lazy();
    if (level == 0) {
      assert(index < count);
      return children[index];
    }
    for (pos_type i = 0; i < count; ++i) {
      auto& child = get_child(i);
      if (index < child.size)
        return child.template locate<WithLazy>(index, children[i]);
      index -= child.size;
    }
    assert(false);
    return null_id;
  }

  /** @brief index の Internal 子を隣の兄弟へ移動し、元の子を回収する。 */
  void integrate_child(pos_type index, id_type self_id) {
    assert(level > 0 && index < count);
    if (index && child_count(index - 1) + child_count(index) < NodeCap) {
      auto& left = get_child(index - 1);
      id_type left_id = children[index - 1];
      auto& right = get_child(index);
      id_type right_id = children[index];

      left.materialize(left_id);
      right.materialize(right_id);
      const auto left_count = left.count;
      for (pos_type i = 0; i < right.count; ++i) {
        left.children[left_count + i] = right.children[i];
        right.children[i] = null_id;
      }
      right.count = 0;
      left.refresh(left_id);
    } else if (index + 1 < count &&
               child_count(index + 1) + child_count(index) < NodeCap) {
      auto& left = get_child(index);
      id_type left_id = children[index];
      auto& right = get_child(index + 1);
      id_type right_id = children[index + 1];

      left.materialize(left_id);
      right.materialize(right_id);
      const auto left_count = left.count;
      for (pos_type i = right.count; i > 0; --i)
        right.children[left_count + i - 1] = right.children[i - 1];
      for (pos_type i = 0; i < left_count; ++i) {
        right.children[i] = left.children[i];
        left.children[i] = null_id;
      }
      left.count = 0;
      right.refresh(right_id);
    } else {
      return;
    }
    erase(index, self_id);
  }

  /** @brief 統合成功時は自身の ID が無効になる。 */
  void integrate() {
    if (this->parent == null_id) return;
    const auto parent_id = this->parent;
    const auto position = this->position;
    Pools::internals()[parent_id].integrate_child(position, parent_id);
  }
};

}  // namespace Bplustree
