#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

#include "data-structure/btree/components/bplustree-base.hpp"

namespace Bplustree {

/** @brief Set 用 Traits。Leaf はキー1個のみ保持する。 */
template <class Key, std::size_t Capacity>
struct SetTraits {
  using T = Key;
  static constexpr std::size_t kNodeCapacity = Capacity;
  static constexpr bool IsReversible = false;
  static constexpr bool IsMergable = false;
  using Node = Bplustree::Node<SetTraits>;
  using Leaf = Bplustree::Leaf<SetTraits>;
  using Internal = Bplustree::Internal<SetTraits>;
};

}  // namespace Bplustree

enum class SetType { Set, MultiSet };

/**
 * @brief ソート済みのキーを保持する B+ tree set / multiset。
 * @details Base の位置変更・反転・点更新を公開しない。
 * Iterator は読み取り専用。キーの重複可否は Type で決まる。
 *
 * 境界探索は現時点では順位二分探索を用いる。
 * ### 計算量
 * lower_bound / upper_bound: O(log N * NodeCapacity * 木の高さ)
 * 挿入・削除: 上記探索 + Base の構造変更
 */
template <class Key = long long, SetType Type = SetType::Set,
          class Compare = std::less<Key>, std::size_t NodeCapacity = 64>
class Bplustree_Set
    : protected Bplustree::Base<Bplustree::SetTraits<Key, NodeCapacity>> {
  using Parent = Bplustree::Base<Bplustree::SetTraits<Key, NodeCapacity>>;
  using base_iterator = typename Parent::iterator;

  static_assert(std::strict_weak_order<Compare&, const Key&, const Key&>);
  [[no_unique_address]] Compare compare_;

  bool equivalent(const Key& a, const Key& b) const {
    return !compare_(a, b) && !compare_(b, a);
  }

  std::size_t bound_index(const Key& key, bool upper) const {
    std::size_t low = 0, high = Parent::size();
    while (low < high) {
      const auto middle = low + (high - low) / 2;
      const Key& value = *Parent::nth(middle);
      const bool before = upper ? !compare_(key, value)
                                : compare_(value, key);
      if (before)
        low = middle + 1;
      else
        high = middle;
    }
    return low;
  }

 public:
  using key_type = Key;
  using value_type = Key;
  using key_compare = Compare;
  using size_type = typename Parent::size_type;
  using difference_type = typename Parent::difference_type;
  using iterator = typename Parent::const_iterator;
  using const_iterator = iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = reverse_iterator;

  Bplustree_Set() requires std::default_initializable<Compare>
      : Parent(), compare_() {}
  explicit Bplustree_Set(Compare compare)
      : Parent(), compare_(std::move(compare)) {}

  template <std::input_iterator It>
  Bplustree_Set(It first, It last)
    requires std::default_initializable<Compare>
      : Bplustree_Set() {
    assign(first, last);
  }
  template <std::input_iterator It>
  Bplustree_Set(It first, It last, Compare compare)
      : Bplustree_Set(std::move(compare)) {
    assign(first, last);
  }

  Bplustree_Set(const Bplustree_Set&) = default;
  Bplustree_Set(Bplustree_Set&&) = default;
  Bplustree_Set& operator=(const Bplustree_Set&) = default;
  Bplustree_Set& operator=(Bplustree_Set&&) = default;

  friend void swap(Bplustree_Set& a, Bplustree_Set& b)
      noexcept(std::is_nothrow_swappable_v<Compare>)
    requires std::swappable<Compare>
  {
    using std::swap;
    swap(a.compare_, b.compare_);
    swap(static_cast<Parent&>(a), static_cast<Parent&>(b));
  }

  const Compare& key_comp() const { return compare_; }
  size_type size() const { return Parent::size(); }
  bool empty() const { return Parent::empty(); }
  iterator bbegin() const { return Parent::bbegin(); }
  iterator begin() const { return Parent::begin(); }
  iterator end() const { return Parent::end(); }
  iterator cbegin() const { return begin(); }
  iterator cend() const { return end(); }
  reverse_iterator rbegin() const { return reverse_iterator(end()); }
  reverse_iterator rend() const { return reverse_iterator(begin()); }
  const_reverse_iterator crbegin() const { return rbegin(); }
  const_reverse_iterator crend() const { return rend(); }
  const Key& operator[](size_type index) const { return Parent::get(index); }
  iterator nth(size_type index) const { return Parent::nth(index); }

  void clear() { Parent::clear(); }
  void rebuild() { Parent::rebuild(); }

  template <std::input_iterator It>
  void assign(It first, It last) {
    std::vector<Key> values;
    for (; first != last; ++first) values.push_back(*first);
    if (!std::is_sorted(values.begin(), values.end(), compare_))
      std::sort(values.begin(), values.end(), compare_);
    if constexpr (Type == SetType::Set) {
      values.erase(std::unique(values.begin(), values.end(),
                               [&](const Key& a, const Key& b) {
                                 return equivalent(a, b);
                               }),
                   values.end());
    }
    this->build(values);
  }

  iterator lower_bound(const Key& key) const {
    return nth(bound_index(key, false));
  }
  iterator upper_bound(const Key& key) const {
    return nth(bound_index(key, true));
  }
  std::pair<iterator, iterator> equal_range(const Key& key) const {
    return {lower_bound(key), upper_bound(key)};
  }
  iterator min_ge(const Key& key) const { return lower_bound(key); }
  iterator min_gt(const Key& key) const { return upper_bound(key); }
  iterator max_lt(const Key& key) const { return lower_bound(key) - 1; }
  iterator max_le(const Key& key) const { return upper_bound(key) - 1; }

  iterator find(const Key& key) const {
    auto pos = lower_bound(key);
    return pos != end() && equivalent(*pos, key) ? pos : end();
  }
  bool contains(const Key& key) const { return find(key) != end(); }
  size_type count(const Key& key) const {
    return bound_index(key, true) - bound_index(key, false);
  }

  std::pair<iterator, bool> insert(const Key& key) {
    auto index = bound_index(key, false);
    if constexpr (Type == SetType::Set) {
      if (index < size() && equivalent(*nth(index), key))
        return {nth(index), false};
    }
    return {Parent::insert(index, key), true};
  }
  template <std::input_iterator It>
  void insert(It first, It last) {
    const std::vector<Key> values(first, last);
    for (const auto& key : values) insert(key);
  }
  size_type erase(const Key& key, bool single_erase = false) {
    const size_type first = bound_index(key, false);
    const size_type last = bound_index(key, true);
    const size_type removed =
        single_erase ? std::min<size_type>(last - first, 1) : last - first;
    if (removed) Parent::erase(first, first + removed);
    return removed;
  }
  iterator erase(iterator pos) {
    const auto index = static_cast<size_type>(pos - begin());
    return Parent::erase(index);
  }
};
template <class T = long long>
using BSet = Bplustree_Set<T>;
template <class T = long long>
using BMultiSet = Bplustree_Set<T, SetType::MultiSet>;

