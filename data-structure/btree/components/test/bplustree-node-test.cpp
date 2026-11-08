#include "data-structure/btree/components/bplustree-node.hpp"

#include <cassert>
#include <iostream>

/** @brief Leaf / Internal の位置情報と反転・部分木回収を確認する。 */
template <std::size_t Capacity>
void node_structure_test() {
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Pools = Bplustree::NodePools<Traits>;
  Pools::reserve_additional(Capacity + 2, 4);
  const auto root_id = Pools::make_internal(0);
  for (std::size_t i = 0; i < Capacity - 1; ++i) {
    const auto leaf_id = Pools::make_leaf(i + 1);
    assert(!Pools::internals()[root_id].insert(leaf_id, i));
  }

  auto& root = Pools::internals()[root_id];
  assert(root.count == Capacity - 1 && root.size == Capacity - 1);
  root.push_reverse();
  root.materialize();
  for (std::size_t i = 0; i < root.count; ++i) {
    const auto leaf_id = root.locate(i);
    assert(Pools::leaves()[leaf_id].get() == Capacity - 1 - i);
    assert(Pools::leaves()[leaf_id].position == i);
    assert(Pools::leaves()[leaf_id].parent == root_id);
  }

  const auto removed_id = root.locate(0);
  assert(!root.erase(0));
  assert(root.count == Capacity - 2);
  assert(root.children[root.count] == Pools::null_id);
  assert(Pools::leaves()[root.locate(0)].get() == Capacity - 2);

  // 回収された ID が再利用される。古い Leaf ID は無効扱い。
  assert(Pools::make_leaf(999) == removed_id);
  Pools::recycle(root_id, false);
  Pools::leaves().erase(removed_id);
  std::cerr << "node structure tests all passed! (capacity=" << Capacity << ")\n";
}

/** @brief 2階層の parent / position と再帰的な recycle を確認する。 */
void node_subtree_test() {
  using Traits = Bplustree::Traits<long long, 5>;
  using Pools = Bplustree::NodePools<Traits>;
  Pools::reserve_additional(5, 4);
  const auto root_id = Pools::make_internal(1);
  const auto child_id = Pools::make_internal(0);
  const auto a = Pools::make_leaf(10);
  const auto b = Pools::make_leaf(20);
  Pools::internals()[child_id].insert(a, 0);
  Pools::internals()[child_id].insert(b, 1);
  Pools::internals()[root_id].insert(child_id, 0);

  assert(Pools::internals()[root_id].size == 2);
  assert(Pools::internals()[root_id].locate(0) == a);
  assert(Pools::internals()[root_id].locate(1) == b);
  assert(Pools::internals()[child_id].parent == root_id);
  assert(Pools::internals()[child_id].position == 0);

  Pools::recycle(root_id, false);
  assert(Pools::make_leaf(30) == b);
  const auto reused_a = Pools::make_leaf(40);
  assert(reused_a == a);
  const auto reused_internal = Pools::make_internal(0);
  assert(reused_internal == root_id);
  Pools::leaves().erase(b);
  Pools::leaves().erase(reused_a);
  Pools::internals().erase(reused_internal);
  std::cerr << "node subtree tests all passed!\n";
}

int main() {
  node_structure_test<4>();
  node_structure_test<8>();
  node_subtree_test();
  std::cerr << "All node test cases passed!\n";
}
