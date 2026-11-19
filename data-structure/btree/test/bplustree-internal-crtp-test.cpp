#include <bits/stdc++.h>

#include "data-structure/btree/components/bplustree-node.hpp"

using namespace std;

/**
 * @brief CRTP により BottomInternal と上層 Internal の静的な処理経路を確認する。
 * @details
 * 本テストは継承・メンバ関数の名前解決と、従来の Internal& 互換経路を確認する。
 * 集約と Lazy を含む既存の全体テストと合わせて検証する。
 */
signed main() {
  using Traits = Bplustree::Traits<long long, 4>;
  using Upper = Bplustree::UpperInternal<Traits>;
  using Bottom = Bplustree::BottomInternal<Traits>;
  using Pools = Bplustree::NodePools<Traits>;

  static_assert(!Upper::UpperOps::is_bottom);
  static_assert(Bottom::BottomOps::is_bottom);
  static_assert(!is_base_of_v<Upper, Bottom>);
  static_assert(!is_base_of_v<Bottom, Upper>);
  static_assert(is_base_of_v<typename Traits::Internal, Upper>);
  static_assert(is_base_of_v<typename Traits::Internal, Bottom>);
  static_assert(is_base_of_v<
                Bplustree::InternalCRTP<Traits, Upper, false>, Upper>);
  static_assert(is_base_of_v<
                Bplustree::InternalCRTP<Traits, Bottom, true>, Bottom>);

  const auto x = Pools::make_leaf(10);
  const auto y = Pools::make_leaf(20);
  const auto bottom_id = Pools::make_internal(0);
  auto& bottom = Pools::bottom(bottom_id);

  bottom.template move_child<true, false>(0, x, bottom_id);
  bottom.template move_child<true, false>(1, y, bottom_id);
  bottom.refresh();
  assert(bottom.size == 2);
  assert(bottom.child_size(0) == 1);
  assert(bottom.locate(0) == x);
  assert(bottom.locate(1) == y);

  const auto root_id = Pools::make_internal(1);
  auto& root = Pools::uppers()[root_id];
  root.template move_child<true, false>(0, bottom_id, root_id);
  root.refresh();
  assert(root.size == 2);
  assert(root.child_size(0) == 2);
  assert(root.locate(0) == x);
  assert(root.locate(1) == y);
  assert(Pools::internals()[root_id].locate(1) == y);

  bool seen_upper = false;
  bool seen_bottom = false;
  Pools::with_internal(root_id, [&](auto& node) {
    if constexpr (is_same_v<remove_cvref_t<decltype(node)>, Upper>)
      seen_upper = true;
  });
  Pools::with_internal(bottom_id, [&](auto& node) {
    if constexpr (is_same_v<remove_cvref_t<decltype(node)>, Bottom>)
      seen_bottom = true;
  });
  assert(seen_upper && seen_bottom);

  // 従来の Internal& 経由でも level == 0 の互換 dispatch が動作する。
  auto& bottom_as_internal = Pools::internals()[bottom_id];
  assert(bottom_as_internal.locate(1) == y);
  bottom_as_internal.push_reverse();
  assert(bottom_as_internal.locate(0) == y);
  assert(bottom.locate(0) == y);

  Pools::recycle(root_id);
  cerr << "bplustree-internal-crtp-test passed!" << endl;
}