// compare_から型推論するためのtrait
template <class T>
struct BinaryCallableTraits;

template <class R, class C, class A, class B>
struct BinaryCallableTraits<R (C::*)(A, B) const> {
  using FirstArgumentType = A;
  using SecondArgumentType = B;
};

template <class R, class C, class A, class B>
struct BinaryCallableTraits<R (C::*)(A, B)> {
  using FirstArgumentType = A;
  using SecondArgumentType = B;
};

template <class R, class C, class A, class B>
struct BinaryCallableTraits<R (C::*)(A, B) const noexcept>
    : BinaryCallableTraits<R (C::*)(A, B) const> {};

template <class R, class C, class A, class B>
struct BinaryCallableTraits<R (C::*)(A, B) noexcept>
    : BinaryCallableTraits<R (C::*)(A, B)> {};

template <class Compare>
struct CompareKey {
 private:
  using Traits = BinaryCallableTraits<decltype(&Compare::operator())>;
  using First = std::remove_cvref_t<typename Traits::FirstArgumentType>;
  using Second = std::remove_cvref_t<typename Traits::SecondArgumentType>;

  static_assert(std::same_as<First, Second>);

 public:
  using type = First;
};

template <class Compare>
using CompareKeyType = typename CompareKey<std::decay_t<Compare>>::type;
/**
 * @brief 指定した比較器を持つ B+ tree set を生成する。
 * @tparam NodeCapacity Node の容量。
 * @param compare 比較器。2引数の非 generic lambda を想定する。
 * @return 指定した比較器を使用する B+ tree set。
 * @details
 * `compare` の2つの引数型から `Key` を自動推定する。
 * 引数が参照型や const 修飾されている場合、それらを除いた型を `Key` とする。
 *
 * generic lambda や、`operator()` がオーバーロードされた関数オブジェクトには
 * 対応しない。
 *
 * 比較器は B+ tree 内部に値として保持される。ただし比較器が外部オブジェクトを
 * 参照キャプチャしている場合、そのオブジェクトは B+ tree より長く生存する
 * 必要がある。
 *
 * また、比較器が参照する外部状態を変更することで既存要素間の順序関係が
 * 変化すると、B+ tree の順序不変条件が崩れ、検索結果などが不正になる。
 *
 * ### 計算量
 * O(1)
 */
