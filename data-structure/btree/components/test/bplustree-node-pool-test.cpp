#include "data-structure/btree/components/bplustree-base.hpp"

#include <array>
#include <cassert>
#include <iostream>

/** @brief Leaf の安定 ID と BottomNode の分割・削除を検証する。 */
template <std::size_t Capacity>
void node_pool_basic_test() {
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Pools = Bplustree::NodePools<Traits>;
  Pools::reserve_buffer(Capacity + 2, 3);
  const auto root_id = Pools::make_bottom();
  assert(Pools::template get<Bplustree::NodeType::Bottom>(root_id).id() ==
         root_id);
  for (std::size_t i = 0; i < Capacity; ++i) {
    const auto id = Pools::make_leaf(static_cast<long long>(10 + i));
    assert(Pools::leaves().index(Pools::leaves()[id]) == id);
    assert(Pools::template get<Bplustree::NodeType::Bottom>(root_id).insert(
               id, i) == (i + 1 == Capacity));
  }
  assert(Pools::template get<Bplustree::NodeType::Bottom>(root_id).count ==
         Capacity);
  const auto right_id =
      Pools::template get<Bplustree::NodeType::Bottom>(root_id).split();
  assert(Pools::template get<Bplustree::NodeType::Bottom>(root_id).count +
             Pools::template get<Bplustree::NodeType::Bottom>(right_id).count ==
         Capacity);
  for (const auto node_id : {root_id, right_id}) {
    auto& node = Pools::template get<Bplustree::NodeType::Bottom>(node_id);
    for (std::size_t i = 0; i < node.count; ++i) {
      const auto& leaf = Pools::leaves()[node.locate(i)];
      assert(leaf.parent == node_id && leaf.position == i);
    }
  }

  const auto removed_id =
      Pools::template get<Bplustree::NodeType::Bottom>(root_id).locate(0);
  Pools::template get<Bplustree::NodeType::Bottom>(root_id).erase(0);
  const auto reused = Pools::make_leaf(99);
  assert(reused == removed_id);
  assert(Pools::leaves()[reused].get() == 99);
  Pools::recycle(root_id);
  Pools::recycle(right_id);
  Pools::recycle(reused);
  std::cerr << "node pool basic tests all passed! (capacity=" << Capacity
            << ")\n";
}

/** @brief split 中の BottomNode Pool 再確保でも ID から再取得できる。 */
void node_pool_reallocation_test() {
  using Traits = Bplustree::Traits<long long, 7>;
  using Pools = Bplustree::NodePools<Traits>;
  const auto root_id = Pools::make_bottom();
  for (long long i = 0; i < 7; ++i)
    Pools::template get<Bplustree::NodeType::Bottom>(root_id).insert(
        Pools::make_leaf(100 + i), i);
  std::array<typename Pools::id_type, 7> padding;
  for (auto& id : padding) id = Pools::make_bottom();
  const auto right_id =
      Pools::template get<Bplustree::NodeType::Bottom>(root_id).split();
  assert(Pools::template get<Bplustree::NodeType::Bottom>(root_id).count +
             Pools::template get<Bplustree::NodeType::Bottom>(right_id).count ==
         7);
  assert(Pools::template get<Bplustree::NodeType::Bottom>(right_id).locate(0) !=
         Pools::null_id);
  Pools::recycle(root_id);
  Pools::recycle(right_id);
  for (const auto id : padding) Pools::recycle(id);
  std::cerr << "node pool reallocation tests all passed!\n";
}

/** @brief 兄弟の吸収方向と、移動後の Leaf ID の生存を検証する。 */
void node_pool_integrate_test() {
  using Traits = Bplustree::Traits<long long, 6>;
  using Pools = Bplustree::NodePools<Traits>;
  Pools::reserve_buffer(8, 6);
  const auto root_id = Pools::make_upper(1);
  const auto left_id = Pools::make_bottom();
  const auto right_id = Pools::make_bottom();
  const auto a = Pools::make_leaf(1);
  const auto b = Pools::make_leaf(2);
  const auto c = Pools::make_leaf(3);
  Pools::template get<Bplustree::NodeType::Bottom>(left_id).insert(a, 0);
  Pools::template get<Bplustree::NodeType::Bottom>(right_id).insert(b, 0);
  Pools::template get<Bplustree::NodeType::Bottom>(right_id).insert(c, 1);
  auto& root = Pools::template get<Bplustree::NodeType::Upper>(root_id);
  root.insert(left_id, 0);
  root.insert(right_id, 1);
  root.integrate_child(0);
  assert(root.count == 1 && root.children[0] == right_id);
  for (std::size_t i = 0; i < 3; ++i)
    assert(Pools::leaves()[root.locate(i)].get() ==
           static_cast<long long>(i + 1));

  const auto extra_id = Pools::make_bottom();
  const auto d = Pools::make_leaf(4);
  Pools::template get<Bplustree::NodeType::Bottom>(extra_id).insert(d, 0);
  root.insert(extra_id, 1);
  Pools::template get<Bplustree::NodeType::Bottom>(extra_id).integrate();
  assert(root.count == 1 && root.children[0] == right_id);
  assert(root.locate(3) == d && Pools::leaves()[d].parent == right_id);
  Pools::recycle(root_id);
  std::cerr << "node pool integration tests all passed!\n";
}

Bplustree::Base<Bplustree::Traits<long long, 12>> global_tree{1, 2, 3};

int main() {
  assert(global_tree.size() == 3);
  assert(global_tree.get(0) == 1 && global_tree.get(2) == 3);
  node_pool_basic_test<4>();
  node_pool_basic_test<8>();
  node_pool_reallocation_test();
  node_pool_integrate_test();
  std::cerr << "All node pool test cases passed!\n";
}
