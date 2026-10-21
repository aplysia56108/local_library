#pragma once

#include <vector>
#include <cassert>

#include "utility/random.hpp"

/**
 * @brief ローリングハッシュのモノイド
 *
 * 法は 2^61-1、基数は実行時にランダム生成する。
 * op(a, b) は a と b の連結を表す。
 *
 * ### 計算量
 * - op: O(1) 償却（累乗テーブルの拡張を除く）
 * - e: O(1)
 */
template <class T = char>
struct RollingHashMonoid {
  static constexpr unsigned long long mod = (1ULL << 61) - 1;

  /** @brief ハッシュ値と列の長さ */
  struct value_type {
    using io_via_val = void;
    unsigned long long hash_1 = 0;
    unsigned long long hash_2 = 0;
    long long length = 0;

    value_type() = default;

    /** @brief 1要素のハッシュを生成する */
    explicit value_type(long long x)
        : hash_1(normalize(x)), hash_2(normalize(x)), length(1) {}

    bool operator==(const value_type&) const = default;

    value_type(unsigned long long h1, unsigned long long h2, long long len)
        : hash_1(h1), hash_2(h2), length(len) {}

    T val() const {
      assert(length == 1);
      return static_cast<T>(hash_1);
    }
  };

  /** @brief 2^61-1 を法とする乗算 */
  static unsigned long long mul(unsigned long long a, unsigned long long b) {
    unsigned __int128 x = (unsigned __int128)a * b;
    unsigned long long res =
        (unsigned long long)(x >> 61) + ((unsigned long long)x & mod);
    if (res >= mod) res -= mod;
    return res;
  }

  /** @brief 整数をハッシュの値域に正規化する */
  static unsigned long long normalize(long long x) {
    long long r = x % (long long)mod;
    if (r < 0) r += (long long)mod;
    return (unsigned long long)r;
  }

  /** @brief 2つのハッシュを連結する */
  static value_type op(const value_type& a, const value_type& b) {
    ensure_pow(b.length);
    unsigned long long h_1 = mul(a.hash_1, pow_base_1[b.length]) + b.hash_1;
    unsigned long long h_2 = mul(a.hash_2, pow_base_2[b.length]) + b.hash_2;
    if (h_1 >= mod) h_1 -= mod;
    if (h_2 >= mod) h_2 -= mod;
    return {h_1, h_2, a.length + b.length};
  }

  /** @brief 空列のハッシュ */
  static value_type e() {
    return {};
  }

  /** @brief 2つの累積ハッシュから区間ハッシュを取得する */
  static value_type subtract_prefix(const value_type& l, const value_type& r) {
    assert(l.length <= r.length);
    long long len = r.length - l.length;
    ensure_pow(len);

    unsigned long long h1 = r.hash_1 + mod - mul(l.hash_1, pow_base_1[len]);
    if (h1 >= mod) h1 -= mod;
    unsigned long long h2 = r.hash_2 + mod - mul(l.hash_2, pow_base_2[len]);
    if (h2 >= mod) h2 -= mod;

    return {h1, h2, len};
  }

  /** @brief 基数の累乗を n 次まで事前計算する */
  static void reserve_pow(long long n) {
    ensure_pow(n);
  }

 private:
  static inline const unsigned long long base_1 = rnd(1ULL << 20, mod - 2);
  static inline const unsigned long long base_2 = rnd(1ULL << 20, mod - 2);

  static inline std::vector<unsigned long long> pow_base_1 = {1};
  static inline std::vector<unsigned long long> pow_base_2 = {1};

  static void ensure_pow(long long n) {
    while ((long long)pow_base_1.size() <= n) {
      pow_base_1.push_back(mul(pow_base_1.back(), base_1));
      pow_base_2.push_back(mul(pow_base_2.back(), base_2));
    }
  }
};

template <class T = char>
inline auto make_rolling_hash_value(long long x) {
  return typename RollingHashMonoid<T>::value_type(x);
}
