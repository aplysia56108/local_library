#include "data-structure/btree/components/bplustree-node.hpp"

#include <cassert>
#include <iostream>

/** @brief Leaf / BottomNode の位置情報と反転・部分木回収を確認する。 */
template <std::size_t Capacity>
void node_structure_test() {
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Pools = Bplustree::NodePools<Traits>;
  Pools::reserve_buffer(Capacity + 2, 4);
  const auto root_id = Pools::make_bottom();
  for (std::size_t i = 0; i < Capacity - 1; ++i)
    assert(!Pools::template get<Bplustree::NodeType::Bottom>(root_id).insert(
        Pools::make_leaf(static_cast<long long>(i + 1)), i));

  auto& root = Pools::template get<Bplustree::NodeType::Bottom>(root_id);
  assert(root.count == Capacity - 1 && root.size == Capacity - 1);
  root.push_reverse();
  root.materialize();
  for (std::size_t i = 0; i < root.count; ++i) {
    const auto leaf_id = root.locate(i);
    assert(Pools::leaves()[leaf_id].get() ==
           static_cast<long long>(Capacity - 1 - i));
    assert(Pools::leaves()[leaf_id].position == i);
    assert(Pools::leaves()[leaf_id].parent == root_id);
  }

  const auto removed_id = root.locate(0);
  assert(!root.erase(0));
  assert(root.count == Capacity - 2);
  assert(Pools::leaves()[root.locate(0)].get() ==
         static_cast<long long>(Capacity - 2));
  assert(Pools::make_leaf(999) == removed_id);
  Pools::recycle(root_id);
  Pools::recycle(removed_id);
  std::cerr << "node structure tests all passed! (capacity=" << Capacity
            << ")\n";
}

/** @brief 2階層の parent / position と再帰的な recycle を確認する。 */
void node_subtree_test() {
  using Traits = Bplustree::Traits<long long, 5>;
  using Pools = Bplustree::NodePools<Traits>;
  Pools::reserve_buffer(5, 4);
  const auto root_id = Pools::make_upper(1);
  const auto child_id = Pools::make_bottom();
  const auto a = Pools::make_leaf(10);
  const auto b = Pools::make_leaf(20);
  auto& child = Pools::template get<Bplustree::NodeType::Bottom>(child_id);
  child.insert(a, 0);
  child.insert(b, 1);
  auto& root = Pools::template get<Bplustree::NodeType::Upper>(root_id);
  root.insert(child_id, 0);

  assert(root.size == 2 && root.locate(0) == a && root.locate(1) == b);
  assert(child.parent == root_id && child.position == 0);

  Pools::recycle(root_id);
  assert(Pools::make_leaf(30) == b);
  const auto reused_a = Pools::make_leaf(40);
  assert(reused_a == a);
  const auto reused_upper = Pools::make_upper(1);
  assert(reused_upper == root_id);
  Pools::recycle(b);
  Pools::recycle(reused_a);
  Pools::recycle(reused_upper);
  std::cerr << "node subtree tests all passed!\n";
}

int main() {
  node_structure_test<4>();
  node_structure_test<8>();
  node_subtree_test();
  std::cerr << "All node test cases passed!\n";
}
