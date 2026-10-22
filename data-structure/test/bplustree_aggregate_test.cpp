#include "data-structure/btree/bplustree-segtree.hpp"
#include "data-structure/btree/bplustree-lazysegtree.hpp"
#include "data-structure/btree/bplustree-set.hpp"

#include <cassert>
#include <numeric>
#include <random>
#include <string>
#include <set>
#include <sstream>

struct AffineAction {
  using action_type = std::pair<long long, long long>;
  static long long mapping(action_type f, long long x, std::size_t n) {
    return f.first * x + f.second * n;
  }
  static action_type composition(action_type f, action_type g) {
    return {f.first * g.first, f.first * g.second + f.second};
  }
};
struct StringMonoid {
  using value_type = std::string;
  static std::string e() { return {}; }
  static std::string op(const std::string& a, const std::string& b) { return a + b; }
};
struct LetterAction {
  using action_type = int;
  static std::string mapping(int f, std::string x, std::size_t) {
    for (auto& ch : x) ch = 'a' + (ch - 'a' + f) % 26;
    return x;
  }
  static int composition(int f, int g) { return (f + g) % 26; }
};
template <class T> concept PositionalWrite = requires(T& t) { t.set(0, 3); };
template <class T> concept CanReverse = requires(T& t) { t.reverse(); };
static_assert(!PositionalWrite<BSet<int>>);
static_assert(!CanReverse<BSet<int>>);
static_assert(std::random_access_iterator<SumSegtree::iterator>);
static_assert(std::same_as<std::iter_reference_t<SumSegtree::iterator>, const long long&>);

template <std::size_t C, bool R>
void numeric_random() {
  using Lazy = Bplustree_LazySegtree<SumMonoid, AffineAction, R, C>;
  using Seg = Bplustree_Segtree<SumMonoid, C>;
  std::mt19937 rng(81 + C + R);
  std::vector<long long> values(150, 1);
  Lazy lazy(values.begin(), values.end());
  Seg seg(values.begin(), values.end());
  for (int step = 0; step < 2500; ++step) {
    std::size_t l = rng() % (values.size() + 1);
    std::size_t r = l + rng() % (values.size() - l + 1);
    long long v = rng() % 8;
    switch (rng() % 11) {
      case 0:
        lazy.insert(l, v); seg.insert(l, v); values.insert(values.begin() + l, v); break;
      case 1:
        lazy.erase(l, r); seg.erase(l, r); values.erase(values.begin() + l, values.begin() + r); break;
      case 2:
        lazy.reverse(l, r); seg.reverse(l, r); std::reverse(values.begin() + l, values.begin() + r); break;
      case 3:
        if (l < values.size()) { lazy(l) = v; seg(l) = v; values[l] = v; } break;
      case 4: {
        AffineAction::action_type f{rng() % 2, v};
        lazy.apply(l, r, f);
        for (auto i = l; i < r; ++i) { values[i] = AffineAction::mapping(f, values[i], 1); seg.set(i, values[i]); }
        break;
      }
      case 5: {
        AffineAction::action_type f{0, v};
        lazy.all_apply(f); lazy.all_apply({1, 2});
        for (std::size_t i = 0; i < values.size(); ++i) { values[i] = v + 2; seg.set(i, values[i]); }
        break;
      }
      case 6: {
        auto right_lazy = lazy.split(l); auto right_seg = seg.split(l);
        assert(right_lazy.size() == values.size() - l);
        right_lazy.all_apply({1, v});
        for (std::size_t i = l; i < values.size(); ++i) { values[i] += v; right_seg.set(i - l, values[i]); }
        lazy.merge(right_lazy); seg.merge(right_seg);
        assert(right_lazy.empty() && right_seg.empty()); break;
      }
      case 7:
        if (l < values.size()) { lazy(l).apply(AffineAction::action_type{0, v}); seg(l).apply(v - values[l]); values[l] = v; } break;
      case 8:
        lazy.reverse(); seg.reverse(); std::reverse(values.begin(), values.end()); break;
      case 9:
        if (l < values.size()) { lazy(l).prod(v); seg(l).prod_right(v); values[l] += v; } break;
      case 10:
        if (step % 7 == 0) { lazy.resize(100, v); seg.resize(100, v); values.resize(100, v); } break;
    }
    auto sum = [&](std::size_t a, std::size_t b) { return std::accumulate(values.begin() + a, values.begin() + b, 0LL); };
    assert(lazy.all_prod() == sum(0, values.size()));
    assert(seg.all_prod() == lazy.all_prod());
    l = rng() % (values.size() + 1); r = l + rng() % (values.size() - l + 1);
    assert(lazy.prod(l, r) == sum(l, r)); assert(seg.prod(l, r) == sum(l, r));
    auto limit = rng() % 60;
    std::size_t expected = l; long long total = 0;
    while (expected < values.size() && total + values[expected] <= limit) total += values[expected++];
    auto predicate = [&](long long x) { return x <= limit; };
    assert(lazy.max_right(l, predicate) == expected); assert(seg.right_bound(l, predicate) == expected);
    expected = r; total = 0;
    while (expected > 0 && total + values[expected - 1] <= limit) total += values[--expected];
    assert(lazy.min_left(r, predicate) == expected); assert(seg.left_bound(r, predicate) == expected);
    // Leave tags pending across several structural operations before reading all elements.
    if (step % 17 == 0) {
      assert(std::vector<long long>(lazy.begin(), lazy.end()) == values);
      Lazy copy(lazy); Lazy moved(std::move(copy)); copy = moved; lazy = std::move(copy);
      lazy.rebuild(); seg.rebuild();
      assert(lazy.all_prod() == sum(0, values.size()));
    }
  }
  assert(std::vector<long long>(lazy.begin(), lazy.end()) == values);
  Lazy empty; empty.all_apply({1, 4}); assert(empty.all_prod() == 0);
  MinSegtree minimum(5); assert(minimum.all_prod() == MinMonoid::e());
  minimum.assign(3); minimum.resize(8); assert(minimum.all_prod() == MinMonoid::e());
}

