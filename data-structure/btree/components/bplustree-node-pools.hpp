#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

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
 *
 * ### 計算量
 * ノードへの型付きアクセス・型判定は O(1)。
 */
template <class Traits>
class NodePools {
 public:
  using pos_type = std::uint8_t;
  using id_type = std::uint32_t;

 private:
  static constexpr id_type kNodeBitCount = 30;
  static constexpr id_type kLeafBitCount = 31;
  static constexpr id_type kNodeMask = (id_type{1} << kNodeBitCount) - 1;
  static constexpr id_type kLeafMask = (id_type{1} << kLeafBitCount) - 1;
  static constexpr id_type kUpperTag = id_type{3} << kNodeBitCount;
  static constexpr id_type kBottomTag = id_type{2} << kNodeBitCount;
  using L = typename Traits::Leaf;
  using U = typename Traits::UpperNode;
  using B = typename Traits::BottomNode;

  inline static constinit ObjectPool<L, kLeafMask> leaf_pool_{};
  inline static constinit ObjectPool<U, kNodeMask - 1> upper_pool_{};
  inline static constinit ObjectPool<B, kNodeMask> bottom_pool_{};

  static id_type inner_id(id_type id) {
    return type_of(id) == NodeType::Leaf ? id & kLeafMask : id & kNodeMask;
  }

 public:
  static constexpr id_type null_id = ObjectPool<L, kLeafMask>::null_id;

  static NodeType type_of(id_type id) {
    assert(id != null_id);
    if ((id >> kLeafBitCount) == 0) return NodeType::Leaf;
    if ((id >> kNodeBitCount) == 2) return NodeType::Bottom;
    return NodeType::Upper;
  }

  static bool is_bottom(id_type id) {
    return id != null_id && type_of(id) == NodeType::Bottom;
  }

  static ObjectPool<L, kLeafMask>& leaves() {
    return leaf_pool_;
  }
  static ObjectPool<U, kNodeMask - 1>& uppers() {
    return upper_pool_;
  }
  static ObjectPool<B, kNodeMask>& bottoms() {
    return bottom_pool_;
  }

  static id_type id(const L& leaf) {
    return leaves().index(leaf);
  }
  static id_type id(const U& node) {
    return uppers().index(node) | kUpperTag;
  }
  static id_type id(const B& node) {
    return bottoms().index(node) | kBottomTag;
  }

  template <NodeType Type>
  static auto& get(id_type id) {
    assert(id != null_id && Type == type_of(id));
    constexpr id_type mask = Type == NodeType::Leaf ? kLeafMask : kNodeMask;

    const id_type id_ = id & mask;
    if constexpr (Type == NodeType::Leaf) {
      return leaves()[id_];
    } else if constexpr (Type == NodeType::Bottom) {
      return bottoms()[id_];
    } else {
      return uppers()[id_];
    }
  }

  /**
   * @brief ID タグの動的判定を境界に集約し、具象ノードで callback を実行する。
   * @details 返り値は両 callback 呼び出しで同一型でなければならない。
   */
  template <class Callback>
  static decltype(auto) with_node(id_type id, Callback&& callback) {
    assert(id != null_id);
    if (is_bottom(id))
      return std::forward<Callback>(callback)(get<NodeType::Bottom>(id));
    return std::forward<Callback>(callback)(get<NodeType::Upper>(id));
  }

  /**
   * @brief 追加確保に備えて Leaf / BottomNode / UpperNode Pool の容量を増やす。
   * @details Pool の vector が再確保されると生存中ノードへの参照は失効するが、
   * ID は変わらない。
   */
  static void reserve_buffer(std::size_t leaf_buffer = 8,
                             std::size_t node_buffer = 8) {
    leaves().reserve_buffer(leaf_buffer);
    uppers().reserve_buffer(node_buffer);
    bottoms().reserve_buffer(node_buffer);
  }

  /** @brief Leaf を確保し、その ID を返す。 */
  template <class... Args>
  static id_type make_leaf(Args&&... args) {
    return leaves().emplace(std::forward<Args>(args)...);
  }
  /** @brief UpperNode を確保し、タグ付き ID を返す。 */
  template <class... Args>
  static id_type make_upper(Args&&... args) {
    return uppers().emplace(std::forward<Args>(args)...) | kUpperTag;
  }
  /** @brief BottomNode を確保し、タグ付き ID を返す。 */
  template <class... Args>
  static id_type make_bottom(Args&&... args) {
    return bottoms().emplace(std::forward<Args>(args)...) | kBottomTag;
  }

  /**
   * @brief 指定 ID が表す Leaf またはノード部分木を再利用可能にする。
   * @tparam Recursive false なら対象ノードのみ、true なら子孫も回収する。
   * ### 計算量
   * O(回収するノード数)。
   */
  template <bool Recursive = true>
  static void recycle(id_type id) {
    if (id == null_id) return;
    const NodeType type = type_of(id);
    const id_type id_ = inner_id(id);
    if (type == NodeType::Leaf) {
      leaves().erase(id_);
    } else if (type == NodeType::Bottom) {
      if constexpr (Recursive) {
        const auto bottom = bottoms()[id_];
        for (pos_type i = 0; i < bottom.count; ++i)
          recycle<false>(bottom.children[i]);
      }
      bottoms().erase(id_);
    } else {
      if constexpr (Recursive) {
        const auto upper = uppers()[id_];
        for (pos_type i = 0; i < upper.count; ++i)
          recycle<true>(upper.children[i]);
      }
      uppers().erase(id_);
    }
  }
};

}  // namespace Bplustree
