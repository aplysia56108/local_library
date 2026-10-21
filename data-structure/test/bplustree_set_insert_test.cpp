#include "data-structure/btree/bplustree-set.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <vector>

// Keep structural checks in the test without exposing tree internals publicly.
struct BplustreeSetTestAccess {
  template <class Tree>
  using Node = typename Tree::Node;
  template <class Tree>
  using Leaf = typename Tree::Leaf;
  template <class Tree>
  using Internal = typename Tree::Internal;

  template <class Tree>
  static constexpr std::size_t capacity = Tree::kNodeCapacity;

  template <class Tree>
  static auto root(Tree& tree) { return tree.header_.root_.get(); }
  template <class Tree>
  static auto first_leaf(Tree& tree) { return tree.header_.first_leaf_; }
  template <class Tree>
  static auto last_leaf(Tree& tree) { return tree.header_.last_leaf_; }
};

#define CHECK(expr)                                                    \
  do {                                                                 \
    if (!(expr)) {                                                     \
      std::cout << "CHECK failed: " #expr << " at " << __FILE__ << ':' \
                << __LINE__ << std::endl;                              \
      std::abort();                                                    \
    }                                                                  \
  } while (false)

// Use the public iterator aliases and factory methods.
template <class Tree>
typename Tree::iterator begin_it(Tree& tree) {
  return tree.begin();
}

template <class Tree>
typename Tree::iterator end_it(Tree& tree) {
  return tree.end();
}

template <class Tree>
typename Tree::iterator bbegin_it(Tree& tree) {
  return tree.bbegin();
}

// Node 以下の内部不変条件を再帰的に確認する。
template <class Tree>
std::size_t validate_node(Tree& tree, BplustreeSetTestAccess::Node<Tree>* node,
                          BplustreeSetTestAccess::Internal<Tree>* expected_parent,
                          std::size_t expected_position, std::size_t depth,
                          std::size_t& leaf_depth) {
  CHECK(node != nullptr);
  CHECK(node->parent == expected_parent);

  if (expected_parent != nullptr) {
    CHECK(node->position == expected_position);
  }

  CHECK(node->count > 0);
  CHECK(node->count < BplustreeSetTestAccess::capacity<Tree>);
  CHECK(node->min_key != nullptr);

  if (node->is_leaf()) {
    auto* leaf = static_cast<BplustreeSetTestAccess::Leaf<Tree>*>(node);

    CHECK(node->size == node->count);
    CHECK(node->min_key == &leaf->keys[0]);

    for (std::size_t i = 1; i < leaf->count; ++i) {
      CHECK(!tree.key_comp()(leaf->keys[i], leaf->keys[i - 1]));
    }

    if (leaf_depth == std::numeric_limits<std::size_t>::max()) {
      leaf_depth = depth;
    } else {
      CHECK(leaf_depth == depth);
    }

    return node->size;
  }

  auto* internal = static_cast<BplustreeSetTestAccess::Internal<Tree>*>(node);

  CHECK(node->min_key == internal->children[0]->min_key);

  std::size_t size_sum = 0;

  for (std::size_t i = 0; i < internal->count; ++i) {
    CHECK(internal->children[i] != nullptr);

    if (i > 0) {
      CHECK(!tree.key_comp()(*internal->children[i]->min_key,
                           *internal->children[i - 1]->min_key));
    }

    size_sum += validate_node(tree, internal->children[i].get(), internal, i,
                              depth + 1, leaf_depth);
  }

  // 有効範囲外は nullptr であることも確認する。
  for (std::size_t i = internal->count;
       i < BplustreeSetTestAccess::capacity<Tree>; ++i) {
    CHECK(internal->children[i] == nullptr);
  }

  CHECK(node->size == size_sum);

  return size_sum;
}

// Leaf の prev / next と、全要素の並びを確認する。
template <class Tree>
std::vector<int> validate_leaf_links(Tree& tree) {
  std::vector<int> values;

  auto* leaf = BplustreeSetTestAccess::first_leaf(tree);
  BplustreeSetTestAccess::Leaf<Tree>* prev = nullptr;
  BplustreeSetTestAccess::Leaf<Tree>* last = nullptr;

  std::size_t leaf_count = 0;

  CHECK(leaf != nullptr);
  CHECK(leaf->prev == nullptr);

  while (leaf != nullptr) {
    CHECK(leaf->prev == prev);

    if (prev != nullptr) {
      CHECK(prev->next == leaf);
    }

    for (std::size_t i = 0; i < leaf->count; ++i) {
      values.push_back(leaf->keys[i]);
    }

    prev = leaf;
    last = leaf;
    leaf = leaf->next;

    // link が循環していた場合に無限ループしないため。
    CHECK(++leaf_count <= tree.size() + 1);
  }

  CHECK(last == BplustreeSetTestAccess::last_leaf(tree));
  CHECK(last->next == nullptr);

  // 逆方向も確認する。
  auto* current = BplustreeSetTestAccess::last_leaf(tree);
  BplustreeSetTestAccess::Leaf<Tree>* next = nullptr;
  std::size_t reverse_leaf_count = 0;

  while (current != nullptr) {
    CHECK(current->next == next);
    next = current;
    current = current->prev;
    CHECK(++reverse_leaf_count <= leaf_count);
  }

  CHECK(next == BplustreeSetTestAccess::first_leaf(tree));
  CHECK(reverse_leaf_count == leaf_count);

  return values;
}

