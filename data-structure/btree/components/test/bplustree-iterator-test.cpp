#include "data-structure/btree/components/bplustree-base.hpp"

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

  std::cerr << "iterator random access tests all passed!\n";}

// set(value): refresh ancestors; iterator retains Leaf identity on insertion.
void mutation() {
  Tree tree{1, 2, 3, 4};
  auto it = tree.nth(2);
  it.set(30);
  assert(tree[2] == 30);
  tree.insert(0, 0);
  assert(*it == 30 && it == tree.nth(3));

  std::cerr << "iterator mutation tests all passed!\n";}

int main() {
  assert(It{} == It{});
  random_access();
  mutation();

  std::cerr << "All iterator test cases passed!\n";}
