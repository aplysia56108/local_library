#pragma once

#include <concepts>
#include <cstddef>
#include <utility>

namespace Bplustree {
template <class Tree>
concept Has_Monoid_op =
    requires(typename Tree::value_type a, typename Tree::value_type b) {
      Tree::Monoid::op(a, b);
    };

template <class Tree, class F, class value_type>
concept Has_Action_mapping =
    requires(const F& f, const value_type& x) { Tree::Action::mapping(f, x); };
// One-shot point-update proxy. Derived containers use Setter<Derived, T>.
template <class Tree>
class Setter {
  using io_via_val = void;

 protected:
  using Iterator = typename Tree::iterator;
  Iterator it_;
  using value_type = typename Iterator::value_type;

 public:
  Setter(Iterator it) : it_(it) {}
  Setter(typename Tree::Leaf* leaf) : it_(leaf) {}
  Setter(const Setter&) = default;
  Setter(Setter&&) = default;

  void operator=(const value_type& value)
    requires requires(Iterator it, const value_type& value) { it.set(value); }
  {
    it_.set(value);
  }
  void operator=(const Setter& other)
    requires requires(Iterator it, const value_type& value) { it.set(value); }
  {
    it_.set(other.val());
  }
  value_type val() const
    requires requires(Iterator it) {
      { *it } -> std::convertible_to<value_type>;
    }
  {
    return *it_;
  }
  void prod(const value_type& value)
    requires Has_Monoid_op<Tree>
  {
    prod_right(value);
  }
  void prod_right(const value_type& value)
    requires Has_Monoid_op<Tree>
  {
    it_.set(Tree::Monoid::op(*it_, value));
  }
  void prod_left(const value_type& value)
    requires Has_Monoid_op<Tree>
  {
    it_.set(Tree::Monoid::op(value, *it_));
  }

  template <class F>
  void apply(const F& action)
    requires Has_Action_mapping<Tree, F, value_type>
  {
    it_.set(Tree::Action::mapping(action, *it_));
  }

#define BPLUSTREE_SETTER_COMPOUND(OP)                                \
  void operator OP(const value_type& value)                          \
    requires requires(Iterator it, value_type value, value_type x) { \
      x = *it;                                                       \
      x OP value;                                                    \
      it.set(x);                                                     \
    }                                                                \
  {                                                                  \
    value_type x = *it_;                                             \
    x OP value;                                                      \
    it_.set(x);                                                      \
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