template <class Tree, class Ref>
void check_tree_equals(Tree& tree, const Ref& ref) {
  CHECK(tree.size() == ref.size());
  auto* root = BplustreeSetTestAccess::root(tree);
  CHECK(root != nullptr);
  CHECK(root->parent == nullptr);
  CHECK(root->size == tree.size());

  if (ref.empty()) {
    CHECK(root->is_leaf());
    CHECK(root->count == 0);
    CHECK(root == BplustreeSetTestAccess::first_leaf(tree));
    CHECK(root == BplustreeSetTestAccess::last_leaf(tree));
    CHECK(validate_leaf_links(tree).empty());
    CHECK(tree.begin() == tree.end());
    return;
  }

  std::size_t leaf_depth = std::numeric_limits<std::size_t>::max();

  CHECK(validate_node(tree, root, nullptr, 0, 0,
                      leaf_depth) == tree.size());

  const auto actual = validate_leaf_links(tree);
  const std::vector<int> expected(ref.begin(), ref.end());

  CHECK(actual == expected);
}

template <class Tree, class Ref>
void check_lookup(Tree& tree, const Ref& ref, int min_query, int max_query) {
  const auto end = end_it(tree);
  const auto bbegin = bbegin_it(tree);

  for (int x = min_query; x <= max_query; ++x) {
    // lower_bound
    auto it = tree.lower_bound(x);
    auto expected = ref.lower_bound(x);

    if (expected == ref.end()) {
      CHECK(it == end);
    } else {
      CHECK(*it == *expected);
    }

    // max_lt
    auto lt = tree.max_lt(x);
    auto expected_lt = ref.lower_bound(x);

    if (expected_lt == ref.begin()) {
      CHECK(lt == bbegin);
    } else {
      --expected_lt;
      CHECK(*lt == *expected_lt);
    }
  }
}

template <class Tree, class Ref>
void check_iterators(Tree& tree, const Ref& ref) {
  const std::vector<int> expected(ref.begin(), ref.end());

  CHECK(!expected.empty());

  const auto begin = begin_it(tree);
  const auto end = end_it(tree);
  const auto bbegin = bbegin_it(tree);

  // -------------------------
  // ++
  // -------------------------
  {
    std::vector<int> actual;

    for (auto it = begin; it != end; ++it) {
      actual.push_back(*it);
      CHECK(actual.size() <= expected.size());
    }

    CHECK(actual == expected);
  }

  // -------------------------
  // --
  // -------------------------
  {
    std::vector<int> actual;

    auto it = end;

    while (it != begin) {
      --it;
      actual.push_back(*it);
      CHECK(actual.size() <= expected.size());
    }

    auto reversed = expected;
    std::reverse(reversed.begin(), reversed.end());

    CHECK(actual == reversed);
  }

  // -------------------------
  // bbegin
  // -------------------------
  {
    auto it = begin;

    --it;
    CHECK(it == bbegin);

    ++it;
    CHECK(it == begin);
  }

  // -------------------------
  // postfix ++ / --
  // -------------------------
  if (expected.size() >= 2) {
    auto it = begin;

    auto old = it++;

    CHECK(*old == expected[0]);
    CHECK(*it == expected[1]);

    auto rit = end;

    auto old_end = rit--;

    CHECK(old_end == end);
    CHECK(*rit == expected.back());
  }

  // -------------------------
  // Iterator difference
  // -------------------------
  CHECK(end - begin == static_cast<std::ptrdiff_t>(expected.size()));

  CHECK(begin - end == -static_cast<std::ptrdiff_t>(expected.size()));

  CHECK(begin - bbegin == 1);

  CHECK(end - bbegin == static_cast<std::ptrdiff_t>(expected.size()) + 1);

  // 各位置の Iterator を ++ だけで作る。
  std::vector<typename Tree::iterator> positions;

  positions.push_back(begin);

  auto current = begin;

  for (std::size_t i = 0; i < expected.size(); ++i) {
    ++current;
    positions.push_back(current);
  }

  CHECK(positions.back() == end);

  // 小さいケースなら全組み合わせで距離を確認。
  if (expected.size() <= 100) {
    for (std::size_t i = 0; i <= expected.size(); ++i) {
      for (std::size_t j = 0; j <= expected.size(); ++j) {
        CHECK(positions[i] - positions[j] ==
              static_cast<std::ptrdiff_t>(i) - static_cast<std::ptrdiff_t>(j));
      }
    }
  }

  // -------------------------
  // + / - / += / -=
  // -------------------------
  for (std::size_t i = 0; i <= expected.size(); ++i) {
    CHECK(begin + static_cast<std::ptrdiff_t>(i) == positions[i]);

    CHECK(end - static_cast<std::ptrdiff_t>(expected.size() - i) ==
          positions[i]);
  }

  CHECK(begin - 1 == bbegin);
  CHECK(bbegin + 1 == begin);
}

