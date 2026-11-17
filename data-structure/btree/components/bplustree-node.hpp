#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
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
template <class Traits>
struct BottomInternal;

template <class value_type, std::size_t Capacity = 32>
struct Traits {
  static_assert(3 <= Capacity && Capacity <= 255);
  using T = value_type;
  static constexpr bool lazy = false;
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
 * @details children[0, count) のみ有効で、count 以降の内容は未規定。
 * 通常 count < NodeCap。挿入直後のみ count == NodeCap があり得る。
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
  // count 以降は参照しない。null_id 終端の管理は不要。
  std::array<id_type, NodeCap> children{};

  // Only level-0 nodes use these ports. A port always denotes a physical
  // end of children[], independent of the logical direction of traversal.
  struct Link {
    id_type node = null_id;
    pos_type port = 0;
  };

  /** @brief BottomInternal-only fields live in their separate Pool. */
  BottomInternal<Traits>& bottom() {
    assert(level == 0);
    return static_cast<BottomInternal<Traits>&>(*static_cast<I*>(this));
  }
  const BottomInternal<Traits>& bottom() const {
    assert(level == 0);
    return static_cast<const BottomInternal<Traits>&>(*static_cast<const I*>(this));
  }

  explicit Internal(pos_type lvl = 0) : N(), level(lvl) {}

  L& get_leaf(pos_type index) const {
    assert(level == 0 && index < count);
    return Pools::leaves()[children[index]];
  }
  I& get_child(pos_type index) const {
    assert(level > 0 && index < count);
    return Pools::internals()[children[index]];
  }
  template <bool IsBottom>
  std::size_t child_size_typed(pos_type index) const {
    if constexpr (IsBottom) return 1;
    return get_child(index).size;
  }
  template <bool IsBottom>
  pos_type child_count_typed(pos_type index) const {
    if constexpr (IsBottom) return 1;
    return get_child(index).count;
  }
  std::size_t child_size(pos_type index) const {
    return level == 0 ? child_size_typed<true>(index)
                      : child_size_typed<false>(index);
  }
  pos_type child_count(pos_type index) const {
    return level == 0 ? child_count_typed<true>(index)
                      : child_count_typed<false>(index);
  }

  /** @brief この Internal の Pool ID を取得する。Pool
   * 再確保前の参照だけが有効。 */
  id_type id() const {
    return Pools::internals().index(*static_cast<const I*>(this));
  }


  /** @brief Connect two free ports symmetrically. Either endpoint may be null. */
  static void connect(id_type a, pos_type a_port, id_type b,
                      pos_type b_port) {
    if (a != null_id) Pools::bottom(a).links[a_port] = {b, b_port};
    if (b != null_id) Pools::bottom(b).links[b_port] = {a, a_port};
  }

  /** @brief Insert a new bottom node immediately after this bottom node. */
  void link_after(id_type right_id) {
    assert(level == 0 && right_id != null_id);
    const id_type self = id();
    const Link successor = bottom().links[1];
    connect(self, 1, right_id, 0);
    connect(right_id, 1, successor.node, successor.port);
  }

  /** @brief Remove a bottom node, joining its two surviving neighbors. */
  void unlink() {
    assert(level == 0);
    const Link left = bottom().links[0], right = bottom().links[1];
    connect(left.node, left.port, right.node, right.port);
    bottom().links[0] = bottom().links[1] = {};
  }

  /**
   * @brief 子を配置し、count / size / parent / position を差分更新する。
   * @details FromOther=false は同一親内の位置変更（size不変）。
   * ConsiderOtherSync=true は移動元のsuffixを切り離す操作に限定する。
   * 新しい子や別木の根は ConsiderOtherSync=false で配置する。
   * 一括移動中は一時的に穴を許すが、完了するまで探索しない。
   */
  /** @brief 型が既知のノードについて Leaf/Internal 子の処理を静的に分ける。 */
  template <bool IsBottom, bool FromOther = true,
            bool ConsiderOtherSync = true>
  void move_child_typed(pos_type index, id_type child_id,
                        id_type self_id = null_id) {
    assert(index < NodeCap && child_id != null_id);
    children[index] = child_id;
    if (index >= count) count = index + 1;

    if constexpr (IsBottom) {
      auto& child = Pools::leaves()[child_id];
      if constexpr (FromOther) {
        ++size;
        if constexpr (ConsiderOtherSync) {
          assert(child.parent != null_id);
          auto& other = Pools::bottom(child.parent);
          assert(static_cast<const void*>(&other) != static_cast<const void*>(this)
                 && other.size > 0);
          --other.size;
          if (child.position < other.count) other.count = child.position;
        }
        child.parent = self_id == null_id ? id() : self_id;
      }
      child.position = index;
    } else {
      auto& child = Pools::internals()[child_id];
      if constexpr (FromOther) {
        size += child.size;
        if constexpr (ConsiderOtherSync) {
          assert(child.parent != null_id);
          auto& other = Pools::internals()[child.parent];
          assert(&other != this && other.size >= child.size);
          other.size -= child.size;
          if (child.position < other.count) other.count = child.position;
        }
        child.parent = self_id == null_id ? id() : self_id;
      }
      child.position = index;
    }
  }

