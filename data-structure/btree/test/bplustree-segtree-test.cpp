#include "data-structure/btree/bplustree-segtree.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

struct ConcatMonoid {
  using value_type = std::string;
  static value_type e() { return {}; }
  static value_type op(const value_type& a, const value_type& b) {
    return a + b;
  }
};

// 公開API: prod, reverse_prod, all_prod, all_reverse_prod, Setter, 境界探索。
template <std::size_t Capacity>
void aggregate_tests() {
  Bplustree_Segtree<SumMonoid, Capacity> a{1, 2, 3, 4, 5};
  assert(a.size() == 5);
  assert(a.all_prod() == 15);
  assert(a.prod(1, 4) == 9);
  assert(a.prod(2, 2) == 0);
  assert(a.reverse_prod(1, 4) == 9);
  a(1) = 7;
  a(1).prod(3);
  assert(a.get(1) == 10);
  assert(a.all_prod() == 23);
  a.push_back(6);
  assert(a.all_prod() == 29);
  a.erase(0);
  assert(a.all_prod() == 28);
  a.reverse();
  assert(a.all_prod() == 28);
  assert(a.all_reverse_prod() == 28);
  assert(a.max_right(0, [](long long x) { return x > 0 && x <= 12; }) == 2);
  assert(a.min_left(a.size(), [](long long x) { return x > 0 && x <= 13; }) == 3);
  auto right = a.split(2);
  assert(a.all_prod() + right.all_prod() == 28);
  a.merge(right);
  assert(a.all_prod() == 28);
  assert(right.empty());

  Bplustree_Segtree<MinMonoid, Capacity> minimum(4);
  assert(minimum.all_prod() == MinMonoid::e());
  minimum.set(2, 5);
  assert(minimum.all_prod() == 5);
  std::cerr << "segtree aggregate tests all passed! (capacity="
            << Capacity << ")\n";
}

// 非可換演算の反転・逆方向積・単位元。
template <std::size_t Capacity>
void order_tests() {
  Bplustree_Segtree<ConcatMonoid, Capacity> a{"a", "b", "c", "d"};
  assert(a.all_prod() == "abcd");
  assert(a.all_reverse_prod() == "dcba");
  assert(a.prod(1, 3) == "bc");
  assert(a.reverse_prod(1, 3) == "cb");
  a.reverse(1, 4);
  assert(a.all_prod() == "adcb");
  assert(a.all_reverse_prod() == "bcda");
  a.reverse();
  assert(a.all_prod() == "bcda");
  a(1).prod_left("!");
  assert(a.prod(0, 3) == "b!cd");
  std::cerr << "segtree order tests all passed! (capacity="
            << Capacity << ")\n";
}

int main() {
  aggregate_tests<4>();
  aggregate_tests<8>();
  order_tests<4>();
  order_tests<8>();
  std::cerr << "All B+tree segtree test cases passed!\n";
}
