#pragma once

#include <concepts>
#include <cstddef>
#include <utility>

namespace Bplustree {
/** @brief Tree の value_type に Monoid::op(a,b) が定義されていること。 */
template <class Tree>
concept Has_Monoid_op =
    requires(typename Tree::value_type a, typename Tree::value_type b) {
      Tree::Monoid::op(a, b);
    };

/**
 * @brief Tree の Action が単点への mapping(f,x) または mapping(f,x,1) に対応すること。
 */
template <class Tree, class F, class value_type>
concept Has_Action_mapping = requires(const F& f, const value_type& x) {
  Tree::Action::mapping(f, x);
} || requires(const F& f, const value_type& x) {
  Tree::Action::mapping(f, x, std::size_t{1});
};

/**
 * @brief 1要素を更新するための、Iterator を保持するコピー可能な Proxy。
 * @tparam Tree 元の B+tree 型。iterator・Monoid・Action の有無を参照する。
 * @details
 * 書き換え可能な iterator をコピーして保持し、代入と複合代入、前置 ++/--、
 * モノイド更新、遅延作用を（利用可能な場合のみ）提供する。
 * 実要素 Leaf が削除された場合、この Proxy も失効する。
 * operator=(value) / 複合代入 / 前置 ++/-- は Setter の値を返す。
 * 後置 ++/-- は更新前の value_type を返す。
 * ただし prod / prod_right / prod_left / apply は void の更新操作。
 */
template <class Tree>
class Setter {
  using io_via_val = void;

 protected:
  using Iterator = typename Tree::iterator;
  Iterator it_;
  using value_type = typename Iterator::value_type;

 public:
  /** @brief 更新する要素を指す Iterator をコピーして保持する。 */
  Setter(Iterator it) : it_(it) {}
  // Setter(typename Tree::Leaf* leaf) : it_(leaf) {}
  Setter(const Setter&) = default;
  Setter(Setter&&) = default;

  /** @brief value を直接代入し、祖先の集約情報を更新する。 */
  Setter operator=(const value_type& value)
    requires requires(Iterator it, const value_type& value) { it.set(value); }
  {
    it_.set(value);
    return *this;
  }
  /** @brief other が指す要素の値を読み取り、自分の要素へ代入する。 */
  Setter operator=(const Setter& other)
    requires requires(Iterator it, const value_type& value) { it.set(value); }
  {
    it_.set(other.val());
    return *this;
  }
  /** @brief 現在の要素の値を値としてコピーして返す。 */
  value_type val() const
    requires requires(Iterator it) {
      { *it } -> std::convertible_to<value_type>;
    }
  {
    return *it_;
  }
  /** @brief Monoid::op(現在値, value) を点更新する。戻り値は void。 */
  void prod(const value_type& value)
    requires Has_Monoid_op<Tree>
  {
    prod_right(value);
  }
  /** @brief 右側からモノイド演算を適用（op(現在値, value)）。 */
  void prod_right(const value_type& value)
    requires Has_Monoid_op<Tree>
  {
    it_.set(Tree::Monoid::op(*it_, value));
  }
  /** @brief 左側からモノイド演算を適用（op(value, 現在値)）。 */
  void prod_left(const value_type& value)
    requires Has_Monoid_op<Tree>
  {
    it_.set(Tree::Monoid::op(value, *it_));
  }

  /** @brief Action::mapping によって1要素へ作用を適用する。 */
  template <class F>
  void apply(const F& action)
    requires Has_Action_mapping<Tree, F, value_type>
  {
    if constexpr (requires { Tree::Action::mapping(action, *it_); })
      it_.set(Tree::Action::mapping(action, *it_));
    else
      it_.set(Tree::Action::mapping(action, *it_, std::size_t{1}));
  }

#define BPLUSTREE_SETTER_COMPOUND(OP)              \
  Setter operator OP(const value_type& value)      \
    requires requires(Iterator it, value_type x) { \
      x = *it;                                     \
      x OP value;                                  \
      it.set(x);                                   \
    }                                              \
  {                                                \
    value_type x = *it_;                           \
    x OP value;                                    \
    it_.set(x);                                    \
    return *this;                                  \
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

  // 前置は更新後の Setter を返し、後置は更新前の値（value_type）を返す。
#define BPLUSTREE_SETTER_INCDEC(OP)                \
  Setter operator OP()                             \
    requires requires(Iterator it, value_type x) { \
      x = *it;                                     \
      OP x;                                        \
      it.set(x);                                   \
    }                                              \
  {                                                \
    value_type x = *it_;                           \
    OP x;                                          \
    it_.set(x);                                    \
    return *this;                                  \
  }                                                \
                                                   \
  value_type operator OP(signed)                   \
    requires requires(Iterator it, value_type x) { \
      x = *it;                                     \
      OP x;                                        \
      it.set(x);                                   \
    }                                              \
  {                                                \
    value_type old = *it_;                         \
    value_type x = old;                            \
    OP x;                                          \
    it_.set(x);                                    \
    return old;                                    \
  }
  BPLUSTREE_SETTER_INCDEC(++)
  BPLUSTREE_SETTER_INCDEC(--)
#undef BPLUSTREE_SETTER_INCDEC
};
}  // namespace Bplustree
