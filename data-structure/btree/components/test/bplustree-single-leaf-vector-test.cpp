#include "data-structure/btree/components/bplustree-base.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <random>
#include <vector>

// Specification tests for the one-value-per-Leaf positional tree.
// Deliberately exclude reverse, lazy actions, aggregates, split and merge.
// These tests describe the intended API while the refactor is in progress.
template <std::size_t Capacity>
struct CoreVectorTraits {
  using T = long long;
  static constexpr std::size_t kNodeCapacity = Capacity;
  static constexpr bool IsReversible = false;
  static constexpr bool IsMergable = false;
  using Node = Bplustree::Node<CoreVectorTraits>;
  using Leaf = Bplustree::Leaf<CoreVectorTraits>;
  using Internal = Bplustree::Internal<CoreVectorTraits>;
};

template <std::size_t Capacity>
using CoreVector = Bplustree::Base<CoreVectorTraits<Capacity>>;

template <class Tree>
void check(const Tree& tree, const std::vector<long long>& expected) {
  assert(tree.size() == expected.size());
  assert(tree.empty() == expected.empty());
  assert((tree.begin() == tree.end()) == expected.empty());

  std::vector<long long> actual;
  for (const auto& value : tree) actual.push_back(value);
  assert(actual == expected);

  assert(tree.end() - tree.begin() ==
         static_cast<std::ptrdiff_t>(expected.size()));
  assert(tree.bbegin() + 1 == tree.begin());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    assert(*tree.nth(i) == expected[i]);
    assert(tree[i] == expected[i]);
    assert(*(tree.begin() + static_cast<std::ptrdiff_t>(i)) == expected[i]);
  }
  assert(tree.nth(expected.size()) == tree.end());
}

template <std::size_t Capacity>
void basic_operations() {
  CoreVector<Capacity> tree;
  std::vector<long long> expected;
  check(tree, expected);

  tree.push_back(10);
  tree.push_front(20);
  tree.insert(1, 30);
  expected = {20, 30, 10};
  check(tree, expected);

  tree.set(1, 35);
  expected[1] = 35;
  check(tree, expected);

  tree.erase(0);
  expected.erase(expected.begin());
  check(tree, expected);

  tree.pop_back();
  expected.pop_back();
  check(tree, expected);

  tree.pop_front();
  expected.clear();
  check(tree, expected);

  for (long long i = 0; i < 70; ++i) {
    tree.push_back(i);
    expected.push_back(i);
  }
  check(tree, expected);
  tree.clear();
  check(tree, {});
  tree.push_back(123);
  check(tree, {123});
}

// A Leaf is a separately allocated element node. Inserting or erasing
// *other* elements must preserve iterators to surviving elements.
template <std::size_t Capacity>
void stable_element_iterators() {
  CoreVector<Capacity> tree;
  for (long long i = 0; i < 40; ++i) tree.push_back(i + 100);

  auto first = tree.nth(5);
  auto middle = tree.nth(20);
  auto last = tree.nth(35);
  const auto* first_address = std::addressof(*first);
  const auto* middle_address = std::addressof(*middle);
  const auto* last_address = std::addressof(*last);
  auto before_begin = tree.bbegin();
  auto past_end = tree.end();

  tree.insert(0, -1);
  tree.insert(13, -2);
  tree.insert(tree.size(), -3);
  assert(*first == 105 && *middle == 120 && *last == 135);
  assert(std::addressof(*first) == first_address);
  assert(std::addressof(*middle) == middle_address);
  assert(std::addressof(*last) == last_address);
  assert(before_begin == tree.bbegin());
  assert(past_end == tree.end());

  // Remove inserted elements, never the three tracked elements.
  tree.erase(tree.size() - 1);
  tree.erase(13);
  tree.erase(0);
  assert(*first == 105 && *middle == 120 && *last == 135);
  assert(std::addressof(*first) == first_address);
  assert(std::addressof(*middle) == middle_address);
  assert(std::addressof(*last) == last_address);
  assert(first + 15 == middle);
  assert(middle + 15 == last);
  assert(before_begin == tree.bbegin());
  assert(past_end == tree.end());

  std::vector<long long> expected;
  for (long long i = 0; i < 40; ++i) expected.push_back(i + 100);
  check(tree, expected);
}

template <std::size_t Capacity>
void randomized_against_vector() {
  CoreVector<Capacity> tree;
  std::vector<long long> expected;
  std::mt19937_64 rng(20261024 + Capacity);

  for (std::size_t step = 0; step < 2500; ++step) {
    const std::size_t index = rng() % (expected.size() + 1);
    const long long value = static_cast<long long>(rng() % 100000);
    const auto operation = rng() % 5;
    if ((operation == 0 || operation == 1) && expected.size() < 120) {
      auto it = tree.insert(index, value);
      expected.insert(expected.begin() + static_cast<std::ptrdiff_t>(index),
                      value);
      assert(*it == value);
    } else if (operation == 2 && index < expected.size()) {
      auto it = tree.erase(index);
      expected.erase(expected.begin() + static_cast<std::ptrdiff_t>(index));
      assert(it == tree.nth(index));
    } else if (operation == 3 && index < expected.size()) {
      tree.set(index, value);
      expected[index] = value;
    } else if (operation == 4 && step % 13 == 0) {
      tree.clear();
      expected.clear();
    }
    check(tree, expected);
  }
}

int main() {
  basic_operations<3>();
  basic_operations<4>();
  basic_operations<8>();
  stable_element_iterators<3>();
  stable_element_iterators<4>();
  stable_element_iterators<8>();
  randomized_against_vector<3>();
  randomized_against_vector<4>();
  randomized_against_vector<8>();
}
