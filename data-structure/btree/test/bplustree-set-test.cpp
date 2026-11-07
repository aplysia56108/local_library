#include "data-structure/btree/bplustree-set.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
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


/**
 * @brief Set の区間削除による分割・結合後の min_key を検証する。
 * @details 通常の Set::erase(key) は単一要素削除経路を通るため、
 * Base::erase(first, last) を明示的に使って別経路を検証する。
 */
template <SetType Type, std::size_t Capacity>
class InspectableSet
    : public Bplustree_Set<long long, Type, std::less<long long>, Capacity> {
  using Traits = Bplustree::SetTraits<long long, Capacity>;
  using Base = Bplustree::Base<Traits>;
  using Internal = typename Traits::Internal;
  using Leaf = typename Traits::Leaf;

  static const long long* validate_node(const Internal* node,
                                        const char* phase, long long step,
                                        std::size_t depth = 0) {
    assert(node != nullptr);
    if (node->count == 0 || node->count >= Capacity) {
      std::cerr << "invalid Internal: phase=" << phase << ", step=" << step
                << ", depth=" << depth << ", level=" << node->level
                << ", count=" << node->count << ", size=" << node->size
                << ", parent=" << node->parent
                << ", position=" << node->position << '\n';
      assert(node->count > 0 && node->count < Capacity);
    }
    const long long* first = nullptr;
    std::size_t size = 0;

    for (std::size_t i = 0; i < node->count; ++i) {
      const auto* child = node->children[i].get();
      assert(child != nullptr);
      assert(child->parent == node);
      assert(child->position == i);

      const long long* min_key;
      if (node->level) {
        const auto* inner = node->get_child(i);
        assert(inner->level + 1 == node->level);
        min_key = validate_node(inner, phase, step, depth + 1);
        size += inner->size;
      } else {
        min_key = std::addressof(static_cast<const Leaf*>(child)->get());
        ++size;
      }
      if (i == 0) first = min_key;
    }

    for (std::size_t i = node->count; i < Capacity; ++i)
      assert(node->children[i] == nullptr);
    assert(node->size == size);
    assert(node->min_key == first);
    return first;
  }

 public:
  void erase_range(std::size_t first, std::size_t last) {
    Base::erase(first, last);
  }

  void validate_structure(const char* phase, long long step) const {
    assert(this->root_->parent == nullptr);
    assert(validate_node(this->root_.get(), phase, step) ==
           std::addressof(*this->bbegin()));
    assert(this->root_->size == this->size() + 2);
  }
};

template <SetType Type, std::size_t Capacity>
void set_range_erase_tests() {
  InspectableSet<Type, Capacity> tree;
  using Reference = std::conditional_t<Type == SetType::Set,
                                       std::set<long long>,
                                       std::multiset<long long>>;
  Reference reference;

  for (long long i = 0; i < 120; ++i) {
    const long long value = Type == SetType::Set ? i : i / 3;
    tree.insert(value);
    reference.insert(value);
  }

  auto verify = [&](const char* phase, long long step) {
    tree.validate_structure(phase, step);
    const std::vector<long long> expected(reference.begin(), reference.end());
    check(tree, expected);

    for (long long key : {0LL, 7LL, 23LL, 50LL, 120LL, 200LL}) {
      auto lower = reference.lower_bound(key);
      auto upper = reference.upper_bound(key);
      assert(tree.lower_bound(key) - tree.begin() ==
             std::distance(reference.begin(), lower));
      assert(tree.upper_bound(key) - tree.begin() ==
             std::distance(reference.begin(), upper));
    }
  };
  verify("initial", -1);

  // 先頭・末尾・内部の区間を削除する。各ステップで木の深さや密度が変わる。
  for (long long step = 0; step < 200; ++step) {
    const std::size_t n = reference.size();
    const std::size_t first =
        step % 3 == 0 ? 0 : step % 3 == 1 ? n - 1 : (step * 37) % n;
    const std::size_t width =
        1 + (step * 13) % std::min<std::size_t>(20, n - first);
    const std::size_t last = first + width;

    auto it_first = std::next(reference.begin(), first);
    auto it_last = std::next(it_first, width);
    reference.erase(it_first, it_last);
    tree.erase_range(first, last);
    verify("after erase", step);

    // 削除した部分だけでなく、既存キーと同値の挿入も試す。
    for (long long j = 0; j < 3; ++j) {
      const long long key = (step * 53 + j * 17) % 151;
      tree.insert(key);
      reference.insert(key);
    }
    if (reference.size() < 30) {
      for (long long j = 0; j < 50; ++j) {
        const long long key = 200 + step * 100 + j / 2;
        tree.insert(key);
        reference.insert(key);
      }
    }
    verify("after insert", step);
  }

  std::cerr << "set range erase tests all passed! (type="
            << (Type == SetType::Set ? "set" : "multiset")
            << ", capacity=" << Capacity << ")\n";
}

int main() {
  set_tests<4>();
  set_tests<8>();
  multiset_tests<4>();
  multiset_tests<8>();
  set_range_erase_tests<SetType::Set, 4>();
  set_range_erase_tests<SetType::Set, 8>();
  set_range_erase_tests<SetType::MultiSet, 4>();
  set_range_erase_tests<SetType::MultiSet, 8>();
  std::cerr << "All B+tree set test cases passed!\n";
}
