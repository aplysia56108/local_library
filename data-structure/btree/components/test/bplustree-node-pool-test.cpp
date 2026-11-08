#include "data-structure/btree/components/bplustree-node.hpp"

#include <cassert>
#include <iostream>

/** @brief ID 管理の insert / split / erase と ID の再利用を確認する。 */
template <std::size_t Capacity>
void node_pool_basic_test() {
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Pools = Bplustree::NodePools<Traits>;
  const auto root_id = Pools::make_internal(0);
  for (std::size_t i = 0; i < Capacity; ++i) {
    const auto id = Pools::make_leaf(static_cast<long long>(i + 10));
    auto& root = Pools::internals()[root_id];
    assert(root.insert(id, i) == (i + 1 == Capacity));
  }
  assert(Pools::internals()[root_id].count == Capacity);
  for (std::size_t i = 0; i < Capacity; ++i) {
    const auto leaf_id = Pools::internals()[root_id].locate(i);
    const auto& leaf = Pools::leaves()[leaf_id];
    assert(leaf.get() == static_cast<long long>(i + 10));
    assert(leaf.parent == root_id && leaf.position == i);
  }

  const auto right_id = Pools::internals()[root_id].split();
  assert(Pools::internals()[root_id].count +
             Pools::internals()[right_id].count == Capacity);
  assert(Pools::internals()[root_id].count < Capacity);
  assert(Pools::internals()[right_id].count < Capacity);

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

/** @brief 兄弟を統合しても移動した Leaf ID が生存することを確認する。 */
void node_pool_integrate_test() {
  using Traits = Bplustree::Traits<long long, 6>;
  using Pools = Bplustree::NodePools<Traits>;
  auto root_id = Pools::make_internal(1);
  auto left_id = Pools::make_internal(0);
  auto right_id = Pools::make_internal(0);
  auto a = Pools::make_leaf(1);
  auto b = Pools::make_leaf(2);
  auto c = Pools::make_leaf(3);
  Pools::internals()[left_id].insert(a, 0);
  Pools::internals()[right_id].insert(b, 0);
  Pools::internals()[right_id].insert(c, 1);
  Pools::internals()[root_id].insert(left_id, 0);
  Pools::internals()[root_id].insert(right_id, 1);
  Pools::internals()[root_id].integrate_child(0);
  assert(Pools::internals()[root_id].count == 1);
  assert(Pools::internals()[root_id].locate(0) == a);
  assert(Pools::internals()[root_id].locate(1) == b);
  assert(Pools::internals()[root_id].locate(2) == c);
  assert(Pools::internals()[right_id].count == 3);
  Pools::recycle(root_id, false);
  std::cerr << "node pool integration tests all passed!\n";
}

int main() {
  node_pool_basic_test<4>();
  node_pool_basic_test<8>();
  node_pool_integrate_test();
  std::cerr << "All node pool test cases passed!\n";
}
