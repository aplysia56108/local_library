#pragma once

#include <iterator>
#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>
#include <type_traits>

#include "data-structure/btree/components/bplustree-node.hpp"

/**
 * @brief B+ tree 上の要素を指すイテレータ。
 * @details Leaf の Pool ID で要素と番兵を表す。
 *
 * C++20 の `std::random_access_iterator` コンセプトを満たす。
 * ただし木構造上、任意距離の移動やイテレータ差分は厳密には O(1) ではなく
 * O(NodeCapacity log_{NodeCapacity} N) となり得る。
 * 参照時はepochが変わった場合だけ祖先から遅延更新を適用し、同一epochではO(1)。
 * 連続した ++ は、新しい部分木への下降時だけ未伝搬情報を処理する。
 */
// Traitsが含む情報
// T:要素の型
// Node, Leaf, Internal:それぞれ要素の型。Leaf, InternalはNodeを継承している。
// kNodeCapacity:Internalのmax容量
namespace Bplustree {

/**
 * @brief Iteratorの位置経路と値経路のキャッシュを別々に管理する世代。
 * @details 同一Traitsの木で共有する。split/merge/swapでLeafの所属木が変わっても、
 * IteratorはTreeへの参照を持たずに経路キャッシュを再検証できる。
 * 0はIteratorの未検証状態を表すため、世代値には使用しない。
 */
template <class Traits>
struct IteratorEpoch {
  inline static std::uint64_t position = 1;
  inline static std::uint64_t value = 1;

  static void invalidate_position() noexcept {
    ++position;
    if (position == 0) ++position;
  }
  static void invalidate_value() noexcept {
    ++value;
    if (value == 0) ++value;
  }
  static void invalidate_all() noexcept {
    invalidate_position();
    invalidate_value();
  }
};

template <class Traits, class Tree, bool IsConst>
class Iterator {
  template <class, class, bool>
  friend class Iterator;
  friend Tree;

 protected:
  // using Traits = typename Tree::traits;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  using T = typename Traits::T;
  using Pools = NodePools<Traits>;
  using Epoch = IteratorEpoch<Traits>;
  using id_type = typename Pools::id_type;
  static constexpr id_type null_id = NodePools<Traits>::null_id;

  static constexpr bool HasLazy = [] {
    if constexpr (requires { Traits::lazy; })
      return Traits::lazy;
    else
      // 既存のカスタムTraitsはlazy宣言なしでapply_lazy()を実装できる。
      // 未宣言の場合は安全のためLazyありとして扱う。
      return true;
  }();

  // LazyがないTraitsでは値epochのキャッシュ自体を保持しない。
  // [[no_unique_address]] により空のメンバに領域を割り当てない。
  struct NoValueEpoch {};
  using ValueEpoch = std::conditional_t<HasLazy, std::uint64_t, NoValueEpoch>;

  id_type leaf_id_ = null_id;
  // Place these two 1-byte fields into the padding before the epoch, keeping
  // non-Lazy iterators at 16 bytes on 64-bit platforms.
  mutable pos_type entry_port_ = 0;
  mutable bool observed_port_swap_ = false;
  // 0は未検証。値epochはLazyをサポートするTraitsでのみ保持する。
  mutable std::uint64_t validated_position_epoch_ = 0;
  [[no_unique_address]] mutable ValueEpoch validated_value_epoch_{};

  static Leaf& get_leaf(const id_type& id) {
    return Pools::leaves()[id];
  }
  static Internal& get_node(const id_type& id) {
    return Pools::internals()[id];
  }
  Leaf& this_leaf() const {
    return get_leaf(leaf_id_);
  }

  static void reverse_path(id_type node_id) {
    auto& node = get_node(node_id);
    if (node.parent != null_id) reverse_path(node.parent);
    node.apply_reverse();
  }

  static void materialize_path(id_type node_id) {
    auto& node = get_node(node_id);
    if (node.parent != null_id) materialize_path(node.parent);
    node.materialize();
  }

