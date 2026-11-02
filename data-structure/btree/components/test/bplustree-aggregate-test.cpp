#include "data-structure/btree/bplustree-segtree.hpp"
#include "data-structure/btree/bplustree-lazysegtree.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

struct ConcatMonoid {
  using value_type = std::string;
  static value_type e() { return ""; }
  static value_type op(const value_type& a, const value_type& b) { return a + b; }
};

template <std::size_t Capacity>
void segtree_tests() {
  // Empty tree, build, set, insert, erase, split, merge, reverse and Setter.
  Bplustree_Segtree<SumMonoid, Capacity> a{1, 2, 3, 4, 5};
  assert(a.all_prod() == 15);
  assert(a.prod(1, 4) == 9);
  assert(a.reverse_prod(1, 4) == 9);
  a(1) = 7;
  assert(a.all_prod() == 20);
  a(1).prod(3);
  assert(a.get(1) == 10);
  a.insert(2, 6);
  assert(a.prod(0, a.size()) == 29);
  a.erase(0);
  assert(a.all_prod() == 28);
  a.reverse(1, 4);
  assert(a.all_prod() == 28);
  auto right = a.split(2);
  assert(a.all_prod() + right.all_prod() == 28);
  a.merge(right);
  assert(a.all_prod() == 28);
  assert(right.all_prod() == 0);
  assert(a.max_right(0, [](long long x) { return x <= 17; }) == 3);
  assert(a.min_left(a.size(), [](long long x) { return x <= 9; }) == 4);

  // Sentinels must contribute the monoid identity rather than T{}.
  Bplustree_Segtree<MinMonoid, Capacity> minimum{8, 4, 9};
  assert(minimum.all_prod() == 4);
  assert(minimum.prod(0, 1) == 8);
  Bplustree_Segtree<MinMonoid, Capacity> empty;
  assert(empty.all_prod() == MinMonoid::e());
  std::cerr << "aggregate segtree tests all passed! (capacity=" << Capacity << ")\n";
}

template <std::size_t Capacity>
void noncommutative_tests() {
  // Both directional aggregates must remain correct after reversals.
  Bplustree_Segtree<ConcatMonoid, Capacity> a{"a", "b", "c", "d"};
  assert(a.prod(0, 4) == "abcd");
  assert(a.reverse_prod(0, 4) == "dcba");
  a.reverse(1, 4);
  assert(a.prod(0, 4) == "adcb");
  assert(a.reverse_prod(0, 4) == "bcda");
  a.reverse();
  assert(a.all_prod() == "bcda");
  assert(a.prod(1, 3) == "cd");
  assert(a.reverse_prod(1, 3) == "dc");
  std::cerr << "aggregate noncommutative tests all passed! (capacity=" << Capacity << ")\n";
}

template <std::size_t Capacity>
void lazy_tests() {
  // Pending updates must survive reverse, split, merge, and point mutation.
  using Tree = BLazySegtree<long long, long long,
      [](long long a, long long b) { return a + b; },
      []() { return 0LL; },
      AddSumMapping, AddComposition, true, Capacity>;
  Tree a{1, 2, 3, 4, 5};
  a.apply(1, 4, 10);
  assert(a.all_prod() == 45);
  assert(a.prod(1, 3) == 25);
  a.reverse(0, 5);
  assert(a.prod(0, 2) == 19);
  a(0).apply(2);
  assert(a.get(0) == 7);
  a.all_apply(3);
  assert(a.all_prod() == 62);
  auto right = a.split(2);
  assert(a.all_prod() + right.all_prod() == 62);
  a.merge(right);
  assert(a.all_prod() == 62);
  a.set(1, 100);
  assert(a.all_prod() == 62 - 17 + 100);
  std::cerr << "aggregate lazy tests all passed! (capacity=" << Capacity << ")\n";
}

int main() {
  segtree_tests<4>();
  segtree_tests<8>();
  noncommutative_tests<4>();
  noncommutative_tests<8>();
  lazy_tests<4>();
  lazy_tests<8>();
  std::cerr << "All B+tree aggregate test cases passed!\n";
}
