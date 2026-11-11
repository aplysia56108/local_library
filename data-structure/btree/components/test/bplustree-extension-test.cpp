#include "data-structure/btree/components/bplustree-base.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// Traits::Internal::after_sync: derived metadata is refreshed on edits.
// Leaf holds a single value; only Internal holds the aggregate.
struct SumTraits {
  using T = long long;
  static constexpr std::size_t kNodeCapacity = 4;
  struct Node;
  struct Leaf;
  struct Internal;
};

struct SumTraits::Node : Bplustree::Node<SumTraits> {
  using Bplustree::Node<SumTraits>::Node;
};

struct SumTraits::Leaf : Bplustree::Leaf<SumTraits> {
  using Bplustree::Leaf<SumTraits>::Leaf;
};

struct SumTraits::Internal : Bplustree::Internal<SumTraits> {
  using Parent = Bplustree::Internal<SumTraits>;
  using Parent::Parent;

  long long sum = 0;

  void after_sync() {
    sum = 0;
    if (this->level == 0) {
      for (std::size_t i = 0; i < this->count; ++i)
        sum += this->get_leaf(i).get();
    } else {
      for (std::size_t i = 0; i < this->count; ++i)
        sum += this->get_child(i).sum;
    }
  }
};

class SumSequence : public Bplustree::Base<SumTraits> {
  using Parent = Bplustree::Base<SumTraits>;

 public:
  using Parent::Parent;

  long long total() const {
    return Parent::get_node(this->root_).sum;
  }
};

// Protected Base inheritance: only explicitly re-exported APIs are public.
class Ordered : protected Bplustree::Base<Bplustree::Traits<long long, 4>> {
  using Parent = Bplustree::Base<Bplustree::Traits<long long, 4>>;

 public:
  using Parent::begin;
  using Parent::end;
  using Parent::size;

  bool insert(long long x) {
    auto pos = std::lower_bound(begin(), end(), x);
    if (pos != end() && *pos == x) return false;
    Parent::insert(pos, x);
    return true;
  }
};

template <class T>
concept HasPositionalSet = requires(T& tree) { tree.set(0, 1); };
template <class T>
concept HasPublicReverse = requires(T& tree) { tree.reverse(); };

static_assert(!HasPositionalSet<Ordered>);
static_assert(!HasPublicReverse<Ordered>);

void aggregate_extension_test() {
  // Constructor / set / insert / erase must refresh Internal aggregates.
  SumSequence tree{2, 3, 5, 7};
  assert(tree.total() == 17);

  tree.set(1, 10);
  assert(tree.total() == 24);

  tree.insert(2, 11);
  assert(tree.total() == 35);

  tree.erase(0);
  assert(tree.total() == 33);
  assert(std::vector<long long>(tree.begin(), tree.end()) ==
         std::vector<long long>({10, 11, 5, 7}));

  // Structure changes preserve aggregate metadata across node splits.
  tree.push_back(13);
  tree.push_front(1);
  assert(tree.total() == 47);
  tree.pop_back();
  assert(tree.total() == 34);

  std::cerr << "traits aggregate extension tests all passed!\n";
}

void protected_inheritance_test() {
  // Ordered wrapper permits safe insertion without positional edits.
  Ordered ordered;
  assert(ordered.insert(5));
  assert(ordered.insert(1));
  assert(ordered.insert(3));
  assert(!ordered.insert(3));
  assert(std::vector<long long>(ordered.begin(), ordered.end()) ==
         std::vector<long long>({1, 3, 5}));

  std::cerr << "traits protected inheritance tests all passed!\n";
}

int main() {
  aggregate_extension_test();
  protected_inheritance_test();
  std::cerr << "All Traits extension test cases passed!\n";
}
