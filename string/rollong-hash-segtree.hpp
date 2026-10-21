#pragma once

#include <concepts>
#include <iterator>
#include <string>
#include <vector>

#include "string/rolling-hash-monoid.hpp"
#include "data-structure/btree/bplustree-segtree.hpp"

template <class T = char>
class RollingHashSegtree : public Bplustree_Segtree<RollingHashMonoid<T>> {
  using M = RollingHashMonoid<T>;
  using Base = Bplustree_Segtree<M>;
  using Hash = typename M::value_type;
  using SetterBase = typename Base::Setter;

  class RHSetter : SetterBase {
   public:
    RHSetter(Base* tree, std::size_t index) : SetterBase(tree, index) {}
    using SetterBase::operator=;
    void operator=(const T& value) && {
      this->tree_->set(this->index_, make_rolling_hash_value<T>(value));
    }
  };

 public:
  using io_via_val = void;
  using Base::Base;

  /**
   * @brief イテレータ範囲 [first, last) から構築する。
   *
   * ### 計算量
   * O(N)
   */
  template <std::input_iterator InputIt>
  RollingHashSegtree(InputIt first, InputIt last) {
    using U = std::iter_value_t<InputIt>;
    if constexpr (std::same_as<U, Hash>) {
      this->assign(first, last);
    } else {
      std::vector<Hash> v;
      for (; first != last; ++first) {
        if constexpr (std::convertible_to<U, T>)
          v.emplace_back(make_rolling_hash_value<T>(static_cast<T>(*first)));
        else
          v.emplace_back(
              make_rolling_hash_value<T>(static_cast<T>(first->val())));
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

  std::string val() const
    requires std::same_as<T, char>
  {
    std::string out;
    out.reserve(this->size());

    for (const auto& x : *this) {
      out.push_back(x.val());
    }
    return out;
  };

 public:
  RHSetter operator()(std::size_t i) {
    return {this, i};
  }
};

RollingHashSegtree(const std::string&) -> RollingHashSegtree<char>;

template <std::input_iterator InputIt>
RollingHashSegtree(InputIt, InputIt)
    -> RollingHashSegtree<std::iter_value_t<InputIt>>;