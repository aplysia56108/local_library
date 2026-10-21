#pragma once

#include <cstddef>
#include <utility>

/**
 * @brief 1要素を更新するための一時的な Proxy。
 * @details
 * `operator()(index)`
 * から取得し、代入またはモノイド演算による点更新に使用する。
 *
 * コピー・ムーブはできず、一時オブジェクトとしてのみ使用できる。
 *
 * @code
 * seg(i) = value;
 * seg(i) += value;
 * seg(i).apply(value);
 * seg(i).apply_left(value);
 * @endcode
 */
template <class Tree, class value_type>
class SegtreeSetter {
  friend Tree;

 protected:
  Tree* tree_;
  std::size_t index_;

  SegtreeSetter(Tree* tree, std::size_t index) : tree_(tree), index_(index) {}

 public:
  SegtreeSetter(const SegtreeSetter&) = delete;
  SegtreeSetter& operator=(const SegtreeSetter&) = delete;
  SegtreeSetter(SegtreeSetter&&) = delete;
  SegtreeSetter& operator=(SegtreeSetter&&) = delete;

  /**
   * @brief index 番目の要素を `value` に置き換える。
   * @param value 新しい値。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void operator=(const value_type& value) && {
    tree_->set(index_, value);
  }

  /**
   * @brief index 番目の要素の右側から `value` を作用させる。
   * @param value 作用させる値。
   * @details
   * 現在値を `x` とすると、`x = Monoid::op(x, value)` と更新する。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void prod(const value_type& value) && {
    tree_->prod_right(index_, value);
  }

  /**
   * @brief index 番目の要素の右側から `value` を作用させる。
   * @param value 作用させる値。
   * @details
   * 現在値を `x` とすると、`x = Monoid::op(x, value)` と更新する。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void prod_right(const value_type& value) && {
    tree_->prod_right(index_, value);
  }

  /**
   * @brief index 番目の要素の左側から `value` を作用させる。
   * @param value 作用させる値。
   * @details
   * 現在値を `x` とすると、`x = Monoid::op(value, x)` と更新する。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void prod_left(const value_type& value) && {
    tree_->prod_left(index_, value);
  }

#define SETTER_COMPOUND(OP)                                             \
  void operator OP(const value_type& value) &&                          \
    requires requires(value_type x, const value_type& y) { x OP y; } && \
             requires(Tree& t, std::size_t i, value_type x) {           \
               t.get(i);                                                \
               t.set(i, x);                                             \
             }                                                          \
  {                                                                     \
    value_type x = tree_->get(index_);                                  \
    x OP value;                                                         \
    tree_->set(index_, x);                                              \
  }

  SETTER_COMPOUND(+=)
  SETTER_COMPOUND(-=)
  SETTER_COMPOUND(*=)
  SETTER_COMPOUND(/=)
  SETTER_COMPOUND(%=)
  SETTER_COMPOUND(&=)
  SETTER_COMPOUND(|=)
  SETTER_COMPOUND(^=)
  SETTER_COMPOUND(<<=)
  SETTER_COMPOUND(>>=)

#undef SETTER_COMPOUND
};