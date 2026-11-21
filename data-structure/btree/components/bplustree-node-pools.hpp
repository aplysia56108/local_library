#pragma once

#include <cstdint>

#include "data-structure/object-pool/object_pool.hpp"

namespace Bplustree {
enum class NodeType { Leaf, Bottom, Upper };

/**
 * @brief Traits ごとに共有する Leaf / BottomNode / UpperNode の ObjectPool。
 * @details
 * 各 Pool 内の ID を一つの id_type に符号化する。最上位 bit が 0 の ID は
 * Leaf（31 bit の Pool ID）、`10` で始まる ID は BottomNode（30 bit）、
 * `11` で始まる ID は UpperNode（30 bit）である。`null_id` は
 * `0xffffffff` とし、UpperNode の Pool ID は衝突を避けて
 * `0x3ffffffe` 以下に制限する。同じ Traits の複数の木は Pool を共有し、
 * recycle() 後の ID は再利用され得る。
 */
template <class Traits>
class NodePools {
 public:
  using pos_type = std::uint8_t;
  using id_type = std::uint32_t;

  static constexpr id_type null_id = ObjectPool<typename Traits::Leaf>::null_id;

 private:
  static const id_type node_valid_bit_n = 30;
  static const id_type leaf_valid_bit_n = 31;
  static const id_type node_msk = (id_type{1} << node_valid_bit_n) - 1;
  static const id_type leaf_msk = (id_type{1} << leaf_valid_bit_n) - 1;
  static const id_type upper_tag = (id_type{3} << node_valid_bit_n);
  static const id_type bottom_tag = (id_type{2} << node_valid_bit_n);
  using L = typename Traits::Leaf;
  using U = typename Traits::UpperNode;
  using B = typename Traits::BottomNode;
  // constinit により静的初期化を保証し、関数内 static の初期化ガードを避ける。
  inline static constinit ObjectPool<L, leaf_msk> leaf_pool_{};
  // upperのnode_mskはnull_idと競合するため、許容できない
  inline static constinit ObjectPool<U, node_msk - 1> upper_pool_{};
  inline static constinit ObjectPool<B, node_msk> bottom_pool_{};

  static id_type inner_id(const id_type id) {
    if (type_of(id) == NodeType::Leaf) return id & leaf_msk;
    return id & node_msk;
  }

 public:
  static id_type id(L& leaf) {
    return leaf_pool_.index(leaf);
  }
  static id_type id(U& node) {
    return upper_pool_.index(node) | upper_tag;
  }
  static id_type id(B& node) {
    return bottom_pool_.index(node) | bottom_tag;
  }

  static NodeType type_of(const id_type id) {
    if ((id >> leaf_valid_bit_n) == 0) return NodeType::Leaf;
    if ((id >> node_valid_bit_n) == 2) return NodeType::Bottom;
    return NodeType::Upper;
  };

  template <NodeType T>
  static auto& get(id_type id) {
    assert(id != null_id);
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
   * @brief 追加確保に備えて Leaf / BottomNode / UpperNode Pool の容量を増やす。
   * @details Pool の vector が再確保されると生存中ノードへの参照は失効するが、
   * ID は変わらない。
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
  /** @brief UpperNode を確保し、タグ付き ID を返す。 */
  template <class... Args>
  static id_type make_upper(Args&&... args) {
    auto id = upper_pool_.emplace(std::forward<Args>(args)...);
    return id | upper_tag;
  }
  /** @brief BottomNode を確保し、タグ付き ID を返す。 */
  template <class... Args>
  static id_type make_bottom(Args&&... args) {
    auto id = bottom_pool_.emplace(std::forward<Args>(args)...);
    return id | bottom_tag;
  }
  /**
   * @brief 指定 ID が表す Leaf またはノード部分木を再利用可能にする。
   * @tparam Recursive false なら対象ノードのみ、true なら子孫も回収する。
   * @details ID は型タグから対応する Pool を選んで回収する。各要素の
   * デストラクタは直ちには呼ばれず、回収した ID を持つ iterator は失効する。
   * ### 計算量
   * O(回収するノード数)。
   */

  template <bool Recursive = true>
  static void recycle(const id_type& id) {
    if (id == null_id) return;

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
