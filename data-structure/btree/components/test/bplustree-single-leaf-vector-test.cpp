#include "data-structure/btree/components/bplustree-base.hpp"

#include <cassert>
#include <iostream>

// One-value-per-Leaf: unrelated structural changes preserve iterator identity.
// The pool may relocate Leaf objects, so value addresses are not stable.
// No randomized operations; insertion/deletion positions are intentional.
template <std::size_t Capacity>
void test_stable_leaf_iterators() {
  using Tree = Bplustree::Base<Bplustree::Traits<long long, Capacity>>;
  Tree tree;
  for (long long x = 0; x < 12; ++x) tree.push_back(100 + x);

  auto first = tree.nth(1), moved = tree.nth(3);
  auto middle = tree.nth(5), last = tree.nth(10);
  const auto first_copy = first, moved_copy = moved;
  const auto middle_copy = middle, last_copy = last;
  auto before = tree.bbegin(), end = tree.end();

  // insert(index/iterator): existing iterators keep the same Leaf identity.
  tree.insert(0, -1);
  tree.insert(tree.nth(4), -2);
  tree.insert(tree.end(), -3);
  assert(*first == 101 && *moved == 103 && *middle == 105 && *last == 110);
  assert(first == first_copy && moved == moved_copy &&
         middle == middle_copy && last == last_copy);
  assert(first == tree.nth(2) && moved == tree.nth(5));
  assert(middle == tree.nth(7) && last == tree.nth(12));
  assert(before == tree.bbegin() && end == tree.end());

  // erase(index): only erased elements lose their identity.
  tree.erase(0);
  tree.erase(3);
  tree.erase(tree.size() - 1);
  assert(*first == 101 && *moved == 103 && *middle == 105 && *last == 110);
  assert(first == first_copy && moved == moved_copy &&
         middle == middle_copy && last == last_copy);
  assert(first == tree.nth(1) && moved == tree.nth(3));
  assert(middle == tree.nth(5) && last == tree.nth(10));
  assert(first + 4 == middle && middle + 5 == last);

  // reverse(range): an iterator inside the reversed range follows its Leaf.
  tree.reverse(2, 9);
  assert(*first == 101 && *moved == 103 && *middle == 105 && *last == 110);
  assert(first == first_copy && moved == moved_copy &&
         middle == middle_copy && last == last_copy);
  assert(first == tree.nth(1) && moved == tree.nth(7));
  assert(middle == tree.nth(5) && last == tree.nth(10));
  assert(before == tree.bbegin() && end == tree.end());

  std::cerr << "single-leaf iterator stability tests all passed! (capacity="
            << Capacity << ")\n";
}

int main() {
  test_stable_leaf_iterators<4>();
  test_stable_leaf_iterators<8>();

  std::cerr << "All single-leaf vector test cases passed!\n";
}
