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
  if (!std::equal(actual.begin(), actual.end(), expected.begin(),
                  expected.end())) {
    std::cerr << "Mismatch at " << where.file_name() << ":" << where.line()
              << " (" << where.function_name() << ")\n";
    std::cerr << "  expected:";
    for (auto x : expected) std::cerr << ' ' << x;
    std::cerr << "\n  actual:  ";
    for (auto x : actual) std::cerr << ' ' << x;
    std::cerr << '\n';
    assert(false && "B+tree contents differ from expected");
  }

  assert(tree.end() - tree.begin() ==
         static_cast<std::ptrdiff_t>(expected.size()));
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
  std::cerr << "before resize(5)\n";
  a.resize(5, 6);
  std::cerr << "after resize(5)\n";
  check(a, {7, 7, 7, 6, 6});

  std::cerr << "before resize(2)\n";
  a.resize(2);
  std::cerr << "after resize(2)\n";

  check(a, {7, 7});
  a.rebuild();
  check(a, {7, 7});
  a.clear();
  check(a, {});

  std::cerr << "base construction tests all passed! (capacity=" << Capacity
            << ")\n";
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

  std::cerr << "base access and edit tests all passed! (capacity=" << Capacity
            << ")\n";
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

  std::cerr << "base structural operations tests all passed! (capacity="
            << Capacity << ")\n";
}

// ID 木の所有権移動と破棄で、Pool スロットが回収されることを確認する。
template <std::size_t Capacity>
void pool_reclamation_tests() {
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Pools = Bplustree::NodePools<Traits>;
  const auto before_leaves = Pools::leaves().size();
  const auto before_internals = Pools::internals().size();
  {
    Tree<Capacity> a{1, 2, 3, 4, 5, 6, 7, 8};
    auto right = a.split(4);
    a.merge(right);
    a.erase(2, 6);
    a.insert(1, 42);
    a.reverse();
    a.clear();
    a.assign(3, 10);
  }
  assert(Pools::leaves().size() == before_leaves);
  assert(Pools::internals().size() == before_internals);
  std::cerr << "base pool reclamation tests all passed! (capacity=" << Capacity
            << ")\n";
}


/** @brief 多段の分割・統合でも、祖先へのsize差分とLeaf順序が一致する。 */
template <std::size_t Capacity>
void incremental_structure_regression() {
  Tree<Capacity> tree;
  std::vector<long long> expected;
  for (long long i = 0; i < 64; ++i) {
    tree.push_back(i);
    expected.push_back(i);
  }
  const auto verify = [&]() {
    assert(tree.size() == expected.size());
    assert(std::equal(tree.begin(), tree.end(), expected.begin(),
                      expected.end()));
    assert(tree.end() - tree.begin() ==
           static_cast<std::ptrdiff_t>(expected.size()));
  };
  verify();
  for (std::size_t round = 0; round < 24; ++round) {
    const std::size_t split_at = (round * 7 + 3) % (expected.size() + 1);
    auto tail = tree.split(split_at);
    assert(tree.size() == split_at);
    assert(tail.size() == expected.size() - split_at);
    tree.merge(tail);
    verify();

    tree.insert(split_at, static_cast<long long>(1000 + round));
    expected.insert(expected.begin() + split_at,
                    static_cast<long long>(1000 + round));
    verify();

    const std::size_t removed = (round * 11 + 5) % expected.size();
    tree.erase(removed);
    expected.erase(expected.begin() + removed);
    if (round % 3 == 0) {
      tree.reverse();
      std::reverse(expected.begin(), expected.end());
    }
    verify();
  }
  std::cerr << "incremental structural size tests passed! (capacity="
            << Capacity << ")\n";
}


