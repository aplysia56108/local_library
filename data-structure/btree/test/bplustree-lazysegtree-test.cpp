#include "data-structure/btree/bplustree-lazysegtree.hpp"

#include <cassert>
#include <iostream>
#include <vector>

template <std::size_t Capacity>
using SumLazy = BLazySegtree<
    long long, long long,
    [](long long a, long long b) { return a + b; },
    []() { return 0LL; },
    AddSumMapping, AddComposition, true, Capacity>;

// 公開API: 区間/点/全体作用、Setter、逆順、split/merge。
template <std::size_t Capacity>
void lazy_sequence_tests() {
  SumLazy<Capacity> a{1, 2, 3, 4, 5};
  a.apply(1, 4, 10);
  assert(a.all_prod() == 45);
  assert(a.prod(1, 3) == 25);
  assert(a.get(2) == 13);
  a.reverse();
  assert(a.get(0) == 5);
  assert(a.get(1) == 14);
  a.all_apply(2);
  assert(a.all_prod() == 55);
  a(0).apply(3);
  assert(a.get(0) == 10);
  assert(a.all_prod() == 58);
  auto right = a.split(2);
  assert(a.all_prod() == 26);
  assert(right.all_prod() == 32);
  a.merge(right);
  assert(a.all_prod() == 58);
  a.set(1, 1);
  assert(a.all_prod() == 43);
  a.apply(2, 2, 100);
  assert(a.all_prod() == 43);
  std::cerr << "lazy sequence tests all passed! (capacity="
            << Capacity << ")\n";
}

// 遅延タグの合成順と、値0への代入を確認する。
template <std::size_t Capacity>
void lazy_composition_tests() {
  using Tree = BLazySegtree<
      long long, long long,
      [](long long a, long long b) { return a + b; },
      []() { return 0LL; },
      ChangeSumMapping, ChangeComposition, true, Capacity>;
  Tree a{1, 2, 3, 4, 5, 6};
  a.apply(1, 5, 10);
  a.apply(2, 4, 0);
  assert(a.all_prod() == 27);
  assert(a.get(3) == 0);
  a.reverse(1, 5);
  assert(a.all_prod() == 27);
  a.all_apply(3);
  assert(a.all_prod() == 18);
  assert(a.get(0) == 3 && a.get(5) == 3);
  std::cerr << "lazy composition tests all passed! (capacity="
            << Capacity << ")\n";
}

int main() {
  lazy_sequence_tests<4>();
  lazy_sequence_tests<8>();
  lazy_composition_tests<4>();
  lazy_composition_tests<8>();
  std::cerr << "All B+tree lazysegtree test cases passed!\n";
}
