#include "data-structure/btree/bplustree-set.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <random>
#include <ranges>
#include <set>
#include <vector>

#define CHECK(expr)                                                    \
  do {                                                                 \
    if (!(expr)) {                                                     \
      std::cerr << "CHECK failed: " #expr << " at " << __FILE__ << ':' \
                << __LINE__ << '\n';                                   \
      std::abort();                                                    \
    }                                                                  \
  } while (false)

using Key = long long;
using Tree = Bplustree_Set<long long, SetType::Set, std::less<long long>, 4>;

static_assert(std::random_access_iterator<Tree::iterator>);
static_assert(std::random_access_iterator<Tree::const_iterator>);
static_assert(std::same_as<std::iter_value_t<Tree::iterator>, Key>);
static_assert(std::same_as<std::iter_reference_t<Tree::iterator>, const Key&>);

std::vector<Key> to_vector(const Tree& tree) {
  return {tree.begin(), tree.end()};
}

std::vector<Key> to_reverse_vector(const Tree& tree) {
  return {tree.rbegin(), tree.rend()};
}

void check_equal(const Tree& tree, const std::set<Key>& ref) {
  const std::vector<Key> expected(ref.begin(), ref.end());
  const std::vector<Key> actual = to_vector(tree);

  CHECK(tree.size() == ref.size());
  CHECK(tree.empty() == ref.empty());
  CHECK(actual == expected);

  CHECK(tree.begin() == tree.cbegin());
  CHECK(tree.end() == tree.cend());
  CHECK(tree.rbegin() == tree.crbegin());
  CHECK(tree.rend() == tree.crend());

  std::vector<Key> reversed = expected;
  std::reverse(reversed.begin(), reversed.end());
  CHECK(to_reverse_vector(tree) == reversed);

  CHECK(tree.end() - tree.begin() == static_cast<std::ptrdiff_t>(tree.size()));
  CHECK(std::distance(tree.begin(), tree.end()) ==
        static_cast<std::ptrdiff_t>(tree.size()));
  CHECK(std::ranges::distance(tree.begin(), tree.end()) ==
        static_cast<std::ptrdiff_t>(tree.size()));

  if (tree.empty()) {
    CHECK(tree.begin() == tree.end());
    CHECK(tree.rbegin() == tree.rend());
    CHECK(tree.bbegin() + 1 == tree.begin());
    return;
  }

  CHECK(*tree.begin() == *ref.begin());
  CHECK(*tree.rbegin() == *ref.rbegin());
  CHECK(tree.bbegin() + 1 == tree.begin());
  CHECK(tree.begin() - tree.bbegin() == 1);
  CHECK(tree.end() - tree.bbegin() ==
        static_cast<std::ptrdiff_t>(tree.size()) + 1);

  // random access operators
  for (std::size_t i = 0; i < expected.size(); ++i) {
    const auto d = static_cast<std::ptrdiff_t>(i);
    auto it = tree.begin() + d;
    CHECK(*it == expected[i]);
    CHECK(tree.begin()[d] == expected[i]);
    CHECK(it - tree.begin() == d);
    CHECK(tree.end() - it == static_cast<std::ptrdiff_t>(expected.size() - i));

    CHECK((tree.begin() <= it));
    CHECK((it >= tree.begin()));
    CHECK((it < tree.end()));
    CHECK((tree.end() > it));
  }

  // += / -= across Leaf boundaries.
  auto it = tree.begin();
  it += static_cast<std::ptrdiff_t>(tree.size());
  CHECK(it == tree.end());
  it -= static_cast<std::ptrdiff_t>(tree.size());
  CHECK(it == tree.begin());

  // reverse_iterator random access
  for (std::size_t i = 0; i < reversed.size(); ++i) {
    const auto d = static_cast<std::ptrdiff_t>(i);
    CHECK(tree.rbegin()[d] == reversed[i]);
    CHECK(*(tree.rbegin() + d) == reversed[i]);
  }
  CHECK(tree.rend() - tree.rbegin() ==
        static_cast<std::ptrdiff_t>(tree.size()));
}

void check_queries(const Tree& tree, const std::set<Key>& ref, Key lo, Key hi) {
  for (Key key = lo; key <= hi; ++key) {
    const auto lb = tree.lower_bound(key);
    const auto ub = tree.upper_bound(key);
    const auto [first, last] = tree.equal_range(key);
    const auto found = tree.find(key);

    const auto rlb = ref.lower_bound(key);
    const auto rub = ref.upper_bound(key);
    const auto rfound = ref.find(key);

    CHECK((lb == tree.end()) == (rlb == ref.end()));
    if (lb != tree.end()) CHECK(*lb == *rlb);

    CHECK((ub == tree.end()) == (rub == ref.end()));
    if (ub != tree.end()) CHECK(*ub == *rub);

    CHECK(first == lb);
    CHECK(last == ub);
    CHECK(tree.count(key) == ref.count(key));
    CHECK(tree.contains(key) == ref.contains(key));

    CHECK((found == tree.end()) == (rfound == ref.end()));
    if (found != tree.end()) CHECK(*found == *rfound);

    CHECK(tree.min_ge(key) == lb);
    CHECK(tree.min_gt(key) == ub);

    auto lt = tree.max_lt(key);
    if (rlb == ref.begin()) {
      CHECK(lt == tree.bbegin());
    } else {
      auto expected = rlb;
      --expected;
      CHECK(lt != tree.bbegin());
      CHECK(*lt == *expected);
    }

    auto le = tree.max_le(key);
    if (rub == ref.begin()) {
      CHECK(le == tree.bbegin());
    } else {
      auto expected = rub;
      --expected;
      CHECK(le != tree.bbegin());
      CHECK(*le == *expected);
    }
  }
}

void deterministic_test() {
  Tree tree;
  std::set<Key> ref;

  check_equal(tree, ref);
  check_queries(tree, ref, -5, 5);

  // NodeCapacity=64 なので、十分な要素数で複数 Leaf / Internal を作る。
  std::vector<Key> values;
  for (Key i = 0; i < 600; ++i) values.push_back(i * 2);

  // 挿入順を崩して左右の Leaf にまんべんなく挿入する。
  std::mt19937_64 rng(123456789);
  std::shuffle(values.begin(), values.end(), rng);

  for (Key x : values) {
    const auto [it, inserted] = tree.insert(x);
    const auto [rit, rinserted] = ref.insert(x);
    CHECK(inserted == rinserted);
    CHECK(*it == *rit);
  }

  check_equal(tree, ref);
  check_queries(tree, ref, -10, 1210);

  // Set では重複 insert は失敗。
  for (Key x : {0LL, 100LL, 500LL, 1198LL}) {
    const auto old_size = tree.size();
    const auto [it, inserted] = tree.insert(x);
    CHECK(!inserted);
    CHECK(*it == x);
    CHECK(tree.size() == old_size);
  }

  // erase(iterator): 先頭、中間、末尾。
  {
    auto it = tree.begin();
    const Key x = *it;
    auto next = tree.erase(it);
    ref.erase(x);
    CHECK(next == tree.begin());
    check_equal(tree, ref);
  }

  {
    auto it = tree.lower_bound(500);
    CHECK(it != tree.end());
    const Key x = *it;
    auto expected = std::next(ref.find(x));
    auto next = tree.erase(it);
    ref.erase(x);
    if (expected == ref.end())
      CHECK(next == tree.end());
    else
      CHECK(*next == *expected);
    check_equal(tree, ref);
  }

  {
    auto it = tree.end() - 1;
    const Key x = *it;
    auto next = tree.erase(it);
    ref.erase(x);
    CHECK(next == tree.end());
    check_equal(tree, ref);
  }

  // erase(key)
  CHECK(tree.erase(123456) == 0);
  CHECK(tree.erase(200) == ref.erase(200));
  check_equal(tree, ref);

  // single_erase。Set では最大1個。
  {
    const auto expected = ref.erase(202);
    CHECK(tree.erase(202, true) == expected);
    check_equal(tree, ref);
  }

  tree.clear();
  ref.clear();
  check_equal(tree, ref);
  check_queries(tree, ref, -5, 5);

  // clear 後の再利用。
  for (Key x : {5, 1, 9, 3, 7}) {
    tree.insert(x);
    ref.insert(x);
  }
  check_equal(tree, ref);
  check_queries(tree, ref, 0, 10);
}

void random_stress_test() {
  Tree tree;
  std::set<Key> ref;

  std::mt19937_64 rng(0x123456789abcdefULL);
  std::uniform_int_distribution<Key> key_dist(-1000, 1000);
  std::uniform_int_distribution<int> op_dist(0, 99);

  constexpr int kOperations = 100000;

  for (int step = 1; step <= kOperations; ++step) {
    const Key key = key_dist(rng);
    const int op = op_dist(rng);

    if (op < 40) {
      const auto [it, inserted] = tree.insert(key);
      const auto [rit, rinserted] = ref.insert(key);
      CHECK(inserted == rinserted);
      CHECK(*it == *rit);
    } else if (op < 65) {
      const auto erased = tree.erase(key);
      const auto rerased = ref.erase(key);
      CHECK(erased == rerased);
    } else if (op < 75) {
      // erase(iterator)
      if (!ref.empty()) {
        std::uniform_int_distribution<std::size_t> index_dist(0,
                                                              ref.size() - 1);
        const std::size_t index = index_dist(rng);

        auto it = tree.begin() + static_cast<std::ptrdiff_t>(index);
        auto rit = ref.begin();
        std::advance(rit, static_cast<std::ptrdiff_t>(index));
        CHECK(*it == *rit);

        auto rnext = std::next(rit);
        auto next = tree.erase(it);
        ref.erase(rit);

        if (rnext == ref.end())
          CHECK(next == tree.end());
        else
          CHECK(*next == *rnext);
      }
    } else {
      // 読み取り系。
      const auto it = tree.find(key);
      const auto rit = ref.find(key);
      CHECK((it == tree.end()) == (rit == ref.end()));
      if (it != tree.end()) CHECK(*it == *rit);

      const auto lb = tree.lower_bound(key);
      const auto rlb = ref.lower_bound(key);
      CHECK((lb == tree.end()) == (rlb == ref.end()));
      if (lb != tree.end()) CHECK(*lb == *rlb);

      const auto ub = tree.upper_bound(key);
      const auto rub = ref.upper_bound(key);
      CHECK((ub == tree.end()) == (rub == ref.end()));
      if (ub != tree.end()) CHECK(*ub == *rub);
    }

    if (step % 1000 == 0) {
      check_equal(tree, ref);
      check_queries(tree, ref, -1010, 1010);
    }
  }

  check_equal(tree, ref);
  check_queries(tree, ref, -1010, 1010);
}

int main() {
  deterministic_test();
  random_stress_test();

  std::cout << "All tests passed.\n";
}
