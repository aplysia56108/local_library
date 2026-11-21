#pragma once

#include <cstdint>

#include "data-structure/object-pool/object_pool.hpp"

namespace Bplustree {
/**
 * @brief Traits ごとに共有する Leaf / Internal の2つの ObjectPool。
 * @details ID は型ごとの Pool 内のスロット番号で、Leaf ID と Internal ID
 * は別空間。 同じ Traits の複数の木が Pool を共有する。recycle 後の ID
 * は再利用され得る。
 */
template <class Traits>
struct NodePools {
  using pos_type = std::uint8_t;

  using id_type = std::uint32_t;
  using L = typename Traits::Leaf;
  using U = typename Traits::UpperNode;
  using B = typename Traits::BottomNode;
  static constexpr id_type null_id = ObjectPool<L>::null_id;

 private:
  // constinit により静的初期化を保証し、関数内 static の初期化ガードを避ける。
  inline static constinit ObjectPool<L> leaf_pool_{};
  inline static constinit ObjectPool<U> upper_pool_{};
  inline static constinit ObjectPool<B> bottom_pool_{};

 public:
  static id_type id(L& leaf) {
    return leaves().index(leaf);
  }
  static id_type id(U& node) {
    return uppers().index(node);
  }
  static id_type id(B& node) {
    return bottoms().index(node);
  }
  static ObjectPool<L>& leaves() {
    return leaf_pool_;
  }
  static ObjectPool<U>& uppers() {
    return upper_pool_;
  }
  static ObjectPool<B>& bottoms() {
    return bottom_pool_;
  }
  enum class NodeType { Leaf, Bottom, Upper };
  template <NodeType T>
  static auto& get(id_type id) {
    if constexpr (T == NodeType::Leaf) {
      return leaves()[id];
    }
    if constexpr (T == NodeType::Bottom) {
      return bottoms()[id];
    }
    if constexpr (T == NodeType::Upper) {
      return uppers()[id];
    }
  };

  template <std::size_t T>
  static auto& get(id_type id) {
    if constexpr (T == 0) {
      return leaves()[id];
    }
    if constexpr (T == 1) {
      return bottoms()[id];
    }
    if constexpr (T == 2) {
      return uppers()[id];
    }
  };
  /**
   * @brief 追加確保に備えて Leaf / Internal Pool の容量を増やす。
   * @details Pool の vector
   * が再確保されると生存中ノードへの参照は失効するが、 ID は変わらない。
   */
  static void reserve_buffer(std::size_t leaf_buffer = 8,
                             std::size_t node_buffer = 8) {
    leaves().reserve_buffer(leaf_buffer);
    uppers().reserve_buffer(node_buffer);
    bottoms().reserve_buffer(node_buffer);
  }
  /** @brief Leaf を確保（解放済み ID を優先再利用）し、その ID を返す。 */
  template <class... Args>
  static id_type make_leaf(Args&&... args) {
    auto id = leaves().emplace(std::forward<Args>(args)...);
    return id;
  }
  /** @brief 指定 level の Internal を確保し、その ID を返す。 */
  template <class... Args>
  static id_type make_upper(Args&&... args) {
    auto id = uppers().emplace(std::forward<Args>(args)...);
    return id;
  }
  template <class... Args>
  static id_type make_bottom(Args&&... args) {
    auto id = bottoms().emplace(std::forward<Args>(args)...);
    return id;
  }
  /**
   * @brief 指定 ID が表す Leaf または Internal 部分木を再利用可能にする。
   * @param is_leaf true なら Leaf Pool を解放。false なら Internal
   * 部分木を走査。
   * @details ID 自体を解放 ID 列へ戻す処理であり、各要素のデストラクタは
   * 直ちには呼び出されない。回収した ID を持つ iterator は失効する。
   * ### 計算量
   * O(回収するノード数)。
   */
  template <std::size_t Type = 2, bool Recursive = true>
  static void recycle(const id_type& id) {
    if (id == null_id) return;
    if constexpr (Type == 0) {
      leaves().erase(id);
    } else if constexpr (Type == 1) {
      auto& bottom = bottoms()[id];
      if constexpr (Recursive)
        for (pos_type i = 0; i < bottom.count; ++i)
          recycle(bottom.children[i], 0);
      bottoms().erase(id);
    } else {
      auto& upper = uppers()[id];
      if constexpr (Recursive)
        for (pos_type i = 0; i < upper.count; ++i)
          recycle(upper.children[i], upper.level > 1 ? 2 : 1);
      uppers().erase(id);
    }
  }
};
}  // namespace Bplustree