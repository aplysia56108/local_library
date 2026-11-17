#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <random>
#include <vector>

#include "data-structure/btree/bplustree-vector.hpp"

/** @brief 子サイズキャッシュと実際の部分木サイズの不変条件を検証する。 */
template <std::size_t Capacity>
class ChildSizeTestTree : public Bplustree_Vector<long long, Capacity> {
  using Parent = Bplustree_Vector<long long, Capacity>;
  using Traits = Bplustree::Traits<long long, Capacity>;
  using Pools = Bplustree::NodePools<Traits>;
  using id_type = typename Pools::id_type;

  static std::size_t check_internal(id_type node_id) {
    const auto& node = Pools::internals()[node_id];
    std::size_t total = 0;
    for (std::size_t i = 0; i < node.count; ++i) {
      const std::size_t actual =
          node.level ? check_internal(node.children[i]) : 1;
      assert(node.child_sizes[i] == actual);
      total += actual;
    }
    assert(node.size == total);
    return total;
  }

 public:
  using Parent::Parent;

  void check_cache() const {
    assert(check_internal(this->root_) == this->size() + 2);
  }
};

/** @brief insert/erase/range erase/reverse/split/mergeを交互に実行する。 */
template <std::size_t Capacity>
void test_child_size_cache() {
  ChildSizeTestTree<Capacity> tree;
  std::vector<long long> reference;
  std::mt19937_64 random(10000 + Capacity);
  for (std::size_t step = 0; step < 1700; ++step) {
    const std::size_t n = reference.size();
    const auto op = static_cast<unsigned>(random() % 6);
    if (op == 0 || n == 0) {
      const std::size_t pos = random() % (n + 1);
      const long long val = static_cast<long long>(random() % 1000000);
      tree.insert(pos, val);
      reference.insert(reference.begin() + pos, val);
    } else if (op == 1) {
      const std::size_t pos = random() % n;
      tree.erase(pos);
      reference.erase(reference.begin() + pos);
    } else if (op == 2) {
      std::size_t a = random() % (n + 1);
      std::size_t b = random() % (n + 1);
      if (a > b) std::swap(a, b);
      tree.reverse(a, b);
      std::reverse(reference.begin() + a, reference.begin() + b);
    } else if (op == 3) {
      const std::size_t pos = random() % (n + 1);
      auto right = tree.split(pos);
      tree.check_cache();
      tree.merge(right);
    } else if (op == 4) {
      std::size_t a = random() % (n + 1);
      std::size_t b = random() % (n + 1);
      if (a > b) std::swap(a, b);
      tree.erase(a, b);
      reference.erase(reference.begin() + a, reference.begin() + b);
    } else {
      tree.push_back(static_cast<long long>(step));
      reference.push_back(static_cast<long long>(step));
    }
    tree.check_cache();
    assert(tree.size() == reference.size());
    assert(std::equal(tree.begin(), tree.end(),
                      reference.begin(), reference.end()));
  }
  std::cerr << "child-size cache test passed (capacity=" << Capacity
            << ")\n";
}

signed main() {
  test_child_size_cache<4>();
  test_child_size_cache<8>();
  test_child_size_cache<16>();
  test_child_size_cache<32>();
  std::cerr << "child-size cache tests all passed!\n";
}