  template <bool FromOther = true, bool ConsiderOtherSync = true>
  void move_child(pos_type index, id_type child_id,
                  id_type self_id = null_id) {
    if (level == 0)
      move_child_typed<true, FromOther, ConsiderOtherSync>(
          index, child_id, self_id);
    else
      move_child_typed<false, FromOther, ConsiderOtherSync>(
          index, child_id, self_id);
  }

  /** @brief 構造情報を再走査せず、派生クラスの集約情報を更新する。 */
  void refresh_metadata() {}
  /**
   * @brief metadata と reverse/lazy の経路をノード種別で静的に分ける。
   */
  template <bool IsBottom>
  void refresh_typed() {
    if constexpr (requires(I& node) {
                    node.template refresh_metadata_typed<IsBottom>();
                  }) {
      static_cast<I*>(this)->template refresh_metadata_typed<IsBottom>();
    } else {
      static_cast<I*>(this)->refresh_metadata();
    }
  }
  void refresh() {
    if (level == 0)
      refresh_typed<true>();
    else
      refresh_typed<false>();
  }

  template <bool IsBottom>
  void apply_reverse_typed() {
    if (!is_reversed) return;
    std::reverse(children.begin(), children.begin() + count);
    is_reversed = false;

    if constexpr (IsBottom) {
      auto& data = bottom();
      std::swap(data.links[0], data.links[1]);
      data.ports_swapped = !data.ports_swapped;
      const id_type self = id();
      for (pos_type port = 0; port < 2; ++port) {
        const Link next = data.links[port];
        if (next.node != null_id)
          Pools::bottom(next.node).links[next.port] = {self, port};
      }
    }

    for (pos_type i = 0; i < count; ++i) {
      this->template move_child_typed<IsBottom, false>(i, children[i]);
      if constexpr (!IsBottom) get_child(i).push_reverse();
    }
  }
  void apply_reverse() {
    if (level == 0)
      apply_reverse_typed<true>();
    else
      apply_reverse_typed<false>();
  }
  void push_reverse() {
    is_reversed = !is_reversed;
    static_cast<I*>(this)->push_reverse_impl();
  }
  void push_reverse_impl() {}
  void apply_lazy() {}

  template <bool IsBottom>
  void materialize_typed() {
    apply_reverse_typed<IsBottom>();
    if constexpr (requires(I& node) {
                    node.template apply_lazy_typed<IsBottom>();
                  }) {
      static_cast<I*>(this)->template apply_lazy_typed<IsBottom>();
    } else {
      static_cast<I*>(this)->apply_lazy();
    }
  }
  void materialize() {
    if (level == 0)
      materialize_typed<true>();
    else
      materialize_typed<false>();
  }

  /** @return 満杯になり直ちに split が必要なら true。 */
  template <bool IsBottom>
  bool insert_typed(id_type child, pos_type index) {
    assert(child != null_id && index <= count && count < NodeCap);
    const pos_type old_count = count;
    for (pos_type i = old_count; i > index; --i)
      this->template move_child_typed<IsBottom, false>(i, children[i - 1]);
    this->template move_child_typed<IsBottom, true, false>(index, child);
    if (count == NodeCap) return true;
    refresh_typed<IsBottom>();
    return false;
  }
  bool insert(id_type child, pos_type index) {
    if (level == 0) return insert_typed<true>(child, index);
    return insert_typed<false>(child, index);
  }

  /** @return Pool に確保された右兄弟の Internal ID。 */
  template <bool IsBottom>
  id_type split_typed(pos_type left_count = HalfNodeCap) {
    assert(count <= NodeCap && left_count <= count);
    assert(left_count < NodeCap && count - left_count < NodeCap);
    const pos_type right_count = count - left_count;
    const id_type left_id = id();
    const pos_type current_level = level;
    const id_type right_id = Pools::make_internal(IsBottom ? 0 : current_level);
    // Pool 拡張により this は無効になることがある。
    auto& left = Pools::internals()[left_id];
    auto& right = Pools::internals()[right_id];
    for (pos_type i = right_count; i > 0;) {
      --i;
      right.template move_child_typed<IsBottom>(
          i, left.children[left_count + i], right_id);
    }
    left.template refresh_typed<IsBottom>();
    right.template refresh_typed<IsBottom>();
    if constexpr (IsBottom) left.link_after(right_id);
    return right_id;
  }
  id_type split(pos_type left_count = HalfNodeCap) {
    if (level == 0) return split_typed<true>(left_count);
    return split_typed<false>(left_count);
  }

