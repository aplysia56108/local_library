#include "data-structure/btree/bplustree-segtree.hpp"
#include "data-structure/btree/bplustree-lazysegtree.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

inline long long lazy_mapping_calls = 0;
inline constexpr auto CountedAddSumMapping =
    [](const long long& f, const long long& x, std::size_t size) {
      ++lazy_mapping_calls;
      return x + f * static_cast<long long>(size);
    };

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
  // Count-only construction and resize must also use the monoid identity.
  Bplustree_Segtree<MinMonoid, Capacity> filled(3);
  assert(filled.all_prod() == MinMonoid::e());
  filled.resize(5);
  assert(filled.all_prod() == MinMonoid::e());
  std::cerr << "aggregate segtree tests all passed! (capacity=" << Capacity << ")\n";
}

// Boundary predicates may reject the empty product, provided all
// non-empty extensions are monotone (true, then possibly false).
template <std::size_t Capacity>
void boundary_predicate_tests() {
  Bplustree_Segtree<SumMonoid, Capacity> a{1, 2, 3, 4, 5};
  auto pred3 = [](long long x) { return x > 0 && x <= 3; };
  auto pred9 = [](long long x) { return x > 0 && x <= 9; };
  assert(!pred3(SumMonoid::e()));
  assert(a.max_right(0, pred3) == 2);
  assert(a.max_right(2, pred9) == 4);
  assert(a.min_left(5, pred9) == 3);
  assert(a.min_left(0, pred9) == 0);
  assert(a.max_right(a.size(), pred9) == a.size());

  std::cerr << "aggregate boundary predicate tests all passed! (capacity="
            << Capacity << ")\n";
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


// Leaf must not carry aggregate or lazy-tag payloads.
template <std::size_t Capacity>
void leaf_storage_tests() {
  using Traits = Bplustree::AggregateTraits<SumMonoid, Bplustree::NoAction,
                                            false, Capacity>;
  static_assert(sizeof(typename Traits::Leaf) ==
                sizeof(Bplustree::Leaf<Traits>));

  // With an action, applying it to a Leaf updates the value immediately.
  using LazyTraits = Bplustree::AggregateTraits<
      SumMonoid, Action<long long, long long, AddSumMapping, AddComposition>,
      true, Capacity>;
  static_assert(sizeof(typename LazyTraits::Leaf) ==
                sizeof(Bplustree::Leaf<LazyTraits>));
  typename LazyTraits::Leaf leaf(7);
  leaf.push_lazy(5);
  assert(leaf.get() == 12);
  leaf.push_lazy(-2);
  assert(leaf.get() == 10);

  std::cerr << "aggregate leaf storage tests all passed! (capacity="
            << Capacity << ")\n";
}

template <std::size_t Capacity>
void lazy_composition_tests() {
  // Pending composition, including an assignment to zero, must be preserved.
  using Tree = BLazySegtree<long long, long long,
      [](long long a, long long b) { return a + b; },
      []() { return 0LL; },
      ChangeSumMapping, ChangeComposition, true, Capacity>;
  Tree a{1, 2, 3, 4, 5, 6};
  a.apply(1, 5, 10);
  a.apply(2, 4, 0);
  assert(a.all_prod() == 1 + 10 + 0 + 0 + 10 + 6);
  assert(a.prod(0, 6) == 27);
  assert(a.get(2) == 0);
  a.reverse(1, 5);
  assert(a.get(1) == 10);
  assert(a.get(2) == 0);
  a.apply(0, 6, 3);
  assert(a.all_prod() == 18);
  auto right = a.split(3);
  assert(a.all_prod() + right.all_prod() == 18);
  a.merge(right);
  assert(a.all_prod() == 18);

  std::cerr << "aggregate lazy composition tests all passed! (capacity="
            << Capacity << ")\n";
}


/** @brief lazy/reverseの未伝搬経路でもepochキャッシュが値を正しく反映する。 */
template <std::size_t Capacity>
void iterator_epoch_lazy_tests() {
  using Tree = BLazySegtree<long long, long long,
      [](long long a, long long b) { return a + b; },
      []() { return 0LL; },
      AddSumMapping, AddComposition, true, Capacity>;
  std::vector<long long> expected;
  for (long long i = 0; i < 96; ++i) expected.push_back(i * 100);
  Tree tree(expected.begin(), expected.end());
  auto saved = tree.nth(23);
  const auto check = [&]() {
    assert(std::equal(tree.begin(), tree.end(), expected.begin(),
                      expected.end()));
    assert(*saved == expected[23]);
    assert(tree.all_prod() ==
           std::accumulate(expected.begin(), expected.end(), 0LL));
  };
  check();

  tree.apply(8, 85, 7);
  for (std::size_t i = 8; i < 85; ++i) expected[i] += 7;
  assert(*saved == 2307);
  check();

  tree.reverse(5, 77);
  std::reverse(expected.begin() + 5, expected.begin() + 77);
  assert(*saved == 2307);
  assert(std::equal(tree.begin(), tree.end(), expected.begin(),
                    expected.end()));

  tree.all_apply(11);
  for (auto& x : expected) x += 11;
  assert(*saved == 2318);
  assert(std::equal(tree.begin(), tree.end(), expected.begin(),
                    expected.end()));

  auto right = tree.split(45);
  assert(*saved == 2318);
  tree.merge(right);
  assert(std::equal(tree.begin(), tree.end(), expected.begin(),
                    expected.end()));

  std::cerr << "iterator epoch lazy tests passed! (capacity="
            << Capacity << ")\n";
}


/**
 * @brief 位置操作はLazyを伝搬せず、値参照では正しく伝搬する。
 * @details 全体反転は位置epochのみ、区間作用は値epochのみ進む。
 */
template <std::size_t Capacity>
void iterator_split_epoch_lazy_deferral_tests() {
  using Tree = BLazySegtree<
      long long, long long,
      [](long long a, long long b) { return a + b; },
      []() { return 0LL; }, CountedAddSumMapping,
      AddComposition, true, Capacity>;
  using Traits = typename Tree::traits_type;
  using Epoch = Bplustree::IteratorEpoch<Traits>;

  std::vector<long long> values(96);
  std::iota(values.begin(), values.end(), 0LL);
  Tree tree(values.begin(), values.end());
  auto it = tree.nth(12);
  assert(*it == 12);

  const auto position_before_apply = Epoch::position;
  const auto value_before_apply = Epoch::value;
  tree.all_apply(7);
  std::cerr << "DEBUG epochs: pos=" << position_before_apply << "->"
            << Epoch::position << " val=" << value_before_apply << "->"
            << Epoch::value << " sum=" << tree.all_prod()
            << " type=" << typeid(Traits).name()
            << " address=" << &Epoch::value << '\n';
  assert(Epoch::position == position_before_apply);
  assert(Epoch::value != value_before_apply);

  const auto mappings_after_apply = lazy_mapping_calls;
  ++it;
  it += 2;
  --it;
  assert(lazy_mapping_calls == mappings_after_apply);
  assert(tree.end() - it == 96 - 14);
  assert(lazy_mapping_calls == mappings_after_apply);
  assert(*it == 21);
  assert(lazy_mapping_calls > mappings_after_apply);

  const auto value_before_reverse = Epoch::value;
  const auto position_before_reverse = Epoch::position;
  tree.reverse();
  assert(Epoch::position != position_before_reverse);
  assert(Epoch::value == value_before_reverse);
  assert(*it == 21);
  assert(tree.end() - it == 15);

  const auto position_before_second_apply = Epoch::position;
  const auto value_before_second_apply = Epoch::value;
  tree.all_apply(5);
  assert(Epoch::position == position_before_second_apply);
  assert(Epoch::value != value_before_second_apply);
  const auto mappings_before_moving = lazy_mapping_calls;
  ++it;
  assert(lazy_mapping_calls == mappings_before_moving);
  assert(*it == 25);  // 反転後の次要素は元の13番目: 13+7+5。

  auto snapshot = tree;
  std::vector<long long> expected(values.size());
  for (std::size_t i = 0; i < values.size(); ++i)
    expected[i] = values[values.size() - i - 1] + 12;
  assert(std::equal(tree.begin(), tree.end(), expected.begin(), expected.end()));
  assert(std::equal(snapshot.begin(), snapshot.end(),
                    expected.begin(), expected.end()));
  std::cerr << "split epoch lazy deferral tests passed! (capacity="
            << Capacity << ")\n";
}

int main() {
  leaf_storage_tests<4>();
  leaf_storage_tests<8>();
  lazy_composition_tests<4>();
  lazy_composition_tests<8>();
  boundary_predicate_tests<4>();
  boundary_predicate_tests<8>();
  segtree_tests<4>();
  segtree_tests<8>();
  noncommutative_tests<4>();
  noncommutative_tests<8>();
  lazy_tests<4>();
  lazy_tests<8>();
  iterator_epoch_lazy_tests<4>();
  iterator_epoch_lazy_tests<8>();
  iterator_epoch_lazy_tests<16>();
  iterator_split_epoch_lazy_deferral_tests<4>();
  iterator_split_epoch_lazy_deferral_tests<8>();
  iterator_split_epoch_lazy_deferral_tests<16>();
  std::cerr << "All B+tree aggregate test cases passed!\n";
}