template <std::size_t NodeCapacity = 64, class Compare>
auto make_bset(Compare&& compare) {
  using CompareType = std::decay_t<Compare>;
  using Key = CompareKeyType<CompareType>;

  return Bplustree_Set<Key, SetType::Set, CompareType, NodeCapacity>(
      std::forward<Compare>(compare));
}

/**
 * @brief 指定した比較器を持つ B+ tree multiset を生成する。
 * @tparam NodeCapacity Node の容量。
 * @param compare 比較器。2引数の非 generic lambda を想定する。
 * @return 指定した比較器を使用する B+ tree multiset。
 * @details
 * `compare` の2つの引数型から `Key` を自動推定する。
 * 引数が参照型や const 修飾されている場合、それらを除いた型を `Key` とする。
 *
 * generic lambda や、`operator()` がオーバーロードされた関数オブジェクトには
 * 対応しない。
 *
 * 比較器は B+ tree 内部に値として保持される。ただし比較器が外部オブジェクトを
 * 参照キャプチャしている場合、そのオブジェクトは B+ tree より長く生存する
 * 必要がある。
 *
 * また、比較器が参照する外部状態を変更することで既存要素間の順序関係が
 * 変化すると、B+ tree の順序不変条件が崩れ、検索結果などが不正になる。
 *
 * ### 計算量
 * O(1)
 */
template <std::size_t NodeCapacity = 64, class Compare>
auto make_bmultiset(Compare&& compare) {
  using CompareType = std::decay_t<Compare>;
  using Key = CompareKeyType<CompareType>;

  return Bplustree_Set<Key, SetType::MultiSet, CompareType, NodeCapacity>(
      std::forward<Compare>(compare));
}

/**
 * @brief 指定した比較器を持つ B+ tree set を範囲構築する。
 * @tparam NodeCapacity Node の容量。
 * @param first 範囲の先頭。
 * @param last 範囲の終端。
 * @param compare 比較器。2引数のlambda
 * を想定する。範囲から型推論可能なため、genericでも可。
 * @return 指定した比較器を使用し、範囲を要素として構築した B+ tree set。
 * @details
 * `InputIt` の value type から `Key` を自動推定する。
 * 比較器は B+ tree 内部に値として保持される。ただし比較器が外部オブジェクトを
 * 参照キャプチャしている場合、そのオブジェクトは B+ tree より長く生存する
 * 必要がある。
 *
 * また、比較器が参照する外部状態を変更することで既存要素間の順序関係が
 * 変化すると、B+ tree の順序不変条件が崩れ、検索結果などが不正になる。
 *
 * ### 計算量
 * N を範囲の要素数として、
 * ソート済みの場合 O(N)。
 * それ以外の場合 O(N log N)。
 */
template <std::size_t NodeCapacity = 64, std::input_iterator InputIt,
          class Compare>
auto make_bset(InputIt first, InputIt last, Compare&& compare) {
  using Key = std::iter_value_t<InputIt>;
  using CompareType = std::decay_t<Compare>;

  return Bplustree_Set<Key, SetType::Set, CompareType, NodeCapacity>(
      first, last, std::forward<Compare>(compare));
}

/**
 * @brief 指定した比較器を持つ B+ tree multiset を範囲構築する。
 * @tparam NodeCapacity Node の容量。
 *@param first 範囲の先頭。
 * @param last 範囲の終端。
 * @param compare 比較器。2引数のlambda
 * を想定する。範囲から型推論可能なため、genericでも可。
 * @return 指定した比較器を使用し、範囲を要素として構築した B+ tree multiset。
 * @details
 * `InputIt` の value type から `Key` を自動推定する。
 * 比較器は B+ tree 内部に値として保持される。ただし比較器が外部オブジェクトを
 * 参照キャプチャしている場合、そのオブジェクトは B+ tree より長く生存する
 * 必要がある。
 *
 * また、比較器が参照する外部状態を変更することで既存要素間の順序関係が
 * 変化すると、B+ tree の順序不変条件が崩れ、検索結果などが不正になる。
 *
 * ### 計算量
 * N を範囲の要素数として、
 * ソート済みの場合 O(N)。
 * それ以外の場合 O(N log N)。
 */
template <std::size_t NodeCapacity = 64, std::input_iterator InputIt,
          class Compare>
auto make_bmultiset(InputIt first, InputIt last, Compare&& compare) {
  using Key = std::iter_value_t<InputIt>;
  using CompareType = std::decay_t<Compare>;

  return Bplustree_Set<Key, SetType::MultiSet, CompareType, NodeCapacity>(
      first, last, std::forward<Compare>(compare));
}
