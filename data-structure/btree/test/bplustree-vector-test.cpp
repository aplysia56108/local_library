#include "data-structure/btree/bplustree-vector.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <iostream>
#include <vector>

template <class Tree>
void check(const Tree& a, const std::vector<long long>& expected) {
  assert(a.size() == expected.size());
  assert(a.empty() == expected.empty());
  assert(std::equal(a.begin(), a.end(), expected.begin(), expected.end()));
  assert(a.end() - a.begin() ==
         static_cast<std::ptrdiff_t>(expected.size()));
}

// 公開API: 構築、Iterator、setter、挿入、削除、resize。
template <std::size_t Capacity>
void sequence_tests() {
  using Tree = Bplustree_Vector<long long, Capacity>;
  static_assert(std::same_as<decltype(*std::declval<typename Tree::iterator>()),
                             const long long&>);
  static_assert(!requires(typename Tree::const_iterator it) { it.set(1LL); });
  Tree a{1, 2, 3};
  assert(a.front() == 1 && a.back() == 3);
  assert(a.get(1) == 2 && a[1] == 2);
  a(1) = 5;
  a(1) += 2;
  a.nth(0).set(4);
  check(a, {4, 7, 3});
  a.push_back(9);
  a.push_front(1);
  a.insert(2, 6);
  check(a, {1, 4, 6, 7, 3, 9});
  auto next = a.erase(a.nth(2));
  assert(next == a.nth(2));
  check(a, {1, 4, 7, 3, 9});
  a.erase(1, 3);
  check(a, {1, 3, 9});
  a.pop_front();
  a.pop_back();
  check(a, {3});
  a.resize(4, 8);
  check(a, {3, 8, 8, 8});
  a.resize(2);
  check(a, {3, 8});
  a.rebuild();
  check(a, {3, 8});
  a.clear();
  check(a, {});
  std::cerr << "vector sequence tests all passed! (capacity="
            << Capacity << ")\n";
}

// 公開API: reverse, split, merge, copy, move。
template <std::size_t Capacity>
void structural_tests() {
  Bplustree_Vector<long long, Capacity> a{1, 2, 3, 4, 5, 6};
  a.reverse(1, 5);
  check(a, {1, 5, 4, 3, 2, 6});
  a.reverse();
  check(a, {6, 2, 3, 4, 5, 1});
  auto right = a.split(3);
  check(a, {6, 2, 3});
  check(right, {4, 5, 1});
  a.merge(right);
  check(a, {6, 2, 3, 4, 5, 1});
  check(right, {});
  auto copy = a;
  check(copy, {6, 2, 3, 4, 5, 1});
  auto moved = std::move(copy);
  check(moved, {6, 2, 3, 4, 5, 1});
  auto all = moved.split(0);
  check(moved, {});
  check(all, {6, 2, 3, 4, 5, 1});
  std::cerr << "vector structural tests all passed! (capacity="
            << Capacity << ")\n";
}

int main() {
  sequence_tests<4>();
  sequence_tests<8>();
  structural_tests<4>();
  structural_tests<8>();
  std::cerr << "All B+tree vector test cases passed!\n";
}
