#pragma once

#include <cstddef>

#include "data-structure/btree/components/bplustree-base.hpp"

namespace Bplustree {

template <class Traits, class Derived, bool IsBottom>
struct SetNodeCRTP;

/**
 * @brief Set / Multiset 用 Traits。Leaf はキー1個を保持する。
 * @details refresh_on_leaf_swap は、固定番兵を入れ替えた際の
 * Internal::min_leaf_id 更新を Base に要求する。
 */
template <class Key, std::size_t Capacity>
struct SetTraits {
  using T = Key;
  static constexpr bool refresh_on_leaf_swap = true;
  static constexpr std::size_t kNodeCapacity = Capacity;
  static constexpr pos_type kNodeHalf = Capacity / 2;
  using Pools = NodePools<SetTraits>;
  using NodeBase = Bplustree::NodeBase<SetTraits>;
  using Leaf = Bplustree::Leaf<SetTraits>;
  struct Node;
  struct UpperNode;
  struct BottomNode;
  using UpperCRTP = Bplustree::SetNodeCRTP<SetTraits, UpperNode, false>;
  using BottomCRTP = Bplustree::SetNodeCRTP<SetTraits, BottomNode, true>;
};

template <class Traits, class Derived, bool IsBottom>
struct SetNodeCRTP : Bplustree::NodeCRTP<Traits, Derived, IsBottom> {
  using Pools = typename Traits::Pools;

  void refresh_metadata() {
    assert(this->count);
    if (IsBottom) {
      this->min_leaf_id = this->children[0];
    } else {
      this->min_leaf_id =
          Pools::with_node(this->children[0], [](const auto& child) {
            return child.min_leaf_id;
          });
    }
  }
};

/**
 * @brief Set 専用の共有メタデータ。Upper/Bottom は Node を共通基底に持つ。
 * @details min_leaf_id は型付き Pool から子のキーを読み直して更新する。
 */
template <class Key, std::size_t Capacity>
struct SetTraits<Key, Capacity>::Node
    : Bplustree::Node<SetTraits<Key, Capacity>> {
  using Traits = SetTraits<Key, Capacity>;
  using Parent = Bplustree::Node<Traits>;
  using Pools = Bplustree::NodePools<Traits>;
  using id_type = typename Pools::id_type;
  using Parent::Parent;

  id_type min_leaf_id = Pools::null_id;

  const Key& min_key() const {
    return Pools::leaves()[min_leaf_id].get();
  }
};

template <class Key, std::size_t Capacity>
struct SetTraits<Key, Capacity>::UpperNode
    : Bplustree::UpperNode<SetTraits<Key, Capacity>> {
  using Traits = SetTraits<Key, Capacity>;
  using Parent = Bplustree::UpperNode<Traits>;
  using Parent::Parent;
  using CRTP = Traits::UpperCRTP;
  using CRTP::refresh_metadata;
};

template <class Key, std::size_t Capacity>
struct SetTraits<Key, Capacity>::BottomNode
    : Bplustree::BottomNode<SetTraits<Key, Capacity>> {
  using Traits = SetTraits<Key, Capacity>;
  using Parent = Bplustree::UpperNode<Traits>;
  using Parent::Parent;
  using CRTP = Traits::BottomCRTP;
  using CRTP::refresh_metadata;
};

}  // namespace Bplustree