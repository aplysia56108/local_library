#include "data-structure/btree/components/bplustree-node.hpp"

#include <cassert>
#include <iostream>

/** @brief Leaf の安定 ID と Internal の分割・削除を検証する。 */
template <std::size_t Capacity>
void node_pool_basic_test() {
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Pools = Bplustree::NodePools<Traits>;
  Pools::reserve_additional(Capacity + 2, 3);
  const auto root_id = Pools::make_internal(0);
  assert(Pools::internals()[root_id].id() == root_id);
  for (std::size_t i = 0; i < Capacity; ++i) {
    const auto id = Pools::make_leaf(static_cast<long long>(10 + i));
    assert(Pools::leaves().index(Pools::leaves()[id]) == id);
    assert(Pools::internals()[root_id].insert(id, i) == (i + 1 == Capacity));
  }
  auto& root = Pools::internals()[root_id];
  assert(root.count == Capacity);
  for (std::size_t i = 0; i < Capacity; ++i) {
    const auto leaf_id = root.locate(i);
    const auto& leaf = Pools::leaves()[leaf_id];
    assert(leaf.get() == static_cast<long long>(10 + i));
    assert(leaf.parent == root_id && leaf.position == i);
  }

  const auto right_id = root.split();
  assert(Pools::internals()[root_id].count +
             Pools::internals()[right_id].count == Capacity);
  assert(Pools::internals()[right_id].id() == right_id);
  for (auto node_id : {root_id, right_id}) {
    auto& node = Pools::internals()[node_id];
    for (std::size_t i = 0; i < node.count; ++i) {
      const auto& leaf = Pools::leaves()[node.locate(i)];
      assert(leaf.parent == node_id && leaf.position == i);
    }
  }

  const auto removed_id = Pools::internals()[root_id].locate(0);
  Pools::internals()[root_id].erase(0);
  const auto reused = Pools::make_leaf(99);
  assert(reused == removed_id);
  assert(Pools::leaves()[reused].get() == 99);
  Pools::recycle(root_id, false);
  Pools::recycle(right_id, false);
  Pools::leaves().erase(reused);
  std::cerr << "node pool basic tests all passed! (capacity=" << Capacity << ")\n";
}

/** @brief split 中の Pool 再確保でも ID からノードを再取得できる。 */
void node_pool_reallocation_test() {
  using Traits = Bplustree::Traits<long long, 7>;
  using Pools = Bplustree::NodePools<Traits>;
  const auto root_id = Pools::make_internal(0);
  for (int i = 0; i < 7; ++i) {
    auto id = Pools::make_leaf(100 + i);
    Pools::internals()[root_id].insert(id, i);
  }
  // reserve は意図的に行わず、split の emplace による再確保を許容する。
  const auto right_id = Pools::internals()[root_id].split();
  assert(Pools::internals()[root_id].count +
             Pools::internals()[right_id].count == 7);
  assert(Pools::internals()[right_id].locate(0) != Pools::null_id);
  Pools::recycle(root_id, false);
  Pools::recycle(right_id, false);
  std::cerr << "node pool reallocation tests all passed!\n";
}

/** @brief 兄弟の吸収方向と、移動後の Leaf ID の生存を検証する。 */
void node_pool_integrate_test() {
  using Traits = Bplustree::Traits<long long, 6>;
  using Pools = Bplustree::NodePools<Traits>;
  Pools::reserve_additional(8, 6);
  const auto root_id = Pools::make_internal(1);
  const auto left_id = Pools::make_internal(0);
  const auto right_id = Pools::make_internal(0);
  const auto a = Pools::make_leaf(1);
  const auto b = Pools::make_leaf(2);
  const auto c = Pools::make_leaf(3);
  Pools::internals()[left_id].insert(a, 0);
  Pools::internals()[right_id].insert(b, 0);
  Pools::internals()[right_id].insert(c, 1);
  Pools::internals()[root_id].insert(left_id, 0);
  Pools::internals()[root_id].insert(right_id, 1);
  Pools::internals()[root_id].integrate_child(0);
  assert(Pools::internals()[root_id].count == 1);
  assert(Pools::internals()[root_id].children[0] == right_id);
  for (int i = 0; i < 3; ++i)
    assert(Pools::leaves()[Pools::internals()[root_id].locate(i)].get() == i + 1);
  for (auto id : {a, b, c})
    assert(Pools::leaves()[id].parent == right_id);

  const auto extra_id = Pools::make_internal(0);
  const auto d = Pools::make_leaf(4);
  Pools::internals()[extra_id].insert(d, 0);
  Pools::internals()[root_id].insert(extra_id, 1);
  Pools::internals()[extra_id].integrate();
  assert(Pools::internals()[root_id].count == 1);
  assert(Pools::internals()[root_id].children[0] == right_id);
  assert(Pools::internals()[root_id].locate(3) == d);
  assert(Pools::leaves()[d].parent == right_id);
  Pools::recycle(root_id, false);
  std::cerr << "node pool integration tests all passed!\n";
}

int main() {
  node_pool_basic_test<4>();
  node_pool_basic_test<8>();
  node_pool_reallocation_test();
  node_pool_integrate_test();
  std::cerr << "All node pool test cases passed!\n";
}
