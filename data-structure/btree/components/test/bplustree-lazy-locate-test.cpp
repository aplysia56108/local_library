#include "data-structure/btree/components/bplustree-base.hpp"

#include <cassert>
#include <iostream>

// Minimal lazy extension: pending increments live in Internal nodes.
// locate<false> keeps a pending action; locate<true> materializes the path.
struct LazyTraits {
  using T = long long;
  static constexpr std::size_t kNodeCapacity = 4;
  static constexpr bool IsReversible = true;
  static constexpr bool IsMergable = true;
  using Node = Bplustree::Node<LazyTraits>;
  struct Leaf;
  struct Internal;
};
struct LazyTraits::Leaf : Bplustree::Leaf<LazyTraits> {
  using Bplustree::Leaf<LazyTraits>::Leaf;
};
struct LazyTraits::Internal : Bplustree::Internal<LazyTraits> {
  using Bplustree::Internal<LazyTraits>::Internal;
  long long pending = 0;
  void apply_lazy() {
    if (pending == 0) return;

    if (this->level == 0) {
      for (std::size_t i = 0; i < this->count; ++i) {
        auto& leaf = this->get_leaf(i);
        leaf.set(leaf.get() + pending);
      }
    } else {
      for (std::size_t i = 0; i < this->count; ++i)
        this->get_child(i).pending += pending;
    }

    pending = 0;
  }
};
class LazySequence : public Bplustree::Base<LazyTraits> {
  using Parent = Bplustree::Base<LazyTraits>;

 public:
  using Parent::Parent;
  void add_all(long long x) {
    Parent::get_node(this->root_).pending += x;
  }
  long long pending() const {
    return Parent::get_node(this->root_).pending;
  }
  long long inspect_without_lazy(std::size_t i) {
    return Parent::get_leaf(this->template locate<false>(i)).get();
  }
  long long inspect_with_lazy(std::size_t i) {
    return Parent::get_leaf(this->template locate<true>(i)).get();
  }
};

int main() {
  LazySequence tree{1, 2, 3, 4, 5, 6, 7};
  // locate<false>: no lazy application, locate<true>: materialize path.
  tree.add_all(10);
  assert(tree.inspect_without_lazy(0) == 1);
  assert(tree.pending() == 10);
  assert(tree.inspect_with_lazy(0) == 11);
  assert(tree.pending() == 0);

  // Iterator::operator* and get() traverse the lazy path.
  assert(*tree.nth(2) == 13);
  assert(tree.get(6) == 17);

  // set() must materialize before writing one element.
  tree.add_all(5);
  tree.set(3, 100);
  assert(tree.get(3) == 100);
  assert(tree.get(0) == 16);
  assert(tree.get(6) == 22);

  std::cerr << "All lazy locate test cases passed!\n";
}
