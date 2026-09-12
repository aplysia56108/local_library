#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

#define private public
#include "data-structure/set/btree-set.hpp"
#undef private

using SmallSet = Btree_Set<int, std::less<int>, 2>;
using Node = SmallSet::Node;

std::size_t verify_node(const Node* node, const Node* parent,
                        std::size_t position, std::optional<int> lower,
                        std::optional<int> upper, bool is_root) {
  assert(node != nullptr);
  assert(node->parent == parent);
  assert(node->position == position);
  assert(node->key_count <= SmallSet::kMaxKeys);
  if (!is_root) assert(node->key_count >= 1);

  for (std::size_t i = 0; i < node->key_count; ++i) {
    if (lower) assert(*lower < node->keys[i]);
    if (upper) assert(node->keys[i] < *upper);
    if (i > 0) assert(node->keys[i - 1] < node->keys[i]);
  }

  std::size_t subtree_size = node->key_count;
  if (node->is_leaf) {
    for (Node* child : node->children) {
      assert(child == nullptr);
    }
  } else {
    // 内部ノードの有効な子は key_count + 1 個である。
    for (std::size_t i = 0; i <= node->key_count; ++i) {
      assert(node->children[i] != nullptr);
      const std::optional<int> child_lower =
          i == 0 ? lower : std::optional<int>{node->keys[i - 1]};
      const std::optional<int> child_upper =
          i == node->key_count ? upper : std::optional<int>{node->keys[i]};
      subtree_size += verify_node(node->children[i], node, i, child_lower,
                                  child_upper, false);
    }
    for (std::size_t i = node->key_count + 1;
         i < SmallSet::kMaxChildren; ++i) {
      assert(node->children[i] == nullptr);
    }
  }

  assert(node->subtree_size == subtree_size);
  return subtree_size;
}

void verify_tree(const SmallSet& set) {
  assert(set.root_ != nullptr);
  assert(verify_node(set.root_, nullptr, 0, std::nullopt, std::nullopt, true) ==
         set.size());
}

int main() {
  using Set = Btree_Set<int>;

  static_assert(std::default_initializable<Set>);
  static_assert(std::same_as<Set::key_type, int>);
  static_assert(std::same_as<Set::value_type, int>);
  static_assert(std::same_as<Set::size_type, std::size_t>);
  static_assert(std::same_as<Set::iterator, Btree_Set<int>::iterator>);
  static_assert(
      std::same_as<Set::const_iterator, Btree_Set<int>::const_iterator>);

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

  verify_tree(set);

  set.clear();
  assert(set.empty());
  assert(set.size() == 0);
}
