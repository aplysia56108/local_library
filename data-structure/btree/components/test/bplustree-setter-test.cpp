#include "data-structure/btree/components/bplustree-base.hpp"

#include <cassert>
#include <iostream>
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

  std::cerr << "setter arithmetic tests all passed!\n";
}


// Return types, chaining, and copyable proxy semantics.
void returning_setter() {
  static_assert(std::copy_constructible<Setter>);
  static_assert(std::move_constructible<Setter>);
  static_assert(std::same_as<decltype(std::declval<Setter>() = 1LL), Setter>);
  static_assert(std::same_as<decltype(std::declval<Setter>() = std::declval<const Setter&>()), Setter>);
  static_assert(std::same_as<decltype(std::declval<Setter>() += 1LL), Setter>);
  static_assert(std::same_as<decltype(std::declval<Setter&>() += 1LL), Setter>);
  static_assert(std::same_as<decltype(++std::declval<Setter>()), Setter>);
  static_assert(std::same_as<decltype(--std::declval<Setter>()), Setter>);
  static_assert(std::same_as<decltype(std::declval<Setter>()++), long long>);
  static_assert(std::same_as<decltype(std::declval<Setter>()--), long long>);

  Tree tree{10, 20};
  auto a = tree(0);
  auto b = a;  // Copy construction keeps the same element.
  b += 5;
  assert(a.val() == 15);
  assert(tree[0] == 15);

  auto assigned = (tree(0) = 7);
  assert(assigned.val() == 7);
  auto copied_assignment = (tree(0) = tree(1));
  assert(copied_assignment.val() == 20);
  assert(tree[0] == 20);

  auto updated = ((tree(0) += 3) *= 2);
  assert(updated.val() == 46);
  assert(tree[0] == 46);
  (a -= 6) /= 2;
  assert(tree[0] == 20);
  assert(b.val() == 20);

  // The assignment of one proxy to another writes the referenced value.
  b = tree(1);
  assert(tree[0] == 20);
  tree(1) = 42;
  assert(b.val() == 20);

  std::cerr << "setter return/copy tests all passed!\n";
}

// Prefix operations return Setter; postfix operations return the previous value.
void increment_decrement_setter() {
  Tree tree{10};
  auto setter = tree(0);

  auto a = ++setter;
  assert(a.val() == 11);
  assert(tree[0] == 11);
  auto b = setter++;
  assert(b == 11);
  assert(tree[0] == 12);

  auto c = --setter;
  assert(c.val() == 11);
  assert(tree[0] == 11);
  auto d = setter--;
  assert(d == 11);
  assert(tree[0] == 10);

  auto e = ++tree(0);
  assert(e.val() == 11);
  assert(tree[0] == 11);
  assert(tree(0)++ == 11);
  assert(tree[0] == 12);
  auto f = --tree(0);
  assert(f.val() == 11);
  assert(tree(0)-- == 11);
  assert(tree[0] == 10);

  ++(++tree(0));
  assert(tree[0] == 12);
  --(--tree(0));
  assert(tree[0] == 10);

  std::cerr << "setter increment/decrement tests all passed!\n";
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
  using iterator = Iterator;
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

  std::cerr << "setter monoid/action tests all passed!\n";
}

int main() {
  arithmetic_setter();
  returning_setter();
  increment_decrement_setter();
  monoid_action_setter();

  std::cerr << "All setter test cases passed!\n";
}
