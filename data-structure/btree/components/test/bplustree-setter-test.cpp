#include "data-structure/btree/components/bplustree-base.hpp"

#include <cassert>
#include <concepts>
#include <string>
#include <type_traits>
#include <utility>

using Tree = Bplustree::Base<Bplustree::Traits<long long, 4>>;
using Setter = Tree::setter_type;
static_assert(!std::default_initializable<Setter>);

// Setter: assignment, val and all supported compound assignment operators.
void arithmetic_setter() {
  Tree tree{12};
  tree(0) = 20;
  assert(tree(0).val() == 20);
  tree(0) += 5;
  tree(0) -= 2;
  tree(0) *= 3;
  tree(0) /= 2;
  tree(0) %= 10;
  assert(tree[0] == 4);
  tree(0) |= 8;
  tree(0) &= 14;
  tree(0) ^= 2;
  tree(0) <<= 1;
  tree(0) >>= 1;
  assert(tree[0] == 14);
  tree(0) = tree(0);
  assert(tree[0] == 14);
}

// Monoid / Action extension detection and left/right operation order.
struct ConcatMonoid {
  static std::string op(const std::string& a, const std::string& b) {
    return a + b;
  }
};
struct PrefixAction {
  static std::string mapping(const std::string& prefix, const std::string& s) {
    return prefix + s;
  }
};
struct ExtendedTree {
  using value_type = std::string;
  using Monoid = ConcatMonoid;
  using Action = PrefixAction;
  struct Iterator {
    using value_type = std::string;
    ExtendedTree* tree;
    const std::string& operator*() const { return tree->value; }
    void set(const std::string& v) const { tree->value = v; }
  };
  std::string value;
  Bplustree::Setter<ExtendedTree> operator()() { return {Iterator{this}}; }
};
template <class S>
concept CanApply = requires(S s) { s.apply(std::string{"prefix"}); };
static_assert(CanApply<Bplustree::Setter<ExtendedTree>>);
static_assert(!CanApply<Setter>);

void monoid_action_setter() {
  ExtendedTree tree;
  tree() = "b";
  tree().prod_right("c");
  tree().prod_left("a");
  tree().prod("d");
  assert(tree.value == "abcd");
  tree().apply("!");
  assert(tree.value == "!abcd");
}

int main() {
  arithmetic_setter();
  monoid_action_setter();
}
