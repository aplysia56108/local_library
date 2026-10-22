#include "data-structure/btree/components/bplustree-base.hpp"

#include <cassert>
#include <numeric>
#include <random>
#include <string>

struct SumTraits {
  using T = long long;
  static constexpr std::size_t kNodeCapacity = 4;
  struct Node;
  struct Leaf;
  struct Internal;
};
struct SumTraits::Node : Bplustree::Node<SumTraits> {
  using Bplustree::Node<SumTraits>::Node;
  long long sum = 0, pending = 0;
  void add(long long delta) { pending += delta; sum += delta * size; }
};
struct SumTraits::Leaf : Bplustree::Leaf<SumTraits> {
  void after_sync() {
    sum = std::accumulate(values.begin(), values.begin() + count, 0LL) + pending * size;
  }
  void apply_lazy() {
    for (std::size_t i = 0; i < count; ++i) values[i] += pending;
    pending = 0;
  }
};
struct SumTraits::Internal : Bplustree::Internal<SumTraits> {
  void after_sync() {
    sum = pending * size;
    for (std::size_t i = 0; i < count; ++i) sum += children[i]->sum;
  }
  void apply_lazy() {
    for (std::size_t i = 0; i < count; ++i) children[i]->add(pending);
    pending = 0;
  }
};

class SumSequence : public Bplustree::Base<SumTraits, true> {
  using Parent = Bplustree::Base<SumTraits, true>;
 public:
  using Parent::Parent;
  explicit SumSequence(Parent&& tree) : Parent(std::move(tree)) {}
  using setter_type = Bplustree::Setter<SumSequence>;
  setter_type operator()(std::size_t index) { assert(index < size()); return {*this, index}; }
  long long sum() const { return root_->sum; }
  void add_all(long long delta) { root_->add(delta); }
  void apply(std::size_t index, long long delta) { set(index, get(index) + delta); }
  void apply(std::size_t first, std::size_t last, long long delta) {
    SumSequence right(Parent::split(last)), middle(Parent::split(first));
    middle.add_all(delta);
    Parent::merge(middle); Parent::merge(right);
  }
};

// An ordered wrapper deliberately exposes no positional writes or reversal.
class OrderedSet : protected Bplustree::Base<Bplustree::Traits<int, 3>, false> {
  using Parent = Bplustree::Base<Bplustree::Traits<int, 3>, false>;
 public:
  using Parent::begin;
  using Parent::end;
  using Parent::size;
  bool insert(int value) {
    auto it = std::lower_bound(begin(), end(), value);
    if (it != end() && *it == value) return false;
    Parent::insert(it, value); return true;
  }
};
template <class T>
concept WritableByIndex = requires(T& tree) { tree.set(0, 1); };
static_assert(!WritableByIndex<OrderedSet>);

struct ConcatTraits {
  using T = std::string;
  static constexpr std::size_t kNodeCapacity = 3;
  struct Node;
  struct Leaf;
  struct Internal;
};
struct ConcatTraits::Node : Bplustree::Node<ConcatTraits> {
  using Bplustree::Node<ConcatTraits>::Node;
  std::string forward, backward;
};
struct ConcatTraits::Leaf : Bplustree::Leaf<ConcatTraits> {
  void after_sync() {
    forward.clear(); backward.clear();
    for (std::size_t i = 0; i < count; ++i) forward += values[i];
    for (std::size_t i = count; i > 0; --i) backward += values[i - 1];
  }
  void push_reverse() {
    Bplustree::Leaf<ConcatTraits>::push_reverse();
    std::swap(forward, backward);
  }
};
struct ConcatTraits::Internal : Bplustree::Internal<ConcatTraits> {
  void after_sync() {
    forward.clear(); backward.clear();
    for (std::size_t i = 0; i < count; ++i) forward += children[i]->forward;
    for (std::size_t i = count; i > 0; --i) backward += children[i - 1]->backward;
  }
  void push_reverse() {
    Bplustree::Internal<ConcatTraits>::push_reverse();
    std::swap(forward, backward);
  }
};
class ConcatSequence : public Bplustree::Base<ConcatTraits, true> {
  using Parent = Bplustree::Base<ConcatTraits, true>;
 public:
  using Parent::Parent;
  std::string prod() const { return root_->forward; }
};

int main() {
  OrderedSet ordered;
  for (int value : {5, 1, 3, 1, 2, 5}) ordered.insert(value);
  assert(std::vector<int>(ordered.begin(), ordered.end()) == std::vector<int>({1, 2, 3, 5}));

  ConcatSequence strings{"a", "bb", "c", "ddd", "e", "f", "gg"};
  assert(strings.prod() == "abbcdddefgg");
  strings.reverse();
  assert(strings.prod() == "ggfedddcbba");
  strings.reverse(1, 5);
  assert(strings.prod() == "ggcdddefbba");
  strings(2) = "X";
  assert(strings.prod() == "ggcXefbba");
  strings.insert(1, "Z");
  assert(strings.prod() == "ggZcXefbba");
  strings.erase(2);
  assert(strings.prod() == "ggZXefbba");

  std::vector<long long> oracle(100);
  std::iota(oracle.begin(), oracle.end(), 0);
  SumSequence tree(oracle.begin(), oracle.end());
  std::mt19937 rng(22);
  for (int step = 0; step < 1400; ++step) {
    std::size_t first = rng() % (oracle.size() + 1);
    std::size_t last = first + rng() % (oracle.size() - first + 1);
    long long value = rng() % 20;
    switch (rng() % 8) {
      case 0:
        tree.apply(first, last, value);
        for (std::size_t i = first; i < last; ++i) oracle[i] += value;
        break;
      case 1:
        tree.reverse(first, last);
        std::reverse(oracle.begin() + first, oracle.begin() + last);
        break;
      case 2:
        tree.insert(first, value); oracle.insert(oracle.begin() + first, value); break;
      case 3:
        if (first < oracle.size()) { tree(first) = value; oracle[first] = value; }
        break;
      case 4:
        if (first < oracle.size()) { tree(first).apply(value); oracle[first] += value; }
        break;
      case 5:
        tree.add_all(value); for (auto& x : oracle) x += value; break;
      case 6:
        if (first < oracle.size()) { tree.erase(first); oracle.erase(oracle.begin() + first); }
        break;
      case 7:
        if (first < oracle.size()) { tree(first) += value; oracle[first] += value; }
        break;
    }
    assert(tree.sum() == std::accumulate(oracle.begin(), oracle.end(), 0LL));
    assert(std::vector<long long>(tree.begin(), tree.end()) == oracle);
    // Reading/materializing nodes must preserve the aggregate too.
    assert(tree.sum() == std::accumulate(oracle.begin(), oracle.end(), 0LL));
  }
}
