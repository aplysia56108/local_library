#include "data-structure/btree/components/bplustree-base.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

// Traits::Internal::after_sync: derived metadata is refreshed on edits.
struct SumTraits {
  using T = long long;
  static constexpr std::size_t kNodeCapacity = 4;
  static constexpr bool IsReversible = true;
  static constexpr bool IsMergable = true;
  struct Node;
  struct Leaf;
  struct Internal;
};
struct SumTraits::Node : Bplustree::Node<SumTraits> {
  using Bplustree::Node<SumTraits>::Node;
  long long sum = 0;
};
struct SumTraits::Leaf : Bplustree::Leaf<SumTraits> {
  explicit Leaf(long long x) : Bplustree::Leaf<SumTraits>(x) { this->sum = x; }
  void set(long long x) {
    Bplustree::Leaf<SumTraits>::set(x);
    this->sum = x;
  }
};
struct SumTraits::Internal : Bplustree::Internal<SumTraits> {
  using Bplustree::Internal<SumTraits>::Internal;
  void after_sync() {
    this->sum = 0;
    for (std::size_t i = 0; i < this->count; ++i)
      this->sum += this->children[i]->sum;
  }
};
class SumSequence : public Bplustree::Base<SumTraits> {
  using Parent = Bplustree::Base<SumTraits>;
 public:
  using Parent::Parent;
  long long total() const { return this->root_->sum; }
};

// protected Base inheritance: only explicitly re-exported APIs are public.
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

int main() {
  // Constructor / insert / set / erase must update aggregate ancestors.
  SumSequence tree{2, 3, 5, 7};
  assert(tree.total() == 17);
  tree.set(1, 10);
  assert(tree.total() == 24);
  tree.insert(2, 11);
  assert(tree.total() == 35);
  tree.erase(0);
  assert(tree.total() == 33);

  // Ordered wrapper permits safe insertion without exposing positional edits.
  Ordered ordered;
  assert(ordered.insert(5));
  assert(ordered.insert(1));
  assert(ordered.insert(3));
  assert(!ordered.insert(3));
  assert(std::vector<long long>(ordered.begin(), ordered.end()) ==
         std::vector<long long>({1, 3, 5}));
}
