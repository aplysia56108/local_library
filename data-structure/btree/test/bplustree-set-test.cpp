#include "data-structure/btree/bplustree-set.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <iostream>
#include <iterator>
#include <set>
#include <type_traits>
#include <vector>

template <class Tree>
void check(const Tree& tree, const std::vector<long long>& expected) {
  assert(tree.size() == expected.size());
  assert(std::equal(tree.begin(), tree.end(), expected.begin(), expected.end()));
  assert(tree.end() - tree.begin() ==
         static_cast<std::ptrdiff_t>(expected.size()));
}

// 公開API: 順序・重複排除・上下界・検索・削除・コピー。
template <std::size_t Capacity>
void set_tests() {
  Bplustree_Set<long long, SetType::Set, std::less<long long>, Capacity> s;
  static_assert(std::same_as<decltype(*s.begin()), const long long&>);
  assert(s.empty());
  assert(s.insert(4).second);
  assert(s.insert(2).second);
  assert(s.insert(8).second);
  assert(!s.insert(4).second);
  check(s, {2, 4, 8});
  assert(*s.lower_bound(3) == 4);
  assert(*s.upper_bound(4) == 8);
  assert(s.lower_bound(9) == s.end());
  assert(s.min_ge(9) == s.end());
  assert(s.max_lt(2) == s.bbegin());
  assert(*s.max_le(4) == 4);
  assert(s.contains(2) && !s.contains(3));
  assert(s.find(8) == s.nth(2));
  assert(s.count(4) == 1 && s.count(3) == 0);
  auto [first, last] = s.equal_range(4);
  assert(last - first == 1);
  assert(*s.nth(1) == 4);

  auto next = s.erase(s.find(4));
  assert(next == s.lower_bound(4));
  check(s, {2, 8});
  assert(s.erase(4) == 0);
  assert(s.erase(2) == 1);
  check(s, {8});

  std::vector<long long> input{5, 1, 3, 5, 2};
  s.assign(input.begin(), input.end());
  check(s, {1, 2, 3, 5});
  auto copy = s;
  assert(copy.size() == s.size());
  copy.insert(9);
  assert(s.size() == 4);
  swap(copy, s);
  assert(s.contains(9));
  s.rebuild();
  check(s, {1, 2, 3, 5, 9});
  s.clear();
  assert(s.empty());
  std::cerr << "set tests all passed! (capacity=" << Capacity << ")\n";
}

// 公開API: multisetの重複、single_erase、逆順比較器とfactory。
template <std::size_t Capacity>
void multiset_tests() {
  Bplustree_Set<long long, SetType::MultiSet, std::less<long long>, Capacity> s;
  const std::vector<long long> values{4, 1, 4, 2, 4, 2};
  s.assign(values.begin(), values.end());
  check(s, {1, 2, 2, 4, 4, 4});
  assert(s.count(4) == 3);
  assert(s.upper_bound(4) - s.lower_bound(4) == 3);
  assert(s.erase(4, true) == 1);
  assert(s.count(4) == 2);
  assert(s.erase(2) == 2);
  check(s, {1, 4, 4});

  auto greater = [](const long long& a, const long long& b) {
    return a > b;
  };
  auto descending = make_bset<Capacity>(greater);
  descending.insert(1);
  descending.insert(5);
  descending.insert(3);
  assert(std::vector<long long>(descending.begin(), descending.end()) ==
         std::vector<long long>({5, 3, 1}));
  assert(*descending.lower_bound(4) == 3);
  assert(*descending.upper_bound(3) == 1);

  auto multi = make_bmultiset<Capacity>(values.begin(), values.end(),
                                       std::less<long long>{});
  assert(multi.count(4) == 3);
  std::cerr << "multiset tests all passed! (capacity=" << Capacity << ")\n";
}

int main() {
  set_tests<4>();
  set_tests<8>();
  multiset_tests<4>();
  multiset_tests<8>();
  std::cerr << "All B+tree set test cases passed!\n";
}
