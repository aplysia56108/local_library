#include "data-structure/btree/components/bplustree-base.hpp"
// The refactoring components must coexist with the original public library.
#include "data-structure/btree/bplustree-vector.hpp"
#include "data-structure/btree/bplustree-segtree.hpp"
#include "data-structure/btree/bplustree-lazysegtree.hpp"

#include <cassert>
#include <numeric>
#include <random>
#include <type_traits>
#include <vector>

template <class T>
concept Reversible = requires(T& tree) { tree.reverse(); tree.reverse(0, 1); };
static_assert(!Reversible<Bplustree::Base<Bplustree::Traits<int>>>);
static_assert(Reversible<Bplustree::Base<Bplustree::Traits<int>, true>>);

template <std::size_t Capacity>
struct CheckedTree : Bplustree::Base<Bplustree::Traits<int, Capacity>, true> {
  using Parent = Bplustree::Base<Bplustree::Traits<int, Capacity>, true>;
  using Parent::Parent;
  using Node = typename Parent::Node;
  using Internal = typename Parent::Internal;
  std::size_t check_node(Node* node) {
    assert(node->count < Capacity);
    if (node->is_leaf()) {
      assert(node->level == 0 && node->size == node->count);
      return node->size;
    }
    Parent::materialize(node);
    auto* internal = static_cast<Internal*>(node);
    std::size_t size = 0;
    assert(node->count > 0);
    for (std::size_t i = 0; i < node->count; ++i) {
      auto* child = internal->children[i].get();
      assert(child && child->parent == internal && child->position == i);
      assert(child->level + 1 == node->level);
      size += check_node(child);
    }
    assert(size == node->size);
    return size;
  }
  void check() {
    assert(this->root_->parent == nullptr && this->root_->position == 0);
    assert(check_node(this->root_.get()) == this->size());
  }
};

template <std::size_t Capacity>
void run() {
  using Tree = CheckedTree<Capacity>;
  static_assert(std::same_as<decltype(*std::declval<Tree>().begin()), const int&>);
  Tree tree;
  std::vector<int> expected;
  std::mt19937 rng(20261022);
  for (int step = 0; step < 2500; ++step) {
    const auto size = expected.size();
    const std::size_t pos = rng() % (size + 1);
    switch (rng() % 9) {
      case 0: case 1: {
        int value = rng() % 1000;
        assert(*tree.insert(pos, value) == value);
        expected.insert(expected.begin() + pos, value);
        break;
      }
      case 2:
        if (pos < size) { tree.erase(pos); expected.erase(expected.begin() + pos); }
        break;
      case 3: {
        std::size_t right = pos + rng() % (size - pos + 1);
        tree.reverse(pos, right);
        std::reverse(expected.begin() + pos, expected.begin() + right);
        break;
      }
      case 4: {
        auto right = tree.split(pos);
        assert(std::vector<int>(right.begin(), right.end()) ==
               std::vector<int>(expected.begin() + pos, expected.end()));
        assert(tree.size() == pos);
        tree.merge(right);
        assert(right.empty());
        break;
      }
      case 5:
        if (pos < size) { tree(pos) += 7; expected[pos] += 7; }
        break;
      case 6: {
        auto right = pos + rng() % (size - pos + 1);
        auto it = tree.erase(pos, right);
        expected.erase(expected.begin() + pos, expected.begin() + right);
        assert(it == tree.nth(pos));
        break;
      }
      case 7:
        tree.reverse(); std::reverse(expected.begin(), expected.end()); break;
      case 8: tree.rebuild(); break;
    }
    tree.check();
    assert(std::vector<int>(tree.begin(), tree.end()) == expected);
    assert(tree.end() - tree.begin() == static_cast<std::ptrdiff_t>(expected.size()));
    assert(tree.bbegin() + 1 == tree.begin());
  }
  std::vector<int> many(180);
  std::iota(many.begin(), many.end(), 0);
  tree.assign(many.begin(), many.end());
  tree.reverse();
  tree.insert(40, -7);
  std::reverse(many.begin(), many.end()); many.insert(many.begin() + 40, -7);
  assert(std::vector<int>(tree.begin(), tree.end()) == many);
  tree.check();
  Tree copy = tree;
  assert(std::vector<int>(copy.begin(), copy.end()) == many);
  Tree moved = std::move(copy);
  assert(copy.empty() && moved.size() == many.size());
  copy.push_back(42);
  moved.resize(0);
  assert(moved.empty());
  tree.assign(tree.begin(), tree.end());
  tree.check();
  tree.erase(0, tree.size());
  assert(tree.begin() == tree.end() && tree.bbegin() + 1 == tree.end());
  for (int size : {0, 1, 2, 3, 8, 27, 80}) {
    std::vector<int> values(size);
    std::iota(values.begin(), values.end(), 0);
    for (int index = 0; index <= size; ++index) {
      Tree left(values.begin(), values.end());
      left.reverse();
      auto right = left.split(index);
      std::vector<int> oracle(values.rbegin(), values.rend());
      assert(std::vector<int>(left.begin(), left.end()) ==
             std::vector<int>(oracle.begin(), oracle.begin() + index));
      assert(std::vector<int>(right.begin(), right.end()) ==
             std::vector<int>(oracle.begin() + index, oracle.end()));
      left.merge(right);
      left.check();
      assert(std::vector<int>(left.begin(), left.end()) == oracle);
    }
  }
}

int main() { run<3>(); run<4>(); run<5>(); }
