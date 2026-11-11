#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#include "data-structure/btree/components/bplustree-node-pools.hpp"

namespace Bplustree {

using pos_type = std::uint8_t;

template <class Traits>
struct Node;
template <class Traits>
struct Leaf;
template <class Traits>
struct Internal;

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
 * @details ID は各型の Pool 内で有効。自身の ID は Pool::index() で逆引きする。
 * Pool の拡張は参照を無効化するため、確保を伴う外部操作前に reserve する。
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

  /** @brief この Internal の Pool ID を取得する。Pool
   * 再確保前の参照だけが有効。 */
  id_type id() const {
    return Pools::internals().index(*static_cast<const I*>(this));
  }

  /** @brief count / size / parent / position を子 ID から再構築する。 */
  void sync() {
    const id_type self_id = id();
    size = 0;
    count = 0;
    for (std::size_t i = 0; i < NodeCap; ++i) {
      if (children[i] == null_id) break;
      if (level == 0) {
        auto& child = Pools::leaves()[children[i]];
        child.parent = self_id;
        child.position = i;
        ++size;
      } else {
        auto& child = Pools::internals()[children[i]];
        child.parent = self_id;
        child.position = i;
        size += child.size;
      }
      ++count;
    }
  }
  void after_sync() {}
  void refresh() {
    sync();
    static_cast<I*>(this)->after_sync();
  }
  void apply_reverse() {
    if (!is_reversed) return;
    std::reverse(children.begin(), children.begin() + count);
    is_reversed = false;
    if (level)
      for (pos_type i = 0; i < count; ++i) get_child(i).push_reverse();
    refresh();
  }
  void push_reverse() {
    is_reversed = !is_reversed;
    static_cast<I*>(this)->push_reverse_impl();
  }
  void push_reverse_impl() {}
  void apply_lazy() {}
  void materialize() {
    apply_reverse();
    static_cast<I*>(this)->apply_lazy();
  }

  /** @return 満杯になり直ちに split が必要なら true。 */
  bool insert(id_type child, pos_type index) {
    assert(child != null_id && index <= count && count < NodeCap);
    for (pos_type i = count; i > index; --i) children[i] = children[i - 1];
    children[index] = child;
    ++count;
    if (count == NodeCap) return true;
    refresh();
    return false;
  }

  /** @return Pool に確保された右兄弟の Internal ID。 */
  id_type split(pos_type left_count = HalfNodeCap) {
    assert(count <= NodeCap && left_count <= count);
    assert(left_count < NodeCap && count - left_count < NodeCap);
    const pos_type right_count = count - left_count;
    // emplace は vector を再確保し得る。ID と level を確保前に退避する。
    const id_type left_id = id();
    const pos_type current_level = level;
    id_type right_id = Pools::make_internal(current_level);
    auto& left = Pools::internals()[left_id];
    auto& right = Pools::internals()[right_id];
    for (pos_type i = 0; i < right_count; ++i) {
      right.children[i] = left.children[left_count + i];
      left.children[left_count + i] = null_id;
    }
    left.refresh();
    right.refresh();
    return right_id;
  }

  /** @return この Internal が空になった場合 true。 */
  bool erase(pos_type index) {
    assert(count < NodeCap && index < count);
    Pools::recycle(children[index], level == 0);
    for (pos_type i = index + 1; i < count; ++i) children[i - 1] = children[i];
    children[count - 1] = null_id;
    refresh();
    return count == 0;
  }

  template <bool WithLazy = false>
  id_type locate(std::size_t index) {
    apply_reverse();
    if constexpr (WithLazy) static_cast<I*>(this)->apply_lazy();
    if (level == 0) {
      assert(index < count);
      return children[index];
    }
    for (pos_type i = 0; i < count; ++i) {
      auto& child = get_child(i);
      if (index < child.size) return child.template locate<WithLazy>(index);
      index -= child.size;
    }
    assert(false);
    return null_id;
  }

  /** @brief index の Internal 子を隣の兄弟へ移動し、元の子を回収する。 */
  void integrate_child(pos_type index) {
    assert(level > 0 && index < count);
    if (index && child_count(index - 1) + child_count(index) < NodeCap) {
      auto& left = get_child(index - 1);
      auto& right = get_child(index);

      left.materialize();
      right.materialize();
      const auto left_count = left.count;
      for (pos_type i = 0; i < right.count; ++i) {
        left.children[left_count + i] = right.children[i];
        right.children[i] = null_id;
      }
      right.count = 0;
      left.refresh();
    } else if (index + 1 < count &&
               child_count(index + 1) + child_count(index) < NodeCap) {
      auto& left = get_child(index);
      auto& right = get_child(index + 1);

      left.materialize();
      right.materialize();
      const auto left_count = left.count;
      for (pos_type i = right.count; i > 0; --i)
        right.children[left_count + i - 1] = right.children[i - 1];
      for (pos_type i = 0; i < left_count; ++i) {
        right.children[i] = left.children[i];
        left.children[i] = null_id;
      }
      left.count = 0;
      right.refresh();
    } else {
      return;
    }
    erase(index);
  }

  /** @brief 統合成功時は自身の ID が無効になる。 */
  void integrate() {
    if (this->parent == null_id) return;
    const auto parent_id = this->parent;
    const auto position = this->position;
    Pools::internals()[parent_id].integrate_child(position);
  }
};

}  // namespace Bplustree
