#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>

#define private public
#include "data-structure/set/btree-set.hpp"
#undef private

using SmallSet = Btree_Set<int, std::less<int>, 3>;
using Node = SmallSet::Node;

template <class Set>
std::size_t verify_node(const typename Set::Node* node,
                        const typename Set::Node* parent,
                        std::size_t position,
                        std::optional<typename Set::key_type> lower,
                        std::optional<typename Set::key_type> upper,
                        bool is_root) {
  assert(node != nullptr);
  assert(node->parent == parent);
  assert(node->position == position);
  assert(node->key_count <= Set::kMaxKeys);
  if (!is_root) assert(node->key_count >= Set::kMinKeys);

  for (std::size_t i = 0; i < node->key_count; ++i) {
    if (lower) assert(*lower < node->keys[i]);
    if (upper) assert(node->keys[i] < *upper);
    if (i > 0) assert(node->keys[i - 1] < node->keys[i]);
  }

  std::size_t subtree_size = node->key_count;
  if (node->is_leaf) {
    for (typename Set::Node* child : node->children) {
      assert(child == nullptr);
    }
  } else {
    // 内部ノードの有効な子は key_count + 1 個である。
    for (std::size_t i = 0; i <= node->key_count; ++i) {
      assert(node->children[i] != nullptr);
      const std::optional<typename Set::key_type> child_lower =
          i == 0 ? lower
                 : std::optional<typename Set::key_type>{node->keys[i - 1]};
      const std::optional<typename Set::key_type> child_upper =
          i == node->key_count
              ? upper
              : std::optional<typename Set::key_type>{node->keys[i]};
      subtree_size += verify_node<Set>(node->children[i], node, i,
                                       child_lower, child_upper, false);
    }
    for (std::size_t i = node->key_count + 1;
         i < Set::kMaxChildren; ++i) {
      assert(node->children[i] == nullptr);
    }
  }

  assert(node->subtree_size == subtree_size);
  return subtree_size;
}

template <class Set>
void verify_tree(const Set& set) {
  assert(set.root_ != nullptr);
  assert(verify_node<Set>(set.root_, nullptr, 0, std::nullopt, std::nullopt,
                          true) == set.size());
}

template <class Set>
void test_insert() {
  Set set;
  Set emplace_test;
  const auto [emplaced, emplace_inserted] = emplace_test.emplace(42);
  assert(emplace_inserted);
  assert(*emplaced == 42);

  assert(set.erase(-1) == 0);
  for (int i = 0; i < 1000; ++i) {
    const int key = (i * 73) % 1000;
    const auto [it, inserted] = set.insert(key);
    (void)it;
    assert(inserted);
    assert(set.size() == static_cast<std::size_t>(i + 1));
    verify_tree(set);
  }
  for (int key = 0; key < 1000; ++key) {
    const auto [it, inserted] = set.insert(key);
    (void)it;
    assert(!inserted);
    assert(set.size() == 1000);

    const auto [node, key_index] = set.locate(key);
    assert(node != nullptr);
    assert(node->keys[key_index] == key);
    verify_tree(set);
  }

  std::set<int> reference;
  for (int key = 0; key < 1000; ++key) {
    reference.insert(key);
  }

  auto it = set.begin();
  for (int key = 0; key < 1000; ++key, ++it) {
    assert(it != set.end());
    assert(*it == key);
    assert(set.index_of(typename Set::const_iterator(it)) ==
           static_cast<std::size_t>(key));
    assert(*set.nth(key) == key);
    assert(set.order_of_key(key) == static_cast<std::size_t>(key));
  }
  assert(it == set.end());

  it = set.end();
  for (int key = 999; key >= 0; --key) {
    --it;
    assert(*it == key);
  }
  assert(it == set.begin());

  const Set& const_set = set;
  for (int index = 0; index <= 1000; ++index) {
    const auto advanced = set.begin() + index;
    assert(advanced - set.begin() == index);
    assert(set.end() - advanced == 1000 - index);
    if (index < 1000) assert(*advanced == index);

    const auto const_advanced = const_set.cbegin() + index;
    assert(const_advanced - const_set.cbegin() == index);
    assert(const_set.cend() - const_advanced == 1000 - index);
    if (index < 1000) assert(*const_advanced == index);
  }

  for (int key = -1; key <= 1000; ++key) {
    const auto expected_lower = reference.lower_bound(key);
    const auto expected_upper = reference.upper_bound(key);
    const auto lower = set.lower_bound(key);
    const auto upper = set.upper_bound(key);
    const auto [equal_first, equal_second] = set.equal_range(key);
    const auto const_lower = const_set.lower_bound(key);
    const auto const_upper = const_set.upper_bound(key);
    const auto [const_equal_first, const_equal_second] =
        const_set.equal_range(key);

    assert(const_set.contains(key) == reference.contains(key));
    assert((const_set.find(key) == const_set.end()) ==
           (reference.find(key) == reference.end()));
    assert(const_equal_first == const_lower);
    assert(const_equal_second == const_upper);
    assert(equal_first == lower);
    assert(equal_second == upper);
    assert((lower == set.end()) == (expected_lower == reference.end()));
    assert((upper == set.end()) == (expected_upper == reference.end()));
    if (expected_lower != reference.end()) assert(*lower == *expected_lower);
    if (expected_upper != reference.end()) assert(*upper == *expected_upper);
    if (expected_lower != reference.end()) {
      assert(*const_lower == *expected_lower);
    }
    if (expected_upper != reference.end()) {
      assert(*const_upper == *expected_upper);
    }
    assert(set.order_of_key(key) ==
           static_cast<std::size_t>(
               std::distance(reference.begin(), expected_lower)));
  }

  for (int i = 0; i < 1000; ++i) {
    const int key = (i * 137) % 1000;
    const auto reference_it = reference.find(key);
    assert(reference_it != reference.end());
    const auto reference_next = reference.erase(reference_it);

    if (i % 2 == 0) {
      const auto found = set.find(key);
      assert(found != set.end());
      const auto next = set.erase(typename Set::const_iterator(found));
      assert((next == set.end()) == (reference_next == reference.end()));
      if (reference_next != reference.end()) assert(*next == *reference_next);
    } else {
      assert(set.erase(key) == 1);
    }
    assert(set.size() == static_cast<std::size_t>(999 - i));
    assert(set.locate(key).first == nullptr);
    assert(set.erase(key) == 0);

    if (set.empty()) {
      assert(set.root_ == nullptr);
    } else {
      verify_tree(set);
    }
  }

  assert(set.empty());
  assert(set.size() == 0);
}

int main() {
  using Set = Btree_Set<int>;

  static_assert(std::default_initializable<Set>);
  static_assert(std::same_as<Set::key_type, int>);
  static_assert(std::same_as<Set::value_type, int>);
  static_assert(std::same_as<Set::size_type, std::size_t>);
  static_assert(std::same_as<Set::iterator, Btree_Set<int>::iterator>);
  static_assert(
      std::same_as<Set::const_iterator, Btree_Set<int>::const_iterator>);

  Node leaf;
  leaf.insert_non_full(1, nullptr, 0);
  leaf.insert_non_full(3, nullptr, 1);
  leaf.insert_non_full(5, nullptr, 2);

  const auto leaf_split = leaf.split_and_insert(4, nullptr, 2);
  assert(leaf.key_count == 1);
  assert(leaf.keys[0] == 1);
  assert(leaf.subtree_size == 1);
  assert(leaf_split.separator == 3);
  assert(leaf_split.right->key_count == 2);
  assert(leaf_split.right->keys[0] == 4);
  assert(leaf_split.right->keys[1] == 5);
  assert(leaf_split.right->subtree_size == 2);
  delete leaf_split.right;

  Node internal;
  internal.is_leaf = false;
  internal.key_count = 3;
  internal.keys[0] = 10;
  internal.keys[1] = 20;
  internal.keys[2] = 30;
  std::array<Node, 5> children = {Node(1), Node(2), Node(3), Node(4),
                                  Node(5)};
  for (std::size_t i = 0; i < 4; ++i) {
    internal.children[i] = &children[i];
    children[i].parent = &internal;
    children[i].position = i;
  }
  internal.recalculate_subtree_size();

  const auto internal_split = internal.split_and_insert(25, &children[4], 2);
  assert(internal.key_count == 1);
  assert(internal.keys[0] == 10);
  assert(internal.children[0] == &children[0]);
  assert(internal.children[1] == &children[1]);
  assert(internal_split.separator == 20);
  assert(internal_split.right->key_count == 2);
  assert(internal_split.right->keys[0] == 25);
  assert(internal_split.right->keys[1] == 30);
  assert(internal_split.right->children[0] == &children[2]);
  assert(internal_split.right->children[1] == &children[4]);
  assert(internal_split.right->children[2] == &children[3]);
  assert(children[4].parent == internal_split.right);
  assert(children[4].position == 1);
  delete internal_split.right;

  test_insert<SmallSet>();
  test_insert<Btree_Set<int, std::less<int>, 10>>();
}