template <std::size_t Capacity>
void deterministic_set_test() {
  using Tree = Bplustree_Set<int, SetType::Set, std::less<int>, Capacity>;

  Tree tree;
  std::set<int> ref;

  check_tree_equals(tree, ref);

  // 順番を崩し、左右両方向の split を多数発生させる。
  const std::vector<int> values = {
      30, 10, 50, 20, 40, 60, 5,  15, 25, 35, 45, 55, 65, 1,
      7,  12, 17, 22, 27, 32, 37, 42, 47, 52, 57, 62, 67,
  };

  for (int x : values) {
    auto [it, inserted] = tree.insert(x);
    auto [expected_it, expected_inserted] = ref.insert(x);

    CHECK(inserted == expected_inserted);
    CHECK(*it == *expected_it);

    check_tree_equals(tree, ref);
  }

  // Set なので重複は入らない。
  for (int x : values) {
    auto [it, inserted] = tree.insert(x);

    CHECK(!inserted);
    CHECK(*it == x);

    check_tree_equals(tree, ref);
  }

  check_lookup(tree, ref, -5, 75);
  check_iterators(tree, ref);
}

template <std::size_t Capacity>
void deterministic_multiset_test() {
  using Tree = Bplustree_Set<int, SetType::MultiSet, std::less<int>, Capacity>;

  Tree tree;
  std::multiset<int> ref;

  check_tree_equals(tree, ref);

  // 同じ値が Leaf 境界を跨ぐケースを重点的に作る。
  const std::vector<int> values = {
      5, 5, 5, 1, 5, 3, 3, 7, 5, 1, 9, 5, 3, 7, 7,
      7, 2, 4, 6, 8, 5, 5, 5, 3, 3, 3, 1, 1, 9, 9,
  };

  for (int x : values) {
    auto [it, inserted] = tree.insert(x);

    ref.insert(x);

    CHECK(inserted);
    CHECK(*it == x);

    check_tree_equals(tree, ref);
  }

  check_lookup(tree, ref, -2, 12);
  check_iterators(tree, ref);
}

void random_set_test() {
  using Tree = Bplustree_Set<int, SetType::Set, std::less<int>, 4>;

  Tree tree;
  std::set<int> ref;

  std::mt19937 rng(123456789);
  std::uniform_int_distribution<int> dist(0, 200);

  for (int t = 0; t < 2000; ++t) {
    const int x = dist(rng);

    auto [it, inserted] = tree.insert(x);
    auto [expected_it, expected_inserted] = ref.insert(x);

    CHECK(inserted == expected_inserted);
    CHECK(*it == *expected_it);

    if (t % 50 == 0) {
      check_tree_equals(tree, ref);
    }
  }

  check_tree_equals(tree, ref);
  check_lookup(tree, ref, -10, 210);
  check_iterators(tree, ref);
}

void random_multiset_test() {
  using Tree = Bplustree_Set<int, SetType::MultiSet, std::less<int>, 4>;

  Tree tree;
  std::multiset<int> ref;

  std::mt19937 rng(987654321);
  std::uniform_int_distribution<int> dist(0, 30);

  for (int t = 0; t < 1000; ++t) {
    const int x = dist(rng);

    auto [it, inserted] = tree.insert(x);

    ref.insert(x);

    CHECK(inserted);
    CHECK(*it == x);

    if (t % 50 == 0) {
      check_tree_equals(tree, ref);
    }
  }

  check_tree_equals(tree, ref);
  check_lookup(tree, ref, -5, 35);
  check_iterators(tree, ref);
}

int main() {
  // 奇数・偶数両方の capacity で split 境界を確認。
  deterministic_set_test<3>();
  deterministic_set_test<4>();
  deterministic_set_test<5>();

  deterministic_multiset_test<3>();
  deterministic_multiset_test<4>();
  deterministic_multiset_test<5>();

  random_set_test();
  random_multiset_test();

  std::cout << "All tests passed.\n";
}
