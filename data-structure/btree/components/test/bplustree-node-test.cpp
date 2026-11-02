#include "data-structure/btree/components/bplustree-node.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

template <std::size_t Capacity>
void test_leaf_and_internal() {
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Node = typename Traits::Node;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  const auto before = Node::live_nodes;
  {
    // Node::is_leaf, Leaf::get/set, Internal::insert/sync/refresh/locate.
    Internal root(0);
    for (std::size_t i = 0; i < Capacity - 1; ++i) {
      auto leaf = std::make_unique<Leaf>(static_cast<long long>(10 + i));
      assert(leaf->is_leaf());
      assert(!root.insert(std::move(leaf), i));
    }
    assert(!root.is_leaf());
    assert(root.count == Capacity - 1 && root.size == Capacity - 1);
    for (std::size_t i = 0; i < root.count; ++i) {
      auto* leaf = root.locate(i);
      assert(leaf->get() == static_cast<long long>(10 + i));
      assert(leaf->parent == &root && leaf->position == i);
    }
    root.locate(0)->set(42);
    assert(root.locate<true>(0)->get() == 42);

    // Internal::erase, compact children and repair parent/position.
    assert(!root.erase(0));
    assert(root.count == Capacity - 2);
    assert(root.children[root.count] == nullptr);
    assert(root.locate(0)->get() == 11);
    assert(root.locate(0)->position == 0);
  }
  {
    // Internal::insert full state and Internal::split(left_count).
    Internal root(0);
    for (std::size_t i = 0; i < Capacity; ++i) {
      auto leaf = std::make_unique<Leaf>(static_cast<long long>(i));
      assert(root.insert(std::move(leaf), i) == (i + 1 == Capacity));
    }
    auto right = root.split();
    assert(root.count + right->count == Capacity);
    assert(root.count < Capacity && right->count < Capacity);
    assert(root.size + right->size == Capacity);
    for (std::size_t i = 0; i < right->count; ++i) {
      assert(right->children[i]->parent == right.get());
      assert(right->children[i]->position == i);
    }
  }
  assert(Node::live_nodes == before);

  std::cerr << "node leaf/internal tests all passed! (capacity=" << Capacity << ")\n";}

void test_integrate() {
  using Traits = Bplustree::Traits<long long, 6>;
  using Leaf = Traits::Leaf;
  using Internal = Traits::Internal;

  // integrate_child(0): absorb the specified left child into its right sibling.
  auto root = std::make_unique<Internal>(1);
  auto left = std::make_unique<Internal>(0);
  auto right = std::make_unique<Internal>(0);
  left->insert(std::make_unique<Leaf>(1), 0);
  right->insert(std::make_unique<Leaf>(2), 0);
  right->insert(std::make_unique<Leaf>(3), 1);
  auto* survivor = right.get();
  root->insert(std::move(left), 0);
  root->insert(std::move(right), 1);
  root->integrate_child(0);
  assert(root->count == 1 && root->size == 3);
  assert(root->get_child(0) == survivor);
  assert(survivor->count == 3);
  assert(root->locate(0)->get() == 1);
  assert(root->locate(1)->get() == 2);
  assert(root->locate(2)->get() == 3);

  // integrate_child(1): absorb specified right child into left sibling.
  auto extra = std::make_unique<Internal>(0);
  extra->insert(std::make_unique<Leaf>(4), 0);
  root->insert(std::move(extra), 1);
  root->get_child(1)->integrate();  // The calling Internal may be destroyed.
  assert(root->count == 1 && root->size == 4);
  assert(root->get_child(0) == survivor);
  assert(root->locate(3)->get() == 4);

  std::cerr << "node integration tests all passed!\n";}

int main() {
  test_leaf_and_internal<3>();
  test_leaf_and_internal<4>();
  test_leaf_and_internal<6>();
  test_integrate();

  std::cerr << "All node test cases passed!\n";}
