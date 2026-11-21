#pragma once

#include <cstddef>
#include <utility>

#include "data-structure/btree/components/bplustree-node.hpp"

namespace Bplustree {

/**
 * @brief ID だけを保持する Base / Iterator のための型付き呼び出し境界。
 * @details
 * Internal ID を BottomNode / UpperNode に一度振り分けた後は
 * CRTP の具象型メンバを静的に呼び出す。
 * Node& は共通構造フィールドの参照専用。型固有の操作は with() を通す。
 *
 * ### 計算量
 * 各操作は ID 判定 O(1) と呼び出すノード操作の計算量。
 */
template <class Traits>
struct NodeAccess {
  using Pools = NodePools<Traits>;
  using Node = typename Traits::Node;
  using id_type = typename Pools::id_type;
  using pos_type = typename Pools::pos_type;

  template <class Callback>
  static decltype(auto) with(id_type id, Callback&& callback) {
    return Pools::with_internal(id, std::forward<Callback>(callback));
  }

  static Node& get(id_type id) {
    return with(id, [](auto& node) -> Node& { return node; });
  }

  static void materialize(id_type id) {
    with(id, [](auto& node) { node.materialize(); });
  }

  static void refresh(id_type id) {
    with(id, [](auto& node) { node.refresh(); });
  }

  static void refresh_metadata(id_type id) {
    with(id, [](auto& node) { node.refresh_metadata(); });
  }

  static void push_reverse(id_type id) {
    with(id, [](auto& node) { node.push_reverse(); });
  }

  static void integrate(id_type id) {
    with(id, [](auto& node) { node.integrate(); });
  }

  static bool insert(id_type id, id_type child, pos_type index) {
    return with(id, [&](auto& node) { return node.insert(child, index); });
  }

  static id_type split(id_type id, pos_type index) {
    return with(id, [&](auto& node) { return node.split(index); });
  }

  static bool erase(id_type id, pos_type index) {
    return with(id, [&](auto& node) { return node.erase(index); });
  }

  static std::size_t child_size(id_type id, pos_type index) {
    return with(id, [&](auto& node) { return node.child_size(index); });
  }

  template <bool WithLazy = false>
  static id_type locate(id_type id, std::size_t index,
                        bool* value_clean = nullptr) {
    return with(id, [&](auto& node) -> id_type {
      return node.template locate<WithLazy>(index, value_clean);
    });
  }
};

}  // namespace Bplustree
