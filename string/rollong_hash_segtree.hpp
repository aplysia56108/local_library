#pragma once

#include <concepts>
#include <iterator>
#include <string>
#include <vector>

#include "string/rolling_hash_monoid.hpp"
#include "data-structure/btree/bplustree-segtree.hpp"

template <class T = char>
class RollingHashSegtree : public Bplustree_Segtree<RollingHashMonoid<T>> {
  using M = RollingHashMonoid<T>;
  using Base = Bplustree_Segtree<M>;
  using Hash = typename M::value_type;

 public:
  using Base::Base;

  /**
   * @brief イテレータ範囲 [first, last) から構築する。
   *
   * ### 計算量
   * O(N)
   */
  template <std::input_iterator InputIt>
  RollingHashSegtree(InputIt first, InputIt last) {
    if constexpr (std::same_as<std::iter_value_t<InputIt>, Hash>) {
      this->assign(first, last);
    } else {
      std::vector<Hash> v;
      for (; first != last; ++first) {
        v.emplace_back(static_cast<long long>(*first));
      }
      this->assign(v.begin(), v.end());
    }
  }

  /**
   * @brief 文字列から構築する。
   *
   * ### 計算量
   * O(N)
   */
  explicit RollingHashSegtree(const std::string& s)
      : RollingHashSegtree(s.begin(), s.end()) {}
};

RollingHashSegtree(const std::string&) -> RollingHashSegtree<char>;

template <std::input_iterator InputIt>
RollingHashSegtree(InputIt, InputIt) -> RollingHashSegtree<char>;