#pragma once

#include <cstdint>

#include "data-structure/object-pool/object_pool.hpp"

namespace Bplustree {
/**
 * @brief Traits ごとに共有する Leaf / Internal の2つの ObjectPool。
 * @details ID は型ごとの Pool 内のスロット番号で、Leaf ID と Internal ID は別空間。
 * 同じ Traits の複数の木が Pool を共有する。recycle 後の ID は再利用され得る。
 */
template <class Traits>
struct NodePools {
  using pos_type = std::uint8_t;

  using id_type = std::uint32_t;
  using L = typename Traits::Leaf;
  using I = typename Traits::Internal;
  static constexpr id_type null_id = ObjectPool<L>::null_id;

 private:
  // constinit により静的初期化を保証し、関数内 static の初期化ガードを避ける。
  inline static constinit ObjectPool<L> leaf_pool_{};
  inline static constinit ObjectPool<I> internal_pool_{};

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
  static ObjectPool<I>& internals() {
    return internal_pool_;
  }

  /**
   * @brief 追加確保に備えて Leaf / Internal Pool の容量を増やす。
   * @details Pool の vector が再確保されると生存中ノードへの参照は失効するが、
   * ID は変わらない。
   */
  static void reserve_buffer(std::size_t leaf_buffer = 8,
                             std::size_t internal_buffer = 8) {
    leaves().reserve_buffer(leaf_buffer);
    internals().reserve_buffer(internal_buffer);
  }
  /** @brief Leaf を確保（解放済み ID を優先再利用）し、その ID を返す。 */
  template <class... Args>
  static id_type make_leaf(Args&&... args) {
    auto id = leaves().emplace(std::forward<Args>(args)...);
    return id;
  }
  /** @brief 指定 level の Internal を確保し、その ID を返す。 */
  static id_type make_internal(pos_type level = 0) {
    auto id = internals().emplace(level);
    return id;
  }
  /**
   * @brief 指定 ID が表す Leaf または Internal 部分木を再利用可能にする。
   * @param is_leaf true なら Leaf Pool を解放。false なら Internal 部分木を走査。
   * @details ID 自体を解放 ID 列へ戻す処理であり、各要素のデストラクタは
   * 直ちには呼び出されない。回収した ID を持つ iterator は失効する。
   * ### 計算量
   * O(回収するノード数)。
   */
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