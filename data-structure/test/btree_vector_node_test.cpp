#include <cassert>
#include <functional>

#define private public
#include "data-structure/btree/btree-vector.hpp"
#undef private

using Tree = Btree_Vector<int, std::less<int>, 3>;

int main() {
  using Leaf = Tree::Leaf_Node;
  using Internal = Tree::Internal_Node;

  Leaf leaf(1);
  leaf.insert_non_full(3, 1);
  leaf.insert_non_full(5, 2);
  Leaf* right_leaf = leaf.split_and_insert(4, 2);
  assert(leaf.key_count == 2);
  assert(leaf.keys[0] == 1 && leaf.keys[1] == 3);
  assert(leaf.subtree_size == 2);
  assert(right_leaf->key_count == 2);
  assert(right_leaf->keys[0] == 4 && right_leaf->keys[1] == 5);
  assert(right_leaf->subtree_size == 2);
  assert(leaf.next == right_leaf && right_leaf->prev == &leaf);
  delete right_leaf;

  Internal* internal = new Internal();
  for (int key : {10, 20, 30, 40}) {
    auto* child = new Leaf(key);
    internal->children[internal->child_count++] = child;
  }
  internal->sync_metadata();
  assert(internal->subtree_size == 4);
  assert(internal->separators[0] == 20);
  assert(internal->separators[1] == 30);
  assert(internal->separators[2] == 40);

  Internal* right_internal =
      internal->split_and_insert(new Leaf(25), 2);
  assert(internal->child_count == 2);
  assert(right_internal->child_count == 3);
  assert(internal->subtree_size == 2);
  assert(right_internal->subtree_size == 3);
  assert(internal->separators[0] == 20);
  assert(right_internal->separators[0] == 30);
  assert(right_internal->separators[1] == 40);
  for (std::size_t i = 0; i < internal->child_count; ++i) {
    assert(internal->children[i]->parent == internal);
    assert(internal->children[i]->position == i);
  }
  for (std::size_t i = 0; i < right_internal->child_count; ++i) {
    assert(right_internal->children[i]->parent == right_internal);
    assert(right_internal->children[i]->position == i);
  }
  Tree::delete_subtree(internal);
  Tree::delete_subtree(right_internal);
}
