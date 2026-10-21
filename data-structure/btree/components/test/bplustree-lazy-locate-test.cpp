#include "data-structure/btree/components/bplustree-node.hpp"
#include "data-structure/btree/components/bplustree-iterator.hpp"

#include <cassert>

// Exercise the Traits extension points with pending additions at multiple levels.
struct LazyTraits {
  using T = int;
  static constexpr std::size_t kNodeCapacity = 4;
  using Node = Bplustree::Node<LazyTraits>;
  struct Leaf;
  struct Internal;
};

struct LazyTraits::Leaf : Bplustree::Leaf<LazyTraits> {
  int pending = 0;
  void apply_lazy() {
    for (std::size_t i = 0; i < count; ++i) values[i] += pending;
    pending = 0;
  }
};

struct LazyTraits::Internal : Bplustree::Internal<LazyTraits> {
  int pending = 0;
  void apply_lazy() {
    for (std::size_t i = 0; i < count; ++i) {
      if (children[i]->is_leaf())
        static_cast<Leaf*>(children[i].get())->pending += pending;
      else
        static_cast<Internal*>(children[i].get())->pending += pending;
    }
    pending = 0;
  }
};

int main() {
  using Node = LazyTraits::Node;
  using Leaf = LazyTraits::Leaf;
  using Internal = LazyTraits::Internal;
  Internal root;
  for (int i = 0; i < 2; ++i) {
    auto branch = std::make_unique<Internal>();
    auto leaf = std::make_unique<Leaf>();
    leaf->insert(i, 0);
    branch->insert(std::move(leaf), 0);
    root.insert(std::move(branch), i);
  }
  Node* node = &root;
  root.pending = 10;
  auto [leaf, index] = node->locate(0);
  assert(leaf->values[index] == 0 && root.pending == 10);
  auto location = node->locate_with_lazy(0);
  assert(location.first->values[location.second] == 10);
  assert(root.pending == 0);
  auto [last, end_index] = node->locate_with_lazy(root.size);
  assert(end_index == 1 && last->values[0] == 11);

  using It = Iterator<Node, Leaf, Internal, int, false>;
  using ConstIt = Iterator<Node, Leaf, Internal, int, true>;
  It begin(leaf, 0);
  ConstIt last_it(last, 0);
  root.pending = 7;
  assert(*begin == 17);  // Dereference must apply pending ancestor updates too.
  assert(*last_it == 18);
  *begin = 42;
  assert(*begin == 42);
}