/** @brief Traits共有epochによるiterator経路キャッシュを検証する。 */
template <std::size_t Capacity>
void iterator_epoch_tests() {
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Epoch = Bplustree::IteratorEpoch<Traits>;
  Tree<Capacity> tree;
  std::vector<long long> expected;
  for (long long i = 0; i < 96; ++i) {
    tree.push_back(i * 100);
    expected.push_back(i * 100);
  }

  auto saved = tree.nth(23);
  auto copy = saved;
  assert(*saved == 2300);
  const auto initial_epoch = Epoch::current;
  // 読み取りとコピーではepochを変えない。キャッシュが違っても同じLeaf。
  assert(saved == copy);
  const auto verify = [&]() {
    assert(std::equal(tree.begin(), tree.end(), expected.begin(),
                      expected.end()));
    assert(tree.end() - tree.begin() ==
           static_cast<std::ptrdiff_t>(expected.size()));
    assert(*saved == 2300);
    auto next = saved;
    ++next;
    const auto index = static_cast<std::size_t>(
        std::find(expected.begin(), expected.end(), 2300) - expected.begin());
    assert(index + 1 < expected.size());
    assert(*next == expected[index + 1]);
    assert(saved == copy);
  };
  verify();
  assert(Epoch::current == initial_epoch);

  tree.reverse(7, 71);
  std::reverse(expected.begin() + 7, expected.begin() + 71);
  assert(Epoch::current != initial_epoch);
  verify();
  tree.reverse();
  std::reverse(expected.begin(), expected.end());
  verify();

  // split/mergeによってLeafの所属Treeと親IDが変化しても既存iteratorは有効。
  const auto split_pos = static_cast<std::size_t>(
      std::find(expected.begin(), expected.end(), 2300) - expected.begin());
  auto right = tree.split(split_pos);
  assert(*saved == 2300);
  assert(std::equal(right.begin(), right.end(), expected.begin() + split_pos,
                    expected.end()));
  tree.merge(right);
  verify();

  // 参照先でないLeafのerase/insert後にも既存iteratorを再利用できる。
  tree.insert(0, -100);
  expected.insert(expected.begin(), -100);
  tree.erase(1);
  expected.erase(expected.begin() + 1);
  verify();
  std::cerr << "iterator epoch cache tests passed! (capacity="
            << Capacity << ")\n";
}


/**
 * @brief 全体反転時に、保持済みの番兵iteratorのepochを無効化できることを検証する。
 * @details reverse()はbbegin_/end_を交換するが、古い番兵iteratorのコピーも
 * 同じLeaf IDを追跡する。反転後の++/--は未伝搬の反転を必ず反映する必要がある。
 */
template <std::size_t Capacity>
void iterator_epoch_reverse_sentinels_test() {
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Epoch = Bplustree::IteratorEpoch<Traits>;
  std::vector<long long> values(96);
  std::iota(values.begin(), values.end(), 1LL);
  Tree<Capacity> tree(values.begin(), values.end());

  auto old_bbegin = tree.bbegin();
  auto old_end = tree.end();
  // 両番兵を一度移動して戻し、逆転前のepochがキャッシュされた状態にする。
  ++old_bbegin;
  --old_bbegin;
  --old_end;
  ++old_end;
  assert(old_bbegin == tree.bbegin());
  assert(old_end == tree.end());

  const auto old_epoch = Epoch::current;
  tree.reverse();
  assert(Epoch::current != old_epoch);

  // 反転後、旧左番兵が右番兵に、旧右番兵が左番兵になる。
  assert(old_bbegin == tree.end());
  assert(old_end == tree.bbegin());
  auto last = old_bbegin;
  --last;
  assert(*last == 1);
  auto first = old_end;
  ++first;
  assert(*first == 96);
  assert(first == tree.begin());
  assert(tree.end() - tree.bbegin() == 97);
  assert(std::equal(tree.begin(), tree.end(),
                    values.rbegin(), values.rend()));

  // 反転後に取得したコピーについても、2度目の全体反転で失効する。
  auto second_bbegin = tree.bbegin();
  auto second_end = tree.end();
  ++second_bbegin;
  --second_bbegin;
  --second_end;
  ++second_end;
  tree.reverse();
  assert(second_bbegin == tree.end());
  assert(second_end == tree.bbegin());
  ++second_end;
  --second_bbegin;
  assert(*second_end == 1);
  assert(*second_bbegin == 96);
  assert(std::equal(tree.begin(), tree.end(),
                    values.begin(), values.end()));

  std::cerr << "iterator epoch reverse sentinel tests passed! (capacity="
            << Capacity << ")\n";
}

int main() {
  std::cerr << "start construction<4>\n";
  construction<4>();
  std::cerr << "end construction<4>\n";

  std::cerr << "start construction<8>\n";
  construction<8>();
  std::cerr << "end construction<8>\n";

  access_and_edit<4>();
  access_and_edit<8>();
  structural_operations<4>();
  structural_operations<8>();
  pool_reclamation_tests<4>();
  pool_reclamation_tests<8>();
  incremental_structure_regression<4>();
  incremental_structure_regression<8>();
  incremental_structure_regression<16>();
  iterator_epoch_tests<4>();
  iterator_epoch_tests<8>();
  iterator_epoch_tests<16>();
  iterator_epoch_reverse_sentinels_test<4>();
  iterator_epoch_reverse_sentinels_test<8>();
  iterator_epoch_reverse_sentinels_test<16>();

  std::cerr << "All B+tree base test cases passed!\n";
}
