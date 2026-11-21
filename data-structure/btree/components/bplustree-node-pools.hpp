#pragma once

#include <cstdint>

#include "data-structure/object-pool/object_pool.hpp"

namespace Bplustree {
enum class NodeType { Leaf, Bottom, Upper };

/**
 * @brief Traits ごとに共有する Leaf / Internal の2つの ObjectPool。
 * @details ID は型ごとの Pool 内のスロット番号で、Leaf ID と Internal ID
 * は別空間。 同じ Traits の複数の木が Pool を共有する。recycle 後の ID
 * は再利用され得る。
 */
template <class Traits>
class NodePools {
 public:
  using pos_type = std::uint8_t;
  using id_type = std::uint32_t;

  static constexpr id_type null_id = ObjectPool<typename Traits::Leaf>::null_id;

 private:
  static const id_type msk = (id_type{1} << 30) - 1;
  using L = typename Traits::Leaf;
  using U = typename Traits::UpperNode;
  using B = typename Traits::BottomNode;
  // constinit により静的初期化を保証し、関数内 static の初期化ガードを避ける。
  inline static constinit ObjectPool<L> leaf_pool_{};
  inline static constinit ObjectPool<U> upper_pool_{};
  inline static constinit ObjectPool<B> bottom_pool_{};

  static NodeType type_of(const id_type id) {
    if (id >> 31 & 1) {
      if (id >> 30 & 1) return NodeType::Upper;
      return NodeType::Bottom;
    } else {
      return NodeType::Leaf;
    }
  };

  static id_type inner_id(const id_type id) {
    return id & msk;
  }

 public:
  static id_type id(L& leaf) {
    return leaf_pool_.index(leaf);
  }
  static id_type id(U& node) {
    return upper_pool_.index(node) | (id_type{1} << 31) | (id_type{1} << 30);
  }
  static id_type id(B& node) {
    return bottom_pool_.index(node) | (id_type{1} << 31);
  }
  template <NodeType T>
  static auto& get(id_type id) {
    auto id_ = inner_id(id);
    assert(T == type_of(id));
    if constexpr (T == NodeType::Leaf) {
      return leaf_pool_[id_];
    }
    if constexpr (T == NodeType::Bottom) {
      return bottom_pool_[id_];
    }
    if constexpr (T == NodeType::Upper) {
      return upper_pool_[id_];
    }
  };

  /**
   * @brief 追加確保に備えて Leaf / Internal Pool の容量を増やす。
   * @details Pool の vector
   * が再確保されると生存中ノードへの参照は失効するが、 ID は変わらない。
   */
  static void reserve_buffer(std::size_t leaf_buffer = 8,
                             std::size_t node_buffer = 8) {
    leaf_pool_.reserve_buffer(leaf_buffer);
    upper_pool_.reserve_buffer(node_buffer);
    bottom_pool_.reserve_buffer(node_buffer);
  }
  /** @brief Leaf を確保（解放済み ID を優先再利用）し、その ID を返す。 */
  template <class... Args>
  static id_type make_leaf(Args&&... args) {
    auto id = leaf_pool_.emplace(std::forward<Args>(args)...);
    return id;
  }
  /** @brief 指定 level の Internal を確保し、その ID を返す。 */
  template <class... Args>
  static id_type make_upper(Args&&... args) {
    auto id = upper_pool_.emplace(std::forward<Args>(args)...);
    return id | (id_type{1} << 31) | (id_type{1} << 30);
  }
  template <class... Args>
  static id_type make_bottom(Args&&... args) {
    auto id = bottom_pool_.emplace(std::forward<Args>(args)...);
    return id | (id_type{1} << 31);
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

  template <bool Recursive = true>
  static void recycle(const id_type& id) {
    auto Type = type_of(id);
    auto id_ = inner_id(id);
    if (Type == NodeType::Leaf)
      leaf_pool_.erase(id_);
    else if (Type == NodeType::Bottom) {
      if constexpr (Recursive) {
        auto bottom = bottom_pool_[id_];
        for (pos_type i = 0; i < bottom.count; ++i)
          recycle<false>(bottom.children[i]);
      }
      bottom_pool_.erase(id_);
    } else {
      if constexpr (Recursive) {
        auto upper = upper_pool_[id_];
        for (pos_type i = 0; i < upper.count; ++i)
          recycle<true>(upper.children[i]);
      }
      upper_pool_.erase(id_);
    }
  };
};
}  // namespace Bplustree