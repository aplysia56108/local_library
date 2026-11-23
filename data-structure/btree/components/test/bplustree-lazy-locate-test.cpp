#include "data-structure/btree/components/bplustree-base.hpp"

#include <cassert>
#include <iostream>

// Minimal lazy extension: pending increments live in Node metadata.
// locate<false> keeps a pending action; locate<true> materializes the path.
struct LazyTraits {
  using T = long long;
  static constexpr std::size_t kNodeCapacity = 4;
  static constexpr Bplustree::pos_type kNodeHalf = kNodeCapacity / 2;
  static constexpr bool lazy = true;
  using NodeBase = Bplustree::NodeBase<LazyTraits>;
  struct Node;
  struct Leaf;
  struct UpperNode;
  struct BottomNode;
  using UpperCRTP = Bplustree::NodeCRTP<LazyTraits, UpperNode, false>;
  using BottomCRTP = Bplustree::NodeCRTP<LazyTraits, BottomNode, true>;
};
struct LazyTraits::Leaf : Bplustree::Leaf<LazyTraits> {
  using Bplustree::Leaf<LazyTraits>::Leaf;
};
struct LazyTraits::Node : Bplustree::Node<LazyTraits> {
  using Parent = Bplustree::Node<LazyTraits>;
  using Pools = Bplustree::NodePools<LazyTraits>;
  using Parent::Parent;
  long long pending = 0;

  void apply_lazy();
};

struct LazyTraits::UpperNode : Bplustree::UpperNode<LazyTraits> {
  using Parent = Bplustree::UpperNode<LazyTraits>;
  using Parent::Parent;
  void apply_lazy() { LazyTraits::Node::apply_lazy(); }
};

struct LazyTraits::BottomNode : Bplustree::BottomNode<LazyTraits> {
  using Parent = Bplustree::BottomNode<LazyTraits>;
  using Parent::Parent;
  void apply_lazy() { LazyTraits::Node::apply_lazy(); }
};

inline void LazyTraits::Node::apply_lazy() {
  if (pending == 0) return;
  if (this->level == 0) {
    for (std::size_t i = 0; i < this->count; ++i) {
      auto& leaf = Pools::leaves()[this->children[i]];
      leaf.set(leaf.get() + pending);
    }
  } else {
    for (std::size_t i = 0; i < this->count; ++i)
      Pools::with_node(this->children[i], [&](auto& child) {
        child.pending += pending;
      });
  }
  pending = 0;
}
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
  iterator nth_with_lazy(std::size_t i) {
    return this->template locate_iterator<true>(i);
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

  // WithLazy=true returns an iterator with both epochs validated.
  tree.add_all(3);
  auto validated = tree.nth_with_lazy(1);
  assert(*validated == 15);
  tree.add_all(2);  // Pending actions need an explicit epoch invalidation.
  // The custom add_all() does not invalidate epochs, so verify a fresh locate.
  assert(*tree.nth_with_lazy(1) == 17);

  // set() must materialize before writing one element.
  tree.add_all(5);
  tree.set(3, 100);
  assert(tree.get(3) == 100);
  assert(tree.get(0) == 21);
  assert(tree.get(6) == 27);

  std::cerr << "All lazy locate test cases passed!\n";
}