void noncommutative() {
  using Lazy = Bplustree_LazySegtree<StringMonoid, LetterAction, false, 3>;
  using Seg = Bplustree_Segtree<StringMonoid, 3>;
  std::vector<std::string> values(80);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = std::string(i % 3 + 1, 'a' + i % 26);
  Lazy lazy(values.begin(), values.end()); Seg seg(values.begin(), values.end());
  std::mt19937 rng(96);
  for (int step = 0; step < 500; ++step) {
    std::size_t l = rng() % (values.size() + 1), r = l + rng() % (values.size() - l + 1);
    if (step % 3 == 0) {
      lazy.all_apply(2);
      for (std::size_t i = 0; i < values.size(); ++i) { values[i] = LetterAction::mapping(2, values[i], 1); seg.set(i, values[i]); }
    } else {
      lazy.apply(l, r, 3);
      for (std::size_t i = l; i < r; ++i) { values[i] = LetterAction::mapping(3, values[i], 1); seg.set(i, values[i]); }
    }
    lazy.reverse(l, r); seg.reverse(l, r); std::reverse(values.begin() + l, values.begin() + r);
    if (step % 4 == 0) { lazy.reverse(); seg.reverse(); std::reverse(values.begin(), values.end()); }
    std::string expected, backward;
    for (auto i = l; i < r; ++i) expected += values[i];
    for (auto i = r; i > l; --i) backward += values[i - 1];
    assert(lazy.prod(l, r) == expected && seg.prod(l, r) == expected);
    assert(lazy.reverse_prod(l, r) == backward && seg.reverse_prod(l, r) == backward);
    // Compare actual concatenated content, so reversing op argument order fails.
    std::string prefix, suffix; auto right = l, left = r;
    while (right < values.size() && prefix.size() + values[right].size() <= 20) prefix += values[right++];
    while (left > 0 && suffix.size() + values[left - 1].size() <= 20) suffix = values[--left] + suffix;
    auto pf = [&](const std::string& s) { return s.size() <= prefix.size() && prefix.compare(0, s.size(), s) == 0; };
    auto sf = [&](const std::string& s) { return s.size() <= suffix.size() && suffix.compare(suffix.size() - s.size(), s.size(), s) == 0; };
    assert(lazy.max_right(l, pf) == right && seg.max_right(l, pf) == right);
    assert(lazy.min_left(r, sf) == left && seg.min_left(r, sf) == left);
    assert(std::vector<std::string>(lazy.begin(), lazy.end()) == values);
  }
}

