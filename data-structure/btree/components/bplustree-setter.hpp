#pragma once

#include <concepts>
#include <cstddef>
#include <utility>

namespace Bplustree {

// One-shot point-update proxy. Derived containers use Setter<Derived, T>.
template <class Tree, class Value = typename Tree::value_type>
class Setter {
 protected:
  Tree* tree_;
  std::size_t index_;
 public:
  Setter(Tree& tree, std::size_t index) : tree_(&tree), index_(index) {}
  Setter(Tree* tree, std::size_t index) : tree_(tree), index_(index) {}
  Setter(const Setter&) = delete;
  Setter& operator=(const Setter&) = delete;
  Setter(Setter&&) = delete;
  Setter& operator=(Setter&&) = delete;
  void operator=(const Value& value) &&
    requires requires(Tree& tree) { tree.set(index_, value); }
  { tree_->set(index_, value); }
  Value val() const
    requires requires(const Tree& tree) { { tree.get(index_) } -> std::convertible_to<Value>; }
  { return tree_->get(index_); }
  void prod(const Value& value) &&
    requires requires(Tree& tree) { tree.prod_right(index_, value); }
  { tree_->prod_right(index_, value); }
  void prod_right(const Value& value) &&
    requires requires(Tree& tree) { tree.prod_right(index_, value); }
  { tree_->prod_right(index_, value); }
  void prod_left(const Value& value) &&
    requires requires(Tree& tree) { tree.prod_left(index_, value); }
  { tree_->prod_left(index_, value); }
  template <class Action>
  void apply(Action&& action) &&
    requires requires(Tree& tree) { tree.apply(index_, std::forward<Action>(action)); }
  { tree_->apply(index_, std::forward<Action>(action)); }
  template <class Action>
  void apply_left(Action&& action) &&
    requires requires(Tree& tree) { tree.apply_left(index_, std::forward<Action>(action)); }
  { tree_->apply_left(index_, std::forward<Action>(action)); }

#define BPLUSTREE_SETTER_COMPOUND(OP)                                     \
  void operator OP(const Value& value) &&                                \
    requires requires(Tree& tree, Value x) {                             \
      { tree.get(index_) } -> std::convertible_to<Value>;                 \
      x OP value;                                                       \
      tree.set(index_, x);                                              \
    }                                                                   \
  {                                                                     \
    Value x = tree_->get(index_);                                        \
    x OP value;                                                         \
    tree_->set(index_, x);                                               \
  }
  BPLUSTREE_SETTER_COMPOUND(+=)
  BPLUSTREE_SETTER_COMPOUND(-=)
  BPLUSTREE_SETTER_COMPOUND(*=)
  BPLUSTREE_SETTER_COMPOUND(/=)
  BPLUSTREE_SETTER_COMPOUND(%=)
  BPLUSTREE_SETTER_COMPOUND(&=)
  BPLUSTREE_SETTER_COMPOUND(|=)
  BPLUSTREE_SETTER_COMPOUND(^=)
  BPLUSTREE_SETTER_COMPOUND(<<=)
  BPLUSTREE_SETTER_COMPOUND(>>=)
#undef BPLUSTREE_SETTER_COMPOUND
};
}  // namespace Bplustree
