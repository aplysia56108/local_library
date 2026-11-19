#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "data-structure/object-pool/object_pool.hpp"

namespace Bplustree {

template <class Traits>
struct BottomInternal;
template <class Traits>
struct UpperInternal;

/**
 * @brief Upper/Internal と BottomInternal を異なるサイズの Pool に格納する。
 * @details
 * Internal ID の最上位bitが1なら BottomInternal、0なら上層Internal。
 * Leaf ID は別空間であり、null_id は両方に共通。
 * ### 計算量
 * 各 ID からノードへのアクセス: O(1)。
 */
template <class Traits>
struct NodePools {
  using pos_type = std::uint8_t;
  using id_type = std::uint32_t;
  using L = typename Traits::Leaf;
  using I = typename Traits::Internal;
  using U = UpperInternal<Traits>;
  using B = BottomInternal<Traits>;
  static constexpr id_type null_id = ObjectPool<L>::null_id;
  static constexpr id_type bottom_mask = id_type{1} << 31;
  template <class Concrete>
  static constexpr bool is_bottom_type =
      std::is_same_v<std::remove_cvref_t<Concrete>, B>;

 private:
  // Two separate vector allocations allow upper internals to omit links.
  inline static constinit ObjectPool<L> leaf_pool_{};
  inline static constinit ObjectPool<U> internal_pool_{};
  inline static constinit ObjectPool<B> bottom_pool_{};

  static constexpr bool is_bottom_id(id_type id) {
    return id != null_id && (id & bottom_mask);
  }

  static I& get_internal(id_type id) {
    // Same precondition as ObjectPool::operator[]: id must be valid.
    // Do not add per-access assertions on the traversal hot path.
    if (id & bottom_mask)
      return static_cast<I&>(bottom_pool_[id & ~bottom_mask]);
    return internal_pool_[id];
  }

  static id_type internal_index(const I& node) {
    if (node.level == 0) {
      const id_type id = bottom_pool_.index(static_cast<const B&>(node));
      assert(id < bottom_mask - 1);
      return bottom_mask | id;
    }
    const id_type id = internal_pool_.index(static_cast<const U&>(node));
    assert(id < bottom_mask);
    return id;
  }

  /**
   * @brief 既存の internals() API を保つ、異種 Pool の軽量ビュー。
   * @details Internal& を返すため、既存の集約 Traits もそのまま利用できる。
   */
  struct InternalView {
    I& operator[](id_type id) const {
      return get_internal(id);
    }
    id_type index(const I& node) const {
      return internal_index(node);
    }
    id_type emplace(pos_type level = 0) const {
      if (level == 0) {
        const id_type id = bottom_pool_.emplace();
        assert(id < bottom_mask - 1);
        return bottom_mask | id;
      }
      const id_type id = internal_pool_.emplace(level);
      assert(id < bottom_mask);
      return id;
    }
    void erase(id_type id) const {
      if (is_bottom_id(id))
        bottom_pool_.erase(id & ~bottom_mask);
      else
        internal_pool_.erase(id);
    }
    void reserve_buffer(std::size_t buffer = 8) const {
      internal_pool_.reserve_buffer(buffer);
      bottom_pool_.reserve_buffer(buffer);
    }
    std::size_t size() const {
      return internal_pool_.size() + bottom_pool_.size();
    }
  };
  inline static constinit InternalView internal_view_{};

 public:
  static id_type id(L& leaf) {
    return leaves().index(leaf);
  }

  static id_type id(I& node) {
    return internals().index(node);
  }

  static ObjectPool<L>& leaves() {
    return leaf_pool_;
  }

  static InternalView& internals() {
    return internal_view_;
  }

  static ObjectPool<B>& bottoms() {
    return bottom_pool_;
  }

  static ObjectPool<U>& uppers() {
    return internal_pool_;
  }

  /**
   * @brief ID から最下層/上層の実際の型へ一度だけ振り分ける。
   * @details Callback の引数型は BottomInternal& または UpperInternal&。
   * Callback 内では if constexpr により子の型を固定できる。
   * ### 計算量
   * O(1)。
   */
  template <class Callback>
  static decltype(auto) with_internal(id_type id, Callback&& callback) {
    if (id & bottom_mask)
      return std::forward<Callback>(callback)(bottom_pool_[id & ~bottom_mask]);
    return std::forward<Callback>(callback)(internal_pool_[id]);
  }

  /**
   * @brief Leaf の親など、Bottom 型が既知の ID を分岐なしで取得する。
   * @pre id は有効な BottomInternal ID。
   */
  static B& bottom(id_type id) {
    return bottom_pool_[id & ~bottom_mask];
  }

  /** @brief reserve による参照無効化に備え、両 Internal Pool を確保する。 */
  static void reserve_buffer(std::size_t leaf_buffer = 8,
                             std::size_t internal_buffer = 8) {
    leaves().reserve_buffer(leaf_buffer);
    internals().reserve_buffer(internal_buffer);
  }

  template <class... Args>
  static id_type make_leaf(Args&&... args) {
    return leaves().emplace(std::forward<Args>(args)...);
  }

  static id_type make_internal(pos_type level = 0) {
    return internals().emplace(level);
  }

  /** @brief 指定部分木の全スロットを再利用可能にする。O(部分木サイズ)。 */
  static void recycle(id_type id, bool is_leaf = false) {
    if (id == null_id) return;
    if (is_leaf) {
      leaves().erase(id);
    } else {
      with_internal(id, [&](auto& node) {
        constexpr bool IsBottom = is_bottom_type<decltype(node)>;
        if constexpr (IsBottom) node.unlink();
        for (pos_type i = 0; i < node.count; ++i)
          recycle(node.children[i], IsBottom);
      });
      internals().erase(id);
    }
  }
};

}  // namespace Bplustree
