#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "string/rolling_hash_monoid.hpp"

/**
 * @brief 累積ハッシュによる静的ローリングハッシュ
 *
 * RollingHashMonoid と共通の基数・ハッシュ値を使用する。
 * 区間 [l, r) のハッシュを O(1) で取得できる。
 *
 * ### 計算量
 * - 構築: O(N)
 * - prod: O(1)
 * - same: O(1)
 * - 空間: O(N)
 */
template <class T = char>
class RollingHash {
 public:
  using M = RollingHashMonoid<T>;
  using value_type = M::value_type;

 private:
  std::vector<value_type> prefix_;

 public:
  RollingHash() : prefix_(1, M::e()) {}

  /** @brief 文字列から構築する */
  explicit RollingHash(const std::string& s) {
    build(s);
  }

  /** @brief 整数列などから構築する */
  template <class V>
  explicit RollingHash(const std::vector<V>& v) {
    build(v);
  }

  /** @brief イテレータ範囲から構築する */
  template <class Iterator>
  RollingHash(Iterator first, Iterator last) {
    prefix_.push_back(M::e());
    for (; first != last; ++first) {
      prefix_.push_back(M::op(prefix_.back(), value_type(*first)));
    }
  }

  /** @brief 列の長さを取得する */
  long long size() const {
    return (long long)prefix_.size() - 1;
  }

  /** @brief 区間 [l, r) のハッシュを取得する */
  value_type prod(long long l, long long r) const {
    assert(0 <= l && l <= r && r <= size());
    return M::subtract_prefix(prefix_[l], prefix_[r]);
  }

  /** @brief 列全体のハッシュを取得する */
  value_type all_prod() const {
    return prefix_.back();
  }

  /** @brief 末尾に1要素を追加する
   *
   * ### 計算量
   * O(1) 償却
   */
  void push_back(long long x) {
    prefix_.push_back(M::op(prefix_.back(), value_type(x)));
  }

  void push_back(value_type x) {
    assert(x.length == 1);
    prefix_.push_back(M::op(prefix_.back(), x));
  }

  /** @brief 末尾の1要素を削除する
   *
   * ### 計算量
   * O(1)
   */
  void pop_back() {
    assert(!empty());
    prefix_.pop_back();
  }

  /** @brief 列が空か判定する */
  bool empty() const {
    return prefix_.size() == 1;
  }

  /** @brief 全要素を削除する */
  void clear() {
    prefix_.resize(1);
  }

  template <class Container>
  void build(const Container& c) {
    prefix_.clear();
    prefix_.reserve(c.size() + 1);
    prefix_.push_back(M::e());

    for (const auto& x : c) {
      push_back(x);
    }

    M::reserve_pow(size());
  }
};
