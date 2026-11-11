#pragma once

#include "data-structure/object-pool/object_pool.hpp"

namespace Bplustree {
/** @brief Leaf と Internal の ID 空間を分けて共有する Pool。 */
template <class Traits>
struct NodePools {
  using pos_type = std::uint8_t;

  using id_type = std::uint32_t;
  using L = typename Traits::Leaf;
  using I = typename Traits::Internal;
  static constexpr id_type null_id = ObjectPool<L>::null_id;

  static id_type id(L& leaf) {
    return leaves().index(leaf);
  }

  static id_type id(I& node) {
    return internals().index(node);
  }

  static ObjectPool<L>& leaves() {
    static ObjectPool<L> pool;
    return pool;
  }
  static ObjectPool<I>& internals() {
    static ObjectPool<I> pool;
    return pool;
  }

  /** @brief
   * 外部操作の前に必要なスロット数を確保する。既存の参照は無効化され得る。 */
  static void reserve_buffer(std::size_t leaf_buffer = 8,
                             std::size_t internal_buffer = 8) {
    leaves().reserve_buffer(leaf_buffer);
    internals().reserve_buffer(internal_buffer);
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
  static void recycle(id_type id, bool is_leaf = false) {
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
}  // namespace Bplustree