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
}