  void materialize_path() const {
    materialize_path(this_leaf().parent);
  }

  /** @brief Reconcile the entry port with a local physical port swap. */
  void sync_entry_port() const {
    const auto& bottom = Pools::bottom(this_leaf().parent);
    if (observed_port_swap_ != bottom.ports_swapped) {
      entry_port_ ^= 1;
      observed_port_swap_ = bottom.ports_swapped;
    }
  }

  void reset_entry_port() const {
    entry_port_ = 0;
    observed_port_swap_ = Pools::bottom(this_leaf().parent).ports_swapped;
  }

  /** @brief 位置確定ではReverseだけを伝搬し、Lazyは保持する。 */
  void ensure_position_path() const {
    if (validated_position_epoch_ != Epoch::position) {
      reverse_path(this_leaf().parent);
      validated_position_epoch_ = Epoch::position;
      reset_entry_port();
    } else {
      sync_entry_port();
    }
  }

  /** @brief Random access needs the physical order to match logical order. */
  void normalize_position_path() const {
    ensure_position_path();
    if (entry_port_ != 0) {
      reverse_path(this_leaf().parent);
      reset_entry_port();
    }
  }

  /** @brief Lazyがなければ位置経路だけ、あれば必要時に値経路を確定する。 */
  void ensure_value_path() const {
    if constexpr (HasLazy) {
      if (validated_value_epoch_ != Epoch::value) {
        materialize_path();
        validated_value_epoch_ = Epoch::value;
        validated_position_epoch_ = Epoch::position;
        reset_entry_port();
      }
    } else {
      ensure_position_path();
    }
  }

  static void refresh_path(id_type node_id) {
    auto& node = get_node(node_id);
    node.refresh_metadata();
    if (node.parent != null_id) refresh_path(node.parent);
  }

  void refresh_path() const {
    refresh_path(this_leaf().parent);
  }

  /** @brief Advance one leaf through the reversible bottom-node path. */
  void step(bool forward) {
    assert(leaf_id_ != null_id);
    ensure_position_path();
    const auto& leaf = this_leaf();
    const auto& bottom = Pools::bottom(leaf.parent);
    const std::ptrdiff_t delta = (entry_port_ == 0 ? 1 : -1) *
                                 (forward ? 1 : -1);
    const auto next_pos = static_cast<std::ptrdiff_t>(leaf.position) + delta;
    if (0 <= next_pos && next_pos < bottom.count) {
      leaf_id_ = bottom.children[static_cast<std::size_t>(next_pos)];
      return;
    }
    const auto link = bottom.links[forward ? (entry_port_ ^ 1) : entry_port_];
    assert(link.node != null_id);
    const auto& neighbor = Pools::bottom(link.node);
    assert(neighbor.level == 0 && neighbor.count > 0);
    entry_port_ = forward ? link.port : (link.port ^ 1);
    observed_port_swap_ = neighbor.ports_swapped;
    const auto pos = forward ? (entry_port_ == 0 ? 0 : neighbor.count - 1)
                             : (entry_port_ == 0 ? neighbor.count - 1 : 0);
    leaf_id_ = neighbor.children[pos];
    // A different bottom node can have unpropagated Lazy on its ancestors.
    if constexpr (HasLazy) validated_value_epoch_ = 0;
  }

  void advance(std::ptrdiff_t offset) {
    assert(leaf_id_ != null_id);
    // Random jumps operate on the materialized physical order.
    normalize_position_path();
    auto& leaf = this_leaf();
    std::ptrdiff_t to_index =
        static_cast<std::ptrdiff_t>(leaf.position) + offset;
    id_type node_id = leaf.parent;

    if constexpr (!HasLazy) {
      // Leafの親は必ず最下層Internal（level == 0）。
      // positionを確定した後、同じ親の中なら汎用locateを使わずに
      // 子IDを直接取得できる。位置epochはすでに検証済み。
      auto& bottom = Pools::bottom(node_id);
      if (0 <= to_index &&
          to_index < static_cast<std::ptrdiff_t>(bottom.count)) {
        leaf_id_ = bottom.children[static_cast<std::size_t>(to_index)];
        return;
      }
    }

    while (true) {
      auto& node = get_node(node_id);
      if (0 <= to_index && to_index < static_cast<std::ptrdiff_t>(node.size)) {
        if constexpr (HasLazy) {
          // Lazyありの場合だけ、値epochと未伝搬タグを確認する。
          bool value_clean = validated_value_epoch_ == Epoch::value;
          leaf_id_ = node.template locate<false>(to_index, &value_clean);
          validated_position_epoch_ = Epoch::position;
          validated_value_epoch_ = value_clean ? Epoch::value : 0;
        } else {
          // Set / Vector / Segtreeには値epochもLazy検証も不要。
          leaf_id_ = node.template locate<false>(to_index);
          validated_position_epoch_ = Epoch::position;
        }
        reset_entry_port();
        return;
      }
      assert(node.parent != null_id);
      for (std::size_t i = 0; i < node.position; ++i) {
        to_index += get_node(node.parent).child_size(i);
      }
      node_id = node.parent;
    }
  }

 public:
  using iterator_concept = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = const T&;
  using pointer = const T*;

  Iterator() = default;
  explicit Iterator(id_type leaf_id) : leaf_id_(leaf_id) {}

  template <bool OtherConst>
    requires(IsConst && !OtherConst)
  Iterator(const Iterator<Traits, Tree, OtherConst>& other)
      : leaf_id_(other.leaf_id_),
        entry_port_(other.entry_port_),
        observed_port_swap_(other.observed_port_swap_),
        validated_position_epoch_(other.validated_position_epoch_) {
    if constexpr (HasLazy)
      validated_value_epoch_ = other.validated_value_epoch_;
  }

  reference operator*() const {
    assert(leaf_id_ != null_id);
    ensure_value_path();
    return this_leaf().get();
  }

  pointer operator->() const {
    return std::addressof(operator*());
  }

  void set(const T& value) const
    requires(!IsConst)
  {
    ensure_value_path();
    this_leaf().set(value);
    refresh_path();
  }

  Iterator& operator++() {
    step(true);
    return *this;
  }

  Iterator operator++(signed) {
    Iterator result = *this;
    ++*this;
    return result;
  }

  Iterator& operator--() {
    step(false);
    return *this;
  }

  Iterator operator--(signed) {
    Iterator result = *this;
    --*this;
    return result;
  }

  Iterator& operator+=(std::ptrdiff_t offset) {
    if (offset == 1)
      step(true);
    else if (offset == -1)
      step(false);
    else if (offset != 0)
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
    lhs.normalize_position_path();
    rhs.normalize_position_path();

    id_type l_node_id = get_leaf(lhs.leaf_id_).parent;
    std::ptrdiff_t l_index = get_leaf(lhs.leaf_id_).position;
    id_type r_node_id = get_leaf(rhs.leaf_id_).parent;
    std::ptrdiff_t r_index = get_leaf(rhs.leaf_id_).position;

    while (l_node_id != r_node_id) {
      auto& l_node = get_node(l_node_id);
      auto& r_node = get_node(r_node_id);
      assert(l_node.parent != null_id && r_node.parent != null_id);
      for (std::size_t i = 0; i < l_node.position; ++i)
        l_index += get_node(l_node.parent).child_size(i);
      for (std::size_t i = 0; i < r_node.position; ++i)
        r_index += get_node(r_node.parent).child_size(i);

      l_node_id = l_node.parent;
      r_node_id = r_node.parent;
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

  // epochはキャッシュ状態であり、iteratorの指す要素の同一性には含めない。
  friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
    return lhs.leaf_id_ == rhs.leaf_id_;
  }
};
}  // namespace Bplustree