#include "data-structure/btree/components/bplustree-base.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <compare>
#include <concepts>
#include <iterator>
#include <type_traits>
#include <vector>

using Tree = Bplustree::Base<Bplustree::Traits<long long, 4>>;
using It = Tree::iterator;
using CIt = Tree::const_iterator;
static_assert(std::random_access_iterator<It>);
static_assert(std::random_access_iterator<CIt>);
static_assert(std::convertible_to<It, CIt>);
static_assert(!std::convertible_to<CIt, It>);

// begin/end/bbegin, dereference/arrow/index, arithmetic and comparisons.
void random_access() {
  Tree tree{10, 20, 30, 40, 50, 60};
  auto begin = tree.begin(), end = tree.end();
  assert(end - begin == 6);
  assert(tree.bbegin() + 1 == begin);
  assert(begin[2] == 30);
  assert(*begin == 10 && *begin.operator->() == 10);
  assert(*(2 + begin) == 30);
  assert(*(end - 1) == 60);
  assert(begin < end && begin <= end && end > begin && end >= begin);
  assert(begin == tree.nth(0) && begin != end);
  assert((end <=> begin) > 0);

  auto it = begin;
  assert(*it++ == 10 && *it == 20);
  assert(*++it == 30);
  assert(*it-- == 30 && *it == 20);
  assert(*--it == 10);
  it += 4;
  assert(*it == 50);
  it -= 2;
  assert(*it == 30);
  assert(it - begin == 2);
  assert(begin - it == -2);

  // Mutable -> const conversion, const traversal, reverse iterators.
  CIt ci = it;
  assert(*ci == 30 && ci == it);
  assert(tree.cbegin() == tree.begin() && tree.cend() == tree.end());
  assert(*tree.rbegin() == 60 && *(tree.rend() - 1) == 10);
  assert(*tree.crbegin() == 60);
  assert(std::vector<long long>(tree.begin(), tree.end()) ==
         std::vector<long long>({10, 20, 30, 40, 50, 60}));

  std::cerr << "iterator random access tests all passed!\n";
}

// set(value): refresh ancestors; iterator retains Leaf identity on insertion.
void mutation() {
  Tree tree{1, 2, 3, 4};
  std::cerr << "A: size=" << tree.size() << '\n';

  auto it = tree.nth(2);
  std::cerr << "B: size=" << tree.size() << '\n';

  it.set(30);
  std::cerr << "C: size=" << tree.size() << '\n';

  assert(tree[2] == 30);
  std::cerr << "D: size=" << tree.size() << '\n';

  tree.insert(0, 0);
  std::cerr << "E: size=" << tree.size() << '\n';

  assert(*it == 30 && it == tree.nth(3));

  std::cerr << "iterator mutation tests all passed!\n";
}

/** @brief Check forward/backward adjacency, never using iterator subtraction. */
void check_linked_walk(Tree& tree, const std::vector<long long>& expected) {
  std::vector<long long> forward, backward;
  auto it = tree.begin();
  for (std::size_t i = 0; i < expected.size(); ++i) {
    assert(it != tree.end());
    forward.push_back(*it);
    ++it;
  }
  assert(it == tree.end());
  it = tree.end();
  for (std::size_t i = 0; i < expected.size(); ++i) {
    --it;
    backward.push_back(*it);
  }
  assert(it == tree.begin());
  assert(forward == expected);
  std::reverse(backward.begin(), backward.end());
  assert(backward == expected);
}

/** @brief Repeated reversals and structural changes preserve bottom links. */
void linked_reverse_and_splice() {
  Tree tree{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  std::vector<long long> expected{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  auto check = [&] { check_linked_walk(tree, expected); };
  check();

  auto reverse_range = [&](std::size_t l, std::size_t r) {
    std::reverse(expected.begin() + l, expected.begin() + r);
    tree.reverse(l, r);
    check();
  };
  reverse_range(1, 10);
  reverse_range(4, 12);
  reverse_range(0, 9);
  reverse_range(2, 8);
  reverse_range(0, expected.size());

  // A retained iterator must revalidate its direction after reverse.
  auto retained = tree.nth(5);
  const auto retained_value = *retained;
  reverse_range(3, 10);
  const auto found = std::find(expected.begin(), expected.end(), retained_value);
  const auto index = static_cast<std::size_t>(found - expected.begin());
  auto next = retained;
  ++next;
  assert(next == tree.nth(index + 1));
  auto prev = retained;
  --prev;
  assert(prev == tree.nth(index - 1));

  auto right = tree.split(5);
  std::vector<long long> left_expected(expected.begin(), expected.begin() + 5);
  std::vector<long long> right_expected(expected.begin() + 5, expected.end());
  check_linked_walk(tree, left_expected);
  check_linked_walk(right, right_expected);
  tree.merge(right);
  check();

  tree.insert(2, 999);
  expected.insert(expected.begin() + 2, 999);
  check();
  tree.erase(2);
  expected.erase(expected.begin() + 2);
  check();
  tree.erase(3, 8);
  expected.erase(expected.begin() + 3, expected.begin() + 8);
  check();
  tree.clear();
  check_linked_walk(tree, {});

  // A second iterator can materialize the very bottom node into which the
  // first iterator entered through port 1. Its cached port must be repaired.
  Tree pending{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  pending.reverse();
  auto crossing = pending.begin();
  assert(*crossing == 16);
  ++crossing; // Cross the boundary, into a bottom with pending reverse.
  auto other = pending.nth(1); // Materialize that bottom from another path.
  assert(*other == 15);
  ++crossing;
  assert(*crossing == 14);
  check_linked_walk(pending, {16, 15, 14, 13, 12, 11, 10, 9,
                              8, 7, 6, 5, 4, 3, 2, 1});
  std::cerr << "linked bottom iteration tests all passed!\\n";
}

int main() {
  assert(It{} == It{});
  mutation();

  random_access();
  linked_reverse_and_splice();

  std::cerr << "All iterator test cases passed!\n";
}
