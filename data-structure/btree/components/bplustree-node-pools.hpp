#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "data-structure/object-pool/object_pool.hpp"

namespace Bplustree {

/**
 * @brief Leaf / BottomNode / UpperNode を独立した Pool に保持する。
 * @details
 * Internal の ID は最上位ビットで Bottom/Upper を区別する。
 * Leaf ID は別空間。型付き get<T>() はタグを除去してから Pool を参照する。
 * root や Internal の parent は ID だけから型を判定できる。
 *
 * ### 計算量
 * ノードへの型付きアクセス・型判定は O(1)。
 */
template <class Traits>
struct NodePools {
  using pos_type = std::uint8_t;
  using id_type = std::uint32_t;
  using L = typename Traits::Leaf;
  using U = typename Traits::UpperNode;
  using B = typename Traits::BottomNode;
  static constexpr id_type null_id = ObjectPool<L>::null_id;
  static constexpr id_type bottom_mask = id_type{1} << 31;

  enum class NodeType { Leaf, Bottom, Upper };

 private:
  inline static constinit ObjectPool<L> leaf_pool_{};
  inline static constinit ObjectPool<U> upper_pool_{};
  inline static constinit ObjectPool<B> bottom_pool_{};

 public:
  static bool is_bottom(id_type id) {
    return id != null_id && (id & bottom_mask) != 0;
  }

  static id_type id(L& leaf) {
    return leaves().index(leaf);
  }
  static id_type id(U& node) {
    const id_type result = uppers().index(node);
    assert(result < bottom_mask);
    return result;
  }
  static id_type id(B& node) {
    const id_type result = bottoms().index(node);
    assert(result < bottom_mask - 1);
    return bottom_mask | result;
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

  template <NodeType Type>
  static auto& get(id_type id) {
    if constexpr (Type == NodeType::Leaf) {
      return leaves()[id];
    } else if constexpr (Type == NodeType::Bottom) {
      return bottoms()[id & ~bottom_mask];
    } else {
      return uppers()[id];
    }
  }

  // 実装途中の NodeCRTP が使っている 0/1/2 の型指定にも対応する。
  template <std::size_t Type>
  static auto& get(id_type id) {
    static_assert(Type <= 2);
    if constexpr (Type == 0) return get<NodeType::Leaf>(id);
    else if constexpr (Type == 1) return get<NodeType::Bottom>(id);
    else return get<NodeType::Upper>(id);
  }

  /**
   * @brief ID タグの動的判定を境界に集約し、具象型で callback を実行する。
   * @details 返り値は両 callback 呼び出しで同一型である必要がある。
   */
  template <class Callback>
  static decltype(auto) with_internal(id_type id, Callback&& callback) {
    assert(id != null_id);
    if (is_bottom(id))
      return std::forward<Callback>(callback)(get<NodeType::Bottom>(id));
    return std::forward<Callback>(callback)(get<NodeType::Upper>(id));
  }

  static void reserve_buffer(std::size_t leaf_buffer = 8,
                             std::size_t node_buffer = 8) {
    leaves().reserve_buffer(leaf_buffer);
    uppers().reserve_buffer(node_buffer);
    bottoms().reserve_buffer(node_buffer);
  }

  template <class... Args>
  static id_type make_leaf(Args&&... args) {
    return leaves().emplace(std::forward<Args>(args)...);
  }
  template <class... Args>
  static id_type make_upper(Args&&... args) {
    const id_type id = uppers().emplace(std::forward<Args>(args)...);
    assert(id < bottom_mask);
    return id;
  }
  template <class... Args>
  static id_type make_bottom(Args&&... args) {
    const id_type id = bottoms().emplace(std::forward<Args>(args)...);
    assert(id < bottom_mask - 1);
    return bottom_mask | id;
  }

  /**
   * @brief 同一サブツリーの全子を回収する。
   * @tparam Type 0: Leaf, 1: Bottom, 2: Upper, 3: ID タグから判定。
   * @tparam Recursive false の場合はノード自体のみを回収する。
   */
  template <std::size_t Type = 3, bool Recursive = true>
  static void recycle(id_type id) {
    if (id == null_id) return;
    if constexpr (Type == 0) {
      leaves().erase(id);
    } else if constexpr (Type == 1) {
      auto& bottom = get<NodeType::Bottom>(id);
      if constexpr (Recursive)
        for (pos_type i = 0; i < bottom.count; ++i)
          recycle<0>(bottom.children[i]);
      bottoms().erase(id & ~bottom_mask);
    } else if constexpr (Type == 2) {
      auto& upper = get<NodeType::Upper>(id);
      if constexpr (Recursive) {
        for (pos_type i = 0; i < upper.count; ++i) {
          if (upper.level == 1) recycle<1>(upper.children[i]);
          else recycle<2>(upper.children[i]);
        }
      }
      uppers().erase(id);
    } else {
      static_assert(Type == 3);
      if (is_bottom(id)) recycle<1, Recursive>(id);
      else recycle<2, Recursive>(id);
    }
  }
};

}  // namespace Bplustree
