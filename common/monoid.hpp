#pragma once

#include <algorithm>
#include <numeric>

#include "data-structure/identity-value/identity-value.hpp"
#include "common/common.hpp"

struct SumMonoid {
  using value_type = long long;

  static value_type op(const value_type& lhs, const value_type& rhs) {
    return lhs + rhs;
  }

  static value_type e() {
    return 0;
  }
};

struct MinMonoid {
  using value_type = long long;

  static value_type op(const value_type& lhs, const value_type& rhs) {
    return std::min(lhs, rhs);
  }

  static value_type e() {
    return library_inf;
  }
};

struct MaxMonoid {
  using value_type = long long;

  static value_type op(const value_type& lhs, const value_type& rhs) {
    return std::max(lhs, rhs);
  }

  static value_type e() {
    return -library_inf;
  }
};

struct GcdMonoid {
  using value_type = long long;

  static value_type op(const value_type& lhs, const value_type& rhs) {
    return std::gcd(lhs, rhs);
  }

  static value_type e() {
    return 0;
  }
};

template <class mint>
struct AffineMonoid {
  using value_type = std::pair<mint, mint>;

  static value_type op(value_type a, value_type b) {
    return {a.first * b.first, a.second + a.first * b.second};
  }

  static value_type e() {
    return {1, 0};
  }
};

/**
 * @brief 関数をテンプレート引数として指定するモノイド定義。
 * @tparam T 値型。
 * @tparam Op 結合則を満たす二項演算。
 * @tparam E 単位元を返す引数なし関数。
 */
template <class T, auto Op, auto E>
struct Monoid {
  using value_type = T;

  static value_type op(const value_type& a, const value_type& b) {
    return Op(a, b);
  }

  static value_type e() {
    return E();
  }
};

/**
 * @brief 単位元を明示せずに使用するモノイド定義。
 * @tparam T 値型。
 * @tparam Op 結合則を満たす二項演算。
 * @details
 * `IdentityValue<T>` に人工的な単位元を追加してモノイド化する。
 *
 * 格納する各要素には通常 `T` の実値を与えて使用することを想定する。
 * 人工的な単位元を実値として取り出す操作はできない。
 */
template <class T, auto Op>
struct MonoidWithoutE {
  using value_type = IdentityValue<T>;

  static value_type op(const value_type& a, const value_type& b) {
    if (!a.has_value()) return b;
    if (!b.has_value()) return a;
    return Op(a.val(), b.val());
  }

  static value_type e() {
    return {};
  }
};