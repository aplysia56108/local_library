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
#include "data-structure/btree/components/bplustree-node-access.hpp"

namespace Bplustree {

/**
 * @brief 同じ Traits の iterator が共有する position / value の世代。
 * @details
 * position は未伝搬の反転・位置経路、value は未伝搬 Lazy の検証用。
 * split / merge / swap で所属木が変わっても、iterator は Leaf ID と
 * 世代を使って経路を再検証する。0 は未検証の番兵値として使用するため
 * 有効な世代には用いない。異なる木の更新でも保守的に世代を進める。
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

/**
 * @brief Leaf ID で要素を識別する、読み取り専用参照の B+tree iterator。
 * @tparam Traits ノード構成と Lazy 有無を定義する Traits。
 * @tparam Tree 所属する Base 型（実体へのポインタは保持しない）。
 * @tparam IsConst true のときは set() を公開しない。
 * @details
 * C++20 の std::random_access_iterator コンセプトに対応する。
 * ただし ++ / -- 以外も含め移動や差分は O(1) とは限らず、
 * 最悪 O(NodeCapacity * 木の高さ) の経路走査が発生する。
 * 実要素 Leaf の削除 / clear / assign / rebuild で当該 ID は失効する。
 * split / merge / swap / reverse 後も、残存する実要素を指す ID は維持する。
 * 差分・順序比較は同一の木に所属する iterator 同士に限る。
 */
template <class Traits, class Tree, bool IsConst>
class Iterator {
  template <class, class, bool>
  friend class Iterator;
  friend Tree;

 protected:
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Node;
  using Access = NodeAccess<Traits>;
  using T = typename Traits::T;
  using Pools = NodePools<Traits>;
  using Epoch = IteratorEpoch<Traits>;
  using id_type = typename Pools::id_type;
  static constexpr id_type null_id = NodePools<Traits>::null_id;

  // lazy=false を明示した Traits では value epoch をコンパイル時に省略。
  // 未宣言の場合はカスタム Traits との互換性のため Lazy ありとして扱う。
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
  // 0は未検証。値epochはLazyをサポートするTraitsでのみ保持する。
  mutable std::uint64_t validated_position_epoch_ = 0;
  [[no_unique_address]] mutable ValueEpoch validated_value_epoch_{};

  static Leaf& get_leaf(const id_type& id) {
    return Pools::leaves()[id];
  }
  static Internal& get_node(const id_type& id) {
    return Access::get(id);
  }
  Leaf& this_leaf() const {
    return get_leaf(leaf_id_);
  }

  static void reverse_path(id_type node_id) {
    auto& node = get_node(node_id);
    if (node.parent != null_id) reverse_path(node.parent);
    Access::with(node_id, [](auto& concrete) { concrete.apply_reverse(); });
  }

  static void materialize_path(id_type node_id) {
    auto& node = get_node(node_id);
    if (node.parent != null_id) materialize_path(node.parent);
    Access::materialize(node_id);
  }

  void materialize_path() const {
    materialize_path(this_leaf().parent);
  }

  /** @brief position 世代が古ければ、根からの経路にある反転のみ伝搬する。 */
  void ensure_position_path() const {
    if (validated_position_epoch_ != Epoch::position) {
      reverse_path(this_leaf().parent);
      validated_position_epoch_ = Epoch::position;
    }
  }

  /** @brief 値参照前に必要な遅延作用を伝搬し、position/value 世代を確定する。 */
  void ensure_value_path() const {
    if constexpr (HasLazy) {
      if (validated_value_epoch_ != Epoch::value) {
        materialize_path();
        validated_value_epoch_ = Epoch::value;
        validated_position_epoch_ = Epoch::position;
      }
    } else {
      ensure_position_path();
    }
  }

  static void refresh_path(id_type node_id) {
    auto& node = get_node(node_id);
    Access::refresh_metadata(node_id);
    if (node.parent != null_id) refresh_path(node.parent);
  }

  void refresh_path() const {
    refresh_path(this_leaf().parent);
  }

  /**
   * @brief 現在の Leaf から offset 個だけ移動する。
   * @details Lazy を値に反映せず、位置情報だけ確定する。
   * Lazy なしの同一最下層 Internal 内移動は O(1)。
   * 範囲外への移動は未定義（assert による部分的な検出のみ）。
   */
  void advance(std::ptrdiff_t offset) {
    assert(leaf_id_ != null_id);
    // 移動時はReverseだけ確定し、値を読まない要素のLazyは保持する。
    ensure_position_path();
    auto& leaf = this_leaf();
    std::ptrdiff_t to_index =
        static_cast<std::ptrdiff_t>(leaf.position) + offset;
    id_type node_id = leaf.parent;

    if constexpr (!HasLazy) {
      // Leafの親は必ず最下層Internal（level == 0）。
      // positionを確定した後、同じ親の中なら汎用locateを使わずに
      // 子IDを直接取得できる。位置epochはすでに検証済み。
      auto& bottom = get_node(node_id);
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
          leaf_id_ = Access::template locate<false>(node_id, to_index, &value_clean);
          validated_position_epoch_ = Epoch::position;
          validated_value_epoch_ = value_clean ? Epoch::value : 0;
        } else {
          // Set / Vector / Segtreeには値epochもLazy検証も不要。
          leaf_id_ = Access::template locate<false>(node_id, to_index);
          validated_position_epoch_ = Epoch::position;
        }
        return;
      }
      assert(node.parent != null_id);
      for (std::size_t i = 0; i < node.position; ++i) {
        to_index += Access::child_size(node.parent, i);
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

  /** @brief コンテナ側の locate が既に検証した経路の種類を示す。 */
  enum class Validation { None, Position, Value };

  /** @brief null_id の未検証 iterator を構築する。 */
  Iterator() = default;
  /**
   * @brief Leaf ID を受け取り、任意で検証済みの世代を記録する。
   * @details Position なら位置経路、Value なら位置と値経路が確定済み。
   */
  explicit Iterator(id_type leaf_id,
                    Validation validation = Validation::None)
      : leaf_id_(leaf_id) {
    if (validation != Validation::None)
      validated_position_epoch_ = Epoch::position;
    if constexpr (HasLazy) {
      if (validation == Validation::Value)
        validated_value_epoch_ = Epoch::value;
    }
  }

  template <bool OtherConst>
    requires(IsConst && !OtherConst)
  Iterator(const Iterator<Traits, Tree, OtherConst>& other)
      : leaf_id_(other.leaf_id_),
        validated_position_epoch_(other.validated_position_epoch_) {
    if constexpr (HasLazy)
      validated_value_epoch_ = other.validated_value_epoch_;
  }

  /**
   * @brief 参照の前に必要な反転と遅延作用を確定し、値を読み取る。
   * @details 返す const 参照は Pool の再確保でも無効化され得る。
   */
  reference operator*() const {
    assert(leaf_id_ != null_id);
    ensure_value_path();
    return this_leaf().get();
  }

  pointer operator->() const {
    return std::addressof(operator*());
  }

  /** @brief 1要素を更新して、内部ノードの集約メタデータを祖先へ反映する。 */
  void set(const T& value) const
    requires(!IsConst)
  {
    ensure_value_path();
    this_leaf().set(value);
    refresh_path();
  }

  Iterator& operator++() {
    advance(1);
    return *this;
  }

  Iterator operator++(signed) {
    Iterator result = *this;
    ++*this;
    return result;
  }

  Iterator& operator--() {
    advance(-1);
    return *this;
  }

  Iterator operator--(signed) {
    Iterator result = *this;
    --*this;
    return result;
  }

  Iterator& operator+=(std::ptrdiff_t offset) {
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

  /**
   * @brief 同じ木に所属する2つの iterator の位置差を求める。
   * @details 違う木の iterator 同士の差分はサポートしない。
   */
  friend std::ptrdiff_t operator-(const Iterator& lhs, const Iterator& rhs) {
    lhs.ensure_position_path();
    rhs.ensure_position_path();

    id_type l_node_id = get_leaf(lhs.leaf_id_).parent;
    std::ptrdiff_t l_index = get_leaf(lhs.leaf_id_).position;
    id_type r_node_id = get_leaf(rhs.leaf_id_).parent;
    std::ptrdiff_t r_index = get_leaf(rhs.leaf_id_).position;

    while (l_node_id != r_node_id) {
      auto& l_node = get_node(l_node_id);
      auto& r_node = get_node(r_node_id);
      assert(l_node.parent != null_id && r_node.parent != null_id);
      for (std::size_t i = 0; i < l_node.position; ++i)
        l_index += Access::child_size(l_node.parent, i);
      for (std::size_t i = 0; i < r_node.position; ++i)
        r_index += Access::child_size(r_node.parent, i);

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

  // 同一性は Leaf ID のみで判定する。epoch はキャッシュ状態であり等値性に含めない。
  friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
    return lhs.leaf_id_ == rhs.leaf_id_;
  }
};
}  // namespace Bplustree