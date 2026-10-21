#include "data-structure/btree/components/bplustree-node.hpp"

#include <cassert>
#include <string>
#include <vector>

template <std::size_t Capacity>
void test_nodes() {
  using Traits = Bplustree::Traits<int, Capacity>;
  using Node = typename Traits::Node;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  {
    Leaf leaf;
    assert(leaf.locate(-1).first == &leaf && leaf.locate(-1).second == -1);
    Node* empty = &leaf;
    assert(empty->locate(-1).second == -1 && empty->locate(0).second == 0);
    assert(empty->locate_with_lazy(-1).second == -1);
    for (std::size_t i = 0; i < Capacity; ++i)
      assert(leaf.insert(static_cast<int>(i), i) == (i + 1 == Capacity));
    auto right = leaf.split();
    assert(leaf.count + right->count == Capacity);
    assert(leaf.count < Capacity && right->count < Capacity);
    for (std::size_t i = 0; i < right->count; ++i)
      assert(right->values[i] == static_cast<int>(leaf.count + i));
    leaf.erase(0);
    assert(leaf.size == leaf.count);
  }
  {
    Leaf leaf;
    for (std::size_t i = 0; i < Capacity - 1; ++i)
      assert(!leaf.insert(i, i));
    leaf.erase(0);  // erase is only valid in the stable, non-full state.
    assert(leaf.count == Capacity - 2);
    for (std::size_t i = 0; i < leaf.count; ++i)
      assert(leaf.values[i] == static_cast<int>(i + 1));
  }
  {
    Internal root;
    for (std::size_t i = 0; i < Capacity; ++i) {
      auto leaf = std::make_unique<Leaf>();
      leaf->insert(static_cast<int>(i), 0);
      assert(root.insert(std::move(leaf), i) == (i + 1 == Capacity));
    }
    // Full insertions must be split before locate / erase / merge / shrink.
    auto right = root.split();
    assert(root.count < Capacity && right->count < Capacity);
    assert(root.count + right->count == Capacity);
    assert(root.size + right->size == Capacity);
    for (std::size_t i = 0; i < right->count; ++i)
      assert(right->children[i]->parent == right.get());
  }
  {
    Internal root;
    for (std::size_t i = 0; i < Capacity - 1; ++i) {
      auto leaf = std::make_unique<Leaf>();
      leaf->insert(static_cast<int>(i), 0);
      assert(!root.insert(std::move(leaf), i));
    }
    assert(root.count == Capacity - 1 && root.size == Capacity - 1 && root.level == 1);
    auto before = root.locate(-1);
    assert(before.first == root.locate(0).first && before.second == -1);
    auto lazy_before = root.locate_with_lazy(-1);
    assert(lazy_before == before);
    root.erase(1);
    assert(root.count == Capacity - 2 && root.size == Capacity - 2);
    assert(!root.children[root.count]);
    for (std::size_t i = 0; i < root.count; ++i) {
      assert(root.children[i]->parent == &root && root.children[i]->position == i);
      assert(root.locate(i).first->values[0] == static_cast<int>(i + (i >= 1)));
    }
    auto right = root.split(1);
    assert(root.count == 1 && right->count == Capacity - 3);
    assert(root.size + right->size == Capacity - 2);
    for (std::size_t i = 0; i < right->count; ++i)
      assert(right->children[i]->parent == right.get());
  }
  for (bool right_end : {false, true}) {
    Internal root;
    for (int i = 0; i < 2; ++i) {
      auto leaf = std::make_unique<Leaf>();
      leaf->insert(10 + i, 0);
      root.insert(std::move(leaf), i);
    }
    root.shrink(right_end);
    assert(root.count == 1 && root.size == 2);
    assert(root.locate(0).first->values[0] == 10);
    assert(root.locate(1).first->values[1] == 11);
  }
  for (bool right_end : {false, true}) {
    Internal root;
    for (int i = 0; i < 2; ++i) {
      auto internal = std::make_unique<Internal>();
      auto leaf = std::make_unique<Leaf>();
      leaf->insert(20 + i, 0);
      internal->insert(std::move(leaf), 0);
      root.insert(std::move(internal), i);
    }
    assert(root.level == 2);
    root.shrink(right_end);
    assert(root.count == 1 && root.size == 2);
    auto* child = static_cast<Internal*>(root.children[0].get());
    assert(child->count == 1 && child->size == 2);
    assert(root.locate(0).first->values[0] == 20);
    assert(root.locate(1).first->values[1] == 21);
    assert(child->children[0]->parent == child);
  }
  for (std::size_t index : {0u, 1u}) {
    Internal root;
    for (int value : {30, 31}) {
      auto leaf = std::make_unique<Leaf>();
      leaf->insert(value, 0);
      root.insert(std::move(leaf), root.count);
    }
    root.merge_child(index);  // Exercise both sibling-selection branches.
    assert(root.count == 1 && root.size == 2);
    assert(root.locate(0).first->values[0] == 30);
    assert(root.locate(1).first->values[1] == 31);
    root.erase(0);
    assert(root.count == 0 && root.size == 0);
    root.shrink(false);
  }
  for (std::size_t total : {Capacity - 1, Capacity}) {
    Internal root;
    auto left = std::make_unique<Leaf>();
    left->insert(0, 0);
    root.insert(std::move(left), 0);
    auto right = std::make_unique<Leaf>();
    for (std::size_t i = 1; i < total; ++i)
      assert(!right->insert(static_cast<int>(i), i - 1));
    root.insert(std::move(right), 1);
    root.merge_child(0);
    // A merge may use NodeCap-1 entries, but may not fill the final slot.
    assert(root.count == (total < Capacity ? 1u : 2u));
    assert(root.size == total);
    for (std::size_t i = 0; i < total; ++i) {
      auto [leaf, index] = root.locate(i);
      assert(leaf->count < Capacity && leaf->values[index] == static_cast<int>(i));
    }
  }
  assert(Node::live_nodes == 0 && Leaf::leaf_count == 0 && Internal::internal_count == 0);
}

int main() {
  test_nodes<3>();
  test_nodes<4>();
  test_nodes<5>();
  using StringLeaf = Bplustree::Traits<std::string, 4>::Leaf;
  StringLeaf leaf;
  leaf.insert("left", 0);
  leaf.insert("middle", 1);
  leaf.insert("right", 2);
  leaf.erase(1);
  auto right = leaf.split(1);
  assert(leaf.values[0] == "left" && right->values[0] == "right");
}
