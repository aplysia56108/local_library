#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#define private public
#include "data-structure/set/btree-set.hpp"
#undef private

int main() {
  using Set = Btree_Set<int>;

  static_assert(std::default_initializable<Set>);
  static_assert(std::same_as<Set::key_type, int>);
  static_assert(std::same_as<Set::value_type, int>);
  static_assert(std::same_as<Set::size_type, std::size_t>);
  static_assert(std::same_as<Set::iterator, Btree_Set<int>::iterator>);
  static_assert(
      std::same_as<Set::const_iterator, Btree_Set<int>::const_iterator>);

  using SmallSet = Btree_Set<int, std::less<int>, 2>;
  using Node = SmallSet::Node;

  Node leaf;
  leaf.insert_non_full(1, nullptr, 0);
  leaf.insert_non_full(3, nullptr, 1);
  leaf.insert_non_full(5, nullptr, 2);

  const auto leaf_split = leaf.split_and_insert(4, nullptr, 2);
  assert(leaf.key_count == 1);
  assert(leaf.keys[0] == 1);
  assert(leaf.subtree_size == 1);
  assert(leaf_split.separator == 3);
  assert(leaf_split.right->key_count == 2);
  assert(leaf_split.right->keys[0] == 4);
  assert(leaf_split.right->keys[1] == 5);
  assert(leaf_split.right->subtree_size == 2);
  delete leaf_split.right;

  Node internal;
  internal.is_leaf = false;
  internal.key_count = 3;
  internal.keys[0] = 10;
  internal.keys[1] = 20;
  internal.keys[2] = 30;
  std::array<Node, 5> children = {Node(1), Node(2), Node(3), Node(4),
                                  Node(5)};
  for (std::size_t i = 0; i < 4; ++i) {
    internal.children[i] = &children[i];
    children[i].parent = &internal;
    children[i].position = i;
  }
  internal.recalculate_subtree_size();

  const auto internal_split = internal.split_and_insert(25, &children[4], 2);
  assert(internal.key_count == 1);
  assert(internal.keys[0] == 10);
  assert(internal.children[0] == &children[0]);
  assert(internal.children[1] == &children[1]);
  assert(internal_split.separator == 20);
  assert(internal_split.right->key_count == 2);
  assert(internal_split.right->keys[0] == 25);
  assert(internal_split.right->keys[1] == 30);
  assert(internal_split.right->children[0] == &children[2]);
  assert(internal_split.right->children[1] == &children[4]);
  assert(internal_split.right->children[2] == &children[3]);
  assert(children[4].parent == internal_split.right);
  assert(children[4].position == 1);
  delete internal_split.right;

  SmallSet set;
  for (int i = 0; i < 199; ++i) {
    const int key = (i * 73) % 199;
    const auto [it, inserted] = set.insert(key);
    (void)it;
    assert(inserted);
    assert(set.size() == static_cast<std::size_t>(i + 1));
  }
  for (int key = 0; key < 199; ++key) {
    const auto [it, inserted] = set.insert(key);
    (void)it;
    assert(!inserted);
    assert(set.size() == 199);

    const auto [node, key_index] = set.locate(key);
    assert(node != nullptr);
    assert(node->keys[key_index] == key);
  }

  const auto verify_subtree = [&](const auto& self, const Node* node,
                                  bool is_root) -> std::size_t {
    assert(node != nullptr);
    assert(node->key_count <= 3);
    if (!is_root) assert(node->key_count >= 1);
    for (std::size_t i = 1; i < node->key_count; ++i) {
      assert(node->keys[i - 1] < node->keys[i]);
    }

    std::size_t subtree_size = node->key_count;
    if (!node->is_leaf) {
      for (std::size_t i = 0; i <= node->key_count; ++i) {
        assert(node->children[i]->parent == node);
        assert(node->children[i]->position == i);
        subtree_size += self(self, node->children[i], false);
      }
    }
    assert(node->subtree_size == subtree_size);
    return subtree_size;
  };
  assert(verify_subtree(verify_subtree, set.root_, true) == set.size());

  set.clear();
  assert(set.empty());
  assert(set.size() == 0);
}
