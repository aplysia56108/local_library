#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
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
  using Node = Bplustree::Node<SetTraits>;
  using Leaf = Bplustree::Leaf<SetTraits>;
  struct Internal;
};

/** @brief 子部分木の最小キーを Internal だけに保持する。 */
template <class Key, std::size_t Capacity>
struct SetTraits<Key, Capacity>::Internal
    : Bplustree::Internal<SetTraits<Key, Capacity>> {
  using Traits = SetTraits<Key, Capacity>;
  using Parent = Bplustree::Internal<Traits>;
  using Leaf = typename Traits::Leaf;
  using Parent::Parent;

  const Key* min_key = nullptr;

  void after_sync() {
    if (this->count == 0) {
      min_key = nullptr;
    } else if (this->level) {
      min_key = this->get_child(0)->min_key;
    } else {
      min_key =
          std::addressof(static_cast<Leaf*>(this->children[0].get())->get());
    }
  }
};

}  // namespace Bplustree

enum class SetType { Set, MultiSet };

/**
 * @brief ソート済みのキーを保持する B+ tree set / multiset。
 * @details Base の位置変更・反転・点更新を公開しない。
 * Iterator は読み取り専用。キーの重複可否は Type で決まる。
 *
 * 境界探索では Internal の最小キー情報を使い1経路だけを降りる。
 * ### 計算量
 * lower_bound / upper_bound: O(NodeCapacity * 木の高さ)
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

  bool inner_comp(const Key& a, const Key& b) const {
    const Key* left = std::addressof(this->bbegin_.leaf_->get());
    const Key* right = std::addressof(this->end_.leaf_->get());
    const Key* pa = std::addressof(a);
    const Key* pb = std::addressof(b);

    if (pa == left) return pb != left;
    if (pb == left) return false;
    if (pb == right) return pa != right;
    if (pa == right) return false;
    return compare_(a, b);
  }

  template <bool Equal>
  bool comp(const Key& a, const Key& b) const {
    if constexpr (Equal) return !inner_comp(b, a);
    return inner_comp(a, b);
  }

  bool equivalent(const Key& a, const Key& b) const {
    return !inner_comp(a, b) && !inner_comp(b, a);
  }

  using Leaf = typename Parent::Leaf;
  using Internal = typename Parent::Internal;
  template <bool Equal>
  Leaf* max_l(Internal* node, const Key& key) const {
    std::size_t l = 0;
    std::size_t r = node->count;

    while (l + 1 < r) {
      std::size_t m = (l + r) / 2;
      const Key& x = node->level
                         ? *node->get_child(m)->min_key
                         : static_cast<Leaf*>(node->children[m].get())->get();
      if (comp<Equal>(x, key))
        l = m;
      else
        r = m;
    }
    return static_cast<Leaf*>(this->children[l].get());
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

  Bplustree_Set()
    requires std::default_initializable<Compare>
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

  friend void swap(Bplustree_Set& a, Bplustree_Set& b) noexcept(
      std::is_nothrow_swappable_v<Compare>)
    requires std::swappable<Compare>
  {
    using std::swap;
    swap(a.compare_, b.compare_);
    swap(static_cast<Parent&>(a), static_cast<Parent&>(b));
  }

  const Compare& key_comp() const {
    return compare_;
  }
  size_type size() const {
    return Parent::size();
  }
  bool empty() const {
    return Parent::empty();
  }
  iterator bbegin() const {
    return Parent::bbegin();
  }
  iterator begin() const {
    return Parent::begin();
  }
  iterator end() const {
    return Parent::end();
  }
  iterator cbegin() const {
    return begin();
  }
  iterator cend() const {
    return end();
  }
  reverse_iterator rbegin() const {
    return reverse_iterator(end());
  }
  reverse_iterator rend() const {
    return reverse_iterator(begin());
  }
  const_reverse_iterator crbegin() const {
    return rbegin();
  }
  const_reverse_iterator crend() const {
    return rend();
  }
  const Key& operator[](size_type index) const {
    return Parent::get(index);
  }
  iterator nth(size_type index) const {
    return Parent::nth(index);
  }

  void clear() {
    Parent::clear();
  }
  void rebuild() {
    Parent::rebuild();
  }

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
    return max_lt(key) + 1;
  }
  iterator upper_bound(const Key& key) const {
    return max_le(key) + 1;
  }
  std::pair<iterator, iterator> equal_range(const Key& key) const {
    return {lower_bound(key), upper_bound(key)};
  }
  iterator min_ge(const Key& key) const {
    return lower_bound(key);
  }
  iterator min_gt(const Key& key) const {
    return upper_bound(key);
  }
  iterator max_lt(const Key& key) const {
    return {max_l<false>(this->root_, key)};
  }
  iterator max_le(const Key& key) const {
    return {max_l<true>(this->root_, key)};
  }

  iterator find(const Key& key) const {
    auto pos = lower_bound(key);
    return pos != end() && equivalent(*pos, key) ? pos : end();
  }
  bool contains(const Key& key) const {
    return find(key) != end();
  }
  size_type count(const Key& key) const {
    return upper_bound(key) - lower_bound(key);
  }

  std::pair<iterator, bool> insert(const Key& key) {
    auto it = base_iterator{max_l<true>(this->root_, key)};
    if constexpr (Type == SetType::Set) {
      if (equivalent(*it, key)) return {it, false};
    }
    return {Parent::insert(it, key), true};
  }
  template <std::input_iterator It>
  void insert(It first, It last) {
    for (; first != last; ++first) insert(*first);
  }
  size_type erase(const Key& key, bool single_erase = false) {
    const iterator first = lower_bound(key);
    if (!equivalent(*first, key)) return 0;
    const iterator last = single_erase ? first + 1 : upper_bound(key);
    const std::size_t res = single_erase ? 1 : last - first;
    Parent::erase(first, last);
    return res;
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
