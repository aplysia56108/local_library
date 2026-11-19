#include "data-structure/btree/bplustree-set.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

template <class Tree>
void check_content_and_lookup(const Tree& tree,
                              const std::vector<long long>& values) {
  assert(tree.size() == values.size());
  assert(std::equal(tree.begin(), tree.end(), values.begin(), values.end()));
  for (long long key : {-1LL, 0LL, 1LL, 7LL, 16LL, 31LL, 48LL, 96LL}) {
    const auto lower = std::lower_bound(values.begin(), values.end(), key);
    const auto upper = std::upper_bound(values.begin(), values.end(), key);
    assert(tree.lower_bound(key) - tree.begin() ==
           std::distance(values.begin(), lower));
    assert(tree.upper_bound(key) - tree.begin() ==
           std::distance(values.begin(), upper));
  }
}

/**
 * @brief split と merge の整合性、固定番兵・実要素 iterator の安定性を検証する。
 */
template <SetType Type, std::size_t Capacity>
void split_merge_tests() {
  using Tree = Bplustree_Set<long long, Type, std::less<long long>, Capacity>;
  std::vector<long long> values(96);
  for (std::size_t i = 0; i < values.size(); ++i)
    values[i] = Type == SetType::Set ? static_cast<long long>(i)
                                   : static_cast<long long>(i / 3);

  const std::vector<std::size_t> cuts{
      0, 1, 2, Capacity - 1, Capacity, Capacity + 1,
      47, 48, 49, values.size() - 1, values.size()};

  for (const std::size_t cut : cuts) {
    Tree tree(values.begin(), values.end());
    const auto old_bbegin = tree.bbegin();
    const auto old_end = tree.end();
    const auto saved = tree.nth(61);
    const long long* saved_address = std::addressof(*saved);

    Tree right = tree.split_by_index(cut);
    check_content_and_lookup(
        tree, std::vector<long long>(values.begin(), values.begin() + cut));
    check_content_and_lookup(
        right, std::vector<long long>(values.begin() + cut, values.end()));
    assert(tree.bbegin() == old_bbegin);
    assert(tree.end() == old_end);
    assert(std::addressof(*saved) == saved_address);
    if (61 < cut)
      assert(saved == tree.nth(61));
    else
      assert(saved == right.nth(61 - cut));

    tree.merge(right);
    check_content_and_lookup(tree, values);
    assert(right.empty());
    assert(tree.bbegin() == old_bbegin && tree.end() == old_end);
    assert(saved == tree.nth(61));
    right.insert(1000);
    assert(right.erase(1000) == 1);
    assert(right.empty());
  }

  for (long long key : {-1LL, 0LL, 1LL, 7LL, 16LL, 31LL, 48LL, 96LL}) {
    Tree tree(values.begin(), values.end());
    const auto cut = static_cast<std::size_t>(
        std::lower_bound(values.begin(), values.end(), key) - values.begin());
    Tree right = tree.split_by_key(key);
    check_content_and_lookup(
        tree, std::vector<long long>(values.begin(), values.begin() + cut));
    check_content_and_lookup(
        right, std::vector<long long>(values.begin() + cut, values.end()));
    tree.merge(right);
    check_content_and_lookup(tree, values);
  }

  {
    Tree tree(values.begin(), values.end());
    const auto pos = tree.nth(33);
    Tree right = tree.split(pos);
    assert(pos == right.begin());
    tree.merge(right);
    check_content_and_lookup(tree, values);
  }

  {
    // Base の特殊メンバ関数が内部で swap_end を使う経路を検査する。
    Tree tree(values.begin(), values.end());
    Tree other;
    other.insert(200);
    const auto old_bbegin = other.bbegin();
    const auto old_end = other.end();
    other = tree;
    check_content_and_lookup(other, values);
    assert(other.bbegin() == old_bbegin && other.end() == old_end);

    Tree moved(std::move(other));
    check_content_and_lookup(moved, values);
    assert(other.empty());

    Tree assigned;
    assigned.insert(300);
    assigned = std::move(moved);
    check_content_and_lookup(assigned, values);
    // Base のムーブ代入は swap による交換。移動元には旧 assigned が残る。
    check_content_and_lookup(moved, {300});
    // 移動元も引き続き操作可能なことを確認する。
    assert(moved.erase(300) == 1);
    assert(moved.empty());
  }

  std::cerr << "set split / merge tests all passed! (type="
            << (Type == SetType::Set ? "set" : "multiset")
            << ", capacity=" << Capacity << ")\n";
}

template <std::size_t Capacity>
void descending_split_merge_tests() {
  struct Compare {
    bool descending;
    bool operator()(long long a, long long b) const {
      return descending ? a > b : a < b;
    }
  };
  using Tree = Bplustree_Set<long long, SetType::Set, Compare, Capacity>;
  Tree tree(Compare{true});
  const std::vector<long long> values{9, 7, 5, 3, 1};
  tree.assign(values.begin(), values.end());

  auto right = tree.split_by_key(5);
  assert((std::vector<long long>(tree.begin(), tree.end()) ==
          std::vector<long long>{9, 7}));
  assert((std::vector<long long>(right.begin(), right.end()) ==
          std::vector<long long>{5, 3, 1}));
  tree.merge(right);
  assert((std::vector<long long>(tree.begin(), tree.end()) == values));
  assert(*tree.lower_bound(6) == 5);

  auto again = tree.split_by_index(3);
  assert((std::vector<long long>(tree.begin(), tree.end()) ==
          std::vector<long long>{9, 7, 5}));
  tree.merge(again);
  assert((std::vector<long long>(tree.begin(), tree.end()) == values));
  std::cerr << "descending split / merge tests all passed! (capacity="
            << Capacity << ")\n";
}

signed main() {
  split_merge_tests<SetType::Set, 4>();
  split_merge_tests<SetType::Set, 8>();
  split_merge_tests<SetType::MultiSet, 4>();
  split_merge_tests<SetType::MultiSet, 8>();
  descending_split_merge_tests<4>();
  descending_split_merge_tests<8>();
  std::cerr << "All B+tree set split / merge tests passed!\n";
}