  /** @return この Internal が空になった場合 true。 */
  template <bool IsBottom>
  bool erase_typed(pos_type index) {
    assert(count < NodeCap && index < count);
    size -= child_size_typed<IsBottom>(index);
    Pools::recycle(children[index], IsBottom);
    for (pos_type i = index + 1; i < count; ++i)
      this->template move_child_typed<IsBottom, false>(i - 1, children[i]);
    --count;
    refresh_typed<IsBottom>();
    return count == 0;
  }
  bool erase(pos_type index) {
    if (level == 0) return erase_typed<true>(index);
    return erase_typed<false>(index);
  }

  template <bool IsBottom, bool WithLazy = false>
  id_type locate_typed(std::size_t index, bool* value_clean = nullptr) {
    apply_reverse_typed<IsBottom>();
    if constexpr (WithLazy) {
      if constexpr (requires(I& node) {
                      node.template apply_lazy_typed<IsBottom>();
                    })
        static_cast<I*>(this)->template apply_lazy_typed<IsBottom>();
      else
        static_cast<I*>(this)->apply_lazy();
    } else if constexpr (requires { Traits::lazy; }) {
      if constexpr (Traits::lazy) {
        if (value_clean && *value_clean && static_cast<I*>(this)->pending)
          *value_clean = false;
      }
    }

    if constexpr (IsBottom) {
      assert(index < count);
      return children[index];
    } else {
      for (pos_type i = 0; i < count; ++i) {
        auto& child = get_child(i);
        if (index < child.size) {
          return Pools::with_internal(children[i], [&](auto& typed_child) {
            using Child = std::remove_cvref_t<decltype(typed_child)>;
            constexpr bool ChildBottom =
                std::is_same_v<Child, BottomInternal<Traits>>;
            return typed_child.template locate_typed<ChildBottom, WithLazy>(
                index, value_clean);
          });
        }
        index -= child.size;
      }
      assert(false);
      return null_id;
    }
  }
  template <bool WithLazy = false>
  id_type locate(std::size_t index, bool* value_clean = nullptr) {
    if (level == 0)
      return locate_typed<true, WithLazy>(index, value_clean);
    return locate_typed<false, WithLazy>(index, value_clean);
  }

  /** @brief 同じ高さの子の型を固定して統合する。 */
  template <bool ChildBottom>
  void integrate_child_typed(pos_type index) {
    assert(level > 0 && index < count);
    auto get_typed_child = [&](pos_type i) -> decltype(auto) {
      if constexpr (ChildBottom)
        return (Pools::bottom(children[i]));
      else
        return (get_child(i));
    };

    if (index && get_typed_child(index - 1).count +
                     get_typed_child(index).count < NodeCap) {
      auto& left = get_typed_child(index - 1);
      auto& right = get_typed_child(index);
      left.template materialize_typed<ChildBottom>();
      right.template materialize_typed<ChildBottom>();
      const pos_type left_count = left.count;
      const pos_type right_count = right.count;
      const id_type left_id = left.id();
      for (pos_type i = right_count; i > 0;) {
        --i;
        left.template move_child_typed<ChildBottom>(
            left_count + i, right.children[i], left_id);
      }
      left.template refresh_typed<ChildBottom>();
    } else if (index + 1 < count &&
               get_typed_child(index + 1).count +
                   get_typed_child(index).count < NodeCap) {
      auto& left = get_typed_child(index);
      auto& right = get_typed_child(index + 1);
      left.template materialize_typed<ChildBottom>();
      right.template materialize_typed<ChildBottom>();
      const pos_type left_count = left.count;
      const pos_type right_count = right.count;
      const id_type right_id = right.id();
      for (pos_type i = right_count; i > 0;) {
        --i;
        right.template move_child_typed<ChildBottom, false>(
            left_count + i, right.children[i]);
      }
      for (pos_type i = left_count; i > 0;) {
        --i;
        right.template move_child_typed<ChildBottom>(
            i, left.children[i], right_id);
      }
      right.template refresh_typed<ChildBottom>();
    } else {
      return;
    }
    erase_typed<false>(index);
  }

  /** @brief index の Internal 子を隣の兄弟へ移動し、元の子を回収する。 */
  void integrate_child(pos_type index) {
    assert(level > 0 && index < count);
    // All siblings have the same node kind. Dispatch only at this boundary.
    if (children[index] & Pools::bottom_mask)
      integrate_child_typed<true>(index);
    else
      integrate_child_typed<false>(index);
  }

  /** @brief 統合成功時は自身の ID が無効になる。 */
  void integrate() {
    if (this->parent == null_id) return;
    const auto parent_id = this->parent;
    const auto position = this->position;
    Pools::internals()[parent_id].integrate_child(position);
  }
};

/**
 * @brief level=0 専用の Internal 実体。隣接リンクは上層ノードには配置しない。
 * @details Traits::Internal を継承するので Set/Aggregate の metadata も保持する。
 */
template <class Traits>
struct BottomInternal : Traits::Internal {
  using Parent = typename Traits::Internal;
  using Link = typename Bplustree::Internal<Traits>::Link;
  std::array<Link, 2> links{};
  bool ports_swapped = false;

  BottomInternal() : Parent(0) {}
};

}  // namespace Bplustree