struct StatefulCompare {
  bool descending;
  bool operator()(int a, int b) const { return descending ? a > b : a < b; }
};

void compatibility() {
  std::istringstream input("7 2 7 1 2 9");
  using Set = Bplustree_Set<int, SetType::Set, StatefulCompare, 3>;
  Set original(std::istream_iterator<int>(input), std::istream_iterator<int>(), StatefulCompare{true});
  const std::vector<int> descending{9, 7, 2, 1};
  assert(std::vector<int>(original.begin(), original.end()) == descending);
  Set copy(original), moved(std::move(copy));
  Set assigned(StatefulCompare{false}); assigned = original;
  assert(assigned.key_comp().descending && assigned.contains(7));
  assigned = std::move(moved);
  Set ascending(StatefulCompare{false}); ascending.insert(3); ascending.insert(5);
  swap(ascending, assigned);
  assert(ascending.key_comp().descending && !assigned.key_comp().descending);
  assert(std::vector<int>(ascending.begin(), ascending.end()) == descending);
  assert(std::vector<int>(assigned.begin(), assigned.end()) == std::vector<int>({3, 5}));
  // Compare-equivalence need not be operator==.
  auto abs_set = make_bset<3>([](int a, int b) { return std::abs(a) < std::abs(b); });
  abs_set.insert(5); assert(!abs_set.insert(-5).second && abs_set.count(-5) == 1);
  BMultiSet<int> multi; multi.insert(2); multi.insert(2);
  assert(multi.erase(2, true) == 1 && multi.count(2) == 1);
  multi.insert(multi.begin(), multi.end()); assert(multi.count(2) == 2);

  AddSumLazySegtree lazy{1, 2, 3}; lazy.apply(0, 2, 10); lazy(2).apply(4); lazy.all_apply(1);
  assert(lazy.all_prod() == 33);
  ChangeSumLazySegtree change{1, 2, 3}; change.all_apply(4); change.reverse();
  change(0) += 1; assert(change.all_prod() == 13);
  AffineSumLazySegtree<long long, 3> affine{1, 2, 3};
  affine.all_apply({2, 1}); affine.all_apply({3, 2}); affine.reverse();
  assert(affine.all_prod() == 51 && affine[0] == 23);
  using WithoutE = BLazySegtreeWithoutE<long long, long long,
      [](long long a, long long b) { return std::max(a, b); }, AddMapping, AddComposition, true, 3>;
  WithoutE no_e{1, 2, 3}; no_e.all_apply(4); no_e.reverse();
  assert(no_e.prod(0, 3).val() == 7 && no_e[0].val() == 7);
}

int main() {
  numeric_random<3, false>(); numeric_random<4, true>(); numeric_random<15, false>(); numeric_random<64, true>();
  noncommutative();
  compatibility();
  std::multiset<int, std::greater<int>> expected;
  Bplustree_Set<int, SetType::MultiSet, std::greater<int>, 3> tree;
  std::mt19937 rng(11);
  for (int i = 0; i < 2000; ++i) {
    int v = rng() % 30;
    if (rng() % 3) { tree.insert(v); expected.insert(v); }
    else { assert(tree.erase(v) == expected.erase(v)); }
    for (int key = 0; key < 30; ++key) {
      assert(tree.count(key) == expected.count(key));
      assert(tree.lower_bound(key) - tree.begin() == std::distance(expected.begin(), expected.lower_bound(key)));
      assert(tree.upper_bound(key) - tree.begin() == std::distance(expected.begin(), expected.upper_bound(key)));
    }
    assert(std::vector<int>(tree.begin(), tree.end()) == std::vector<int>(expected.begin(), expected.end()));
  }
}
