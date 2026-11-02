#include "data-structure/btree/components/bplustree-base.hpp"

#include <algorithm>
#include <iostream>
#include <source_location>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

template <std::size_t Capacity>
using Tree = Bplustree::Base<Bplustree::Traits<long long, Capacity>>;

template <class T>
void check(const T& tree, std::initializer_list<long long> expected,
           std::source_location where = std::source_location::current()) {
  assert(tree.size() == expected.size());
  assert(tree.empty() == (expected.size() == 0));

  const std::vector<long long> actual(tree.begin(), tree.end());
  if (!std::equal(actual.begin(), actual.end(), expected.begin(), expected.end())) {
    std::cerr << "Mismatch at " << where.file_name() << ":"
              << where.line() << " (" << where.function_name() << ")\\n";
    std::cerr << "  expected:";
    for (auto x : expected) std::cerr << ' ' << x;
    std::cerr << "\\n  actual:  ";
    for (auto x : actual) std::cerr << ' ' << x;
    std::cerr << '\\n';
    assert(false && "B+tree contents differ from expected");
  }

  assert(tree.end() - tree.begin() == static_cast<std::ptrdiff_t>(expected.size()));
  assert(tree.bbegin() + 1 == tree.begin());
  assert(tree.nth(expected.size()) == tree.end());
}

// Constructors, assign, copy/move, swap, clear, resize, rebuild.
template <std::size_t Capacity>
void construction() {
  Tree<Capacity> a;
  check(a, {});
  Tree<Capacity> b(3, 7);
  check(b, {7, 7, 7});
  Tree<Capacity> c{1, 2, 3, 4};
  check(c, {1, 2, 3, 4});
  std::vector<long long> values{8, 9, 10};
  Tree<Capacity> d(values.begin(), values.end());
  check(d, {8, 9, 10});
  d.assign(2, 5);
  check(d, {5, 5});
  d.assign(values.begin(), values.end());
  check(d, {8, 9, 10});
  Tree<Capacity> copy(d);
  check(copy, {8, 9, 10});
  copy = b;
  check(copy, {7, 7, 7});
  Tree<Capacity> moved(std::move(copy));
  check(moved, {7, 7, 7});
  copy = std::move(c);
  check(copy, {1, 2, 3, 4});
  swap(a, b);
  check(a, {7, 7, 7});
  check(b, {});
  a.resize(5, 6);
  check(a, {7, 7, 7, 6, 6});
  a.resize(2);
  check(a, {7, 7});
  a.rebuild();
  check(a, {7, 7});
  a.clear();
  check(a, {});

  std::cerr << "base construction tests all passed! (capacity=" << Capacity << ")\n";
}

// Accessors, element updates, position/iterator overloads, search.
template <std::size_t Capacity>
void access_and_edit() {
  Tree<Capacity> a{10, 20, 30};
  assert(a.front() == 10 && a.back() == 30);
  assert(a[1] == 20 && a.get(1) == 20);
  assert(*a.nth(2) == 30);
  assert(a.find(20) == a.nth(1));
  assert(a.find(99) == a.end());
  assert(a.count(20) == 1 && a.count(99) == 0);
  assert(a.contains(10) && !a.contains(99));
  a.set(1, 21);
  a(0) = 11;
  check(a, {11, 21, 30});
  assert(*a.insert(1, 15) == 15);
  assert(*a.insert(a.end(), 40) == 40);
  a.push_front(5);
  a.push_back(45);
  check(a, {5, 11, 15, 21, 30, 40, 45});
  auto after_erase = a.erase(a.nth(2));
  assert(after_erase == a.nth(2));
  assert(*after_erase == 21);
  auto after_front_erase = a.erase(0);
  assert(after_front_erase == a.begin());
  a.pop_front();
  a.pop_back();
  check(a, {21, 30, 40});
  auto after_range_erase = a.erase(1, 3);
  assert(after_range_erase == a.nth(1));
  check(a, {21});
  auto after_empty_erase = a.erase(a.nth(1), a.nth(1));
  assert(after_empty_erase == a.nth(1));
  auto after_all_erase = a.erase(a.begin(), a.end());
  assert(after_all_erase == a.end());
  check(a, {});

  std::cerr << "base access and edit tests all passed! (capacity=" << Capacity << ")\n";
}

// split(index/iterator), merge, reverse(all/range) and boundary positions.
template <std::size_t Capacity>
void structural_operations() {
  Tree<Capacity> a{1, 2, 3, 4, 5, 6};
  auto right = a.split(2);
  check(a, {1, 2});
  check(right, {3, 4, 5, 6});
  a.merge(right);
  check(a, {1, 2, 3, 4, 5, 6});
  check(right, {});
  a.reverse(1, 5);
  check(a, {1, 5, 4, 3, 2, 6});
  a.reverse(a.begin() + 1, a.begin() + 5);
  check(a, {1, 2, 3, 4, 5, 6});
  a.reverse();
  check(a, {6, 5, 4, 3, 2, 1});
  a.reverse();
  auto empty = a.split(a.end());
  check(empty, {});
  check(a, {1, 2, 3, 4, 5, 6});
  auto all = a.split(a.begin());
  check(a, {});
  check(all, {1, 2, 3, 4, 5, 6});
  a.merge(all);
  check(a, {1, 2, 3, 4, 5, 6});
  Tree<Capacity> first;
  first.merge(a);
  check(first, {1, 2, 3, 4, 5, 6});
  check(a, {});

  std::cerr << "base structural operations tests all passed! (capacity=" << Capacity << ")\n";
}

int main() {
  construction<4>();
  construction<8>();
  access_and_edit<4>();
  access_and_edit<8>();
  structural_operations<4>();
  structural_operations<8>();

  std::cerr << "All B+tree base test cases passed!\n";
}
