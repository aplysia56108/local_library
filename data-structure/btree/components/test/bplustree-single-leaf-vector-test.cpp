#include "data-structure/btree/components/bplustree-base.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

// One-value-per-Leaf: unrelated structural changes preserve element identity.
// No randomized operations; the insertion/deletion positions are intentional.
template <std::size_t Capacity>
void test_stable_leaf_addresses() {
  using Tree = Bplustree::Base<Bplustree::Traits<long long, Capacity>>;
  Tree tree;
  for (long long x = 0; x < 12; ++x) tree.push_back(100 + x);

  auto first = tree.nth(1), middle = tree.nth(5), last = tree.nth(10);
  const auto* p1 = std::addressof(*first);
  const auto* p5 = std::addressof(*middle);
  const auto* p10 = std::addressof(*last);
  auto before = tree.bbegin(), end = tree.end();

  // insert(index/iterator): preserved Leaf addresses and iterators.
  tree.insert(0, -1);
  tree.insert(tree.nth(4), -2);
  tree.insert(tree.end(), -3);
  assert(*first == 101 && *middle == 105 && *last == 110);
  assert(std::addressof(*first) == p1);
  assert(std::addressof(*middle) == p5);
  assert(std::addressof(*last) == p10);
  assert(before == tree.bbegin() && end == tree.end());

  // erase(index): only erased elements lose their identity.
  tree.erase(0);
  tree.erase(3);
  tree.erase(tree.size() - 1);
  assert(first + 4 == middle && middle + 5 == last);
  assert(std::addressof(*first) == p1);
  assert(std::addressof(*middle) == p5);
  assert(std::addressof(*last) == p10);

  // reverse(range): Leaf addresses remain unchanged.
  tree.reverse(2, 9);
  assert(std::addressof(*first) == p1);
  assert(std::addressof(*middle) == p5);
  assert(std::addressof(*last) == p10);
  assert(before == tree.bbegin() && end == tree.end());

  std::cerr << "single-leaf iterator stability tests all passed! (capacity=" << Capacity << ")\n";
}

int main() {
  test_stable_leaf_addresses<4>();
  test_stable_leaf_addresses<8>();

  std::cerr << "All single-leaf vector test cases passed!\n";
}
