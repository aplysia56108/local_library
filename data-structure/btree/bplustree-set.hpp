#pragma once

#include <functional>

#include "data-structure/btree/components/bplustree-base.hpp"

namespace Bplustree {
template <class Key, std::size_t Capacity>
struct SetTraits {
  using T = Key;
  static constexpr std::size_t kNodeCapacity = Capacity;
  struct Node;
  struct Leaf;
  struct Internal;
};
template <class K, std::size_t C>
struct SetTraits<K, C>::Node : Bplustree::Node<SetTraits<K, C>> {
  using Bplustree::Node<SetTraits<K, C>>::Node;
  const K* min_key = nullptr;
};
template <class K, std::size_t C>
struct SetTraits<K, C>::Leaf : Bplustree::Leaf<SetTraits<K, C>> {
  void after_sync() { this->min_key = this->count ? &this->values[0] : nullptr; }
};
template <class K, std::size_t C>
struct SetTraits<K, C>::Internal : Bplustree::Internal<SetTraits<K, C>> {
  void after_sync() {
    this->min_key = this->count ? this->children[0]->min_key : nullptr;
  }
};
}  // namespace Bplustree

enum class SetType { Set, MultiSet };

/** Ordered set/multiset backed by shared positional storage.
 * Positional writes and reversal stay protected to preserve sorted order.
 * Bounds visit one path, using subtree minima and Compare equivalence.
 */
template <class Key = long long, SetType Type = SetType::Set,
          class Compare = std::less<Key>, std::size_t NodeCapacity = 64>
class Bplustree_Set
    : protected Bplustree::Base<Bplustree::SetTraits<Key, NodeCapacity>> {
  friend struct BplustreeSetTestAccess;
  using Parent = Bplustree::Base<Bplustree::SetTraits<Key, NodeCapacity>>;
  using typename Parent::Node;
  using typename Parent::Leaf;
  using typename Parent::Internal;
  static constexpr std::size_t kNodeCapacity = NodeCapacity;
  static_assert(std::strict_weak_order<const Compare&, const Key&, const Key&>);
  [[no_unique_address]] Compare compare_;
  bool equivalent(const Key& left, const Key& right) const {
    return !compare_(left, right) && !compare_(right, left);
  }
  std::size_t bound_index(const Key& key, bool upper) const {
    Node* node = this->root_.get();
    std::size_t offset = 0;
    auto before = [&](const Key& value) {
      return upper ? !compare_(key, value) : compare_(value, key);
    };
    while (!node->is_leaf()) {
      auto* internal = static_cast<Internal*>(node);
      // The boundary can be in the child immediately preceding the first
      // non-before minimum. This also handles duplicates spanning children.
      std::size_t index = 0;
      while (index + 1 < node->count &&
             before(*internal->children[index + 1]->min_key)) {
        offset += internal->children[index]->size;
        ++index;
      }
      node = internal->children[index].get();
    }
    auto* leaf = static_cast<Leaf*>(node);
    std::size_t first = 0, last = leaf->count;
    while (first < last) {
      auto middle = first + (last - first) / 2;
      if (before(leaf->values[middle])) first = middle + 1;
      else last = middle;
    }
    return offset + first;
  }
 public:
  using typename Parent::iterator;
  using typename Parent::const_iterator;
  using typename Parent::reverse_iterator;
  using typename Parent::const_reverse_iterator;
  using typename Parent::size_type;
  using typename Parent::difference_type;
  using key_type = Key;
  using value_type = Key;
  using key_compare = Compare;
  using Parent::begin;
  using Parent::end;
  using Parent::bbegin;
  using Parent::cbegin;
  using Parent::cend;
  using Parent::rbegin;
  using Parent::rend;
  using Parent::crbegin;
  using Parent::crend;
  using Parent::size;
  using Parent::empty;
  using Parent::nth;
  using Parent::operator[];
  using Parent::clear;
  using Parent::rebuild;

  Bplustree_Set() requires std::default_initializable<Compare> : compare_() {}
  explicit Bplustree_Set(Compare compare) : compare_(std::move(compare)) {}
  template <std::input_iterator It>
  Bplustree_Set(It first, It last) requires std::default_initializable<Compare> : compare_() {
    assign(first, last);
  }
  template <std::input_iterator It>
  Bplustree_Set(It first, It last, Compare compare) : compare_(std::move(compare)) {
    assign(first, last);
  }
  Bplustree_Set(const Bplustree_Set&) requires std::copy_constructible<Compare> = default;
  Bplustree_Set(Bplustree_Set&&) requires std::move_constructible<Compare> = default;
  Bplustree_Set& operator=(const Bplustree_Set& other)
      requires std::is_copy_assignable_v<Compare> {
    if (this != &other) {
      compare_ = other.compare_;
      Parent::operator=(static_cast<const Parent&>(other));
    }
    return *this;
  }
  Bplustree_Set& operator=(Bplustree_Set&& other)
      requires std::is_move_assignable_v<Compare> {
    if (this != &other) {
      compare_ = std::move(other.compare_);
      Parent::operator=(std::move(other));
    }
    return *this;
  }
  friend void swap(Bplustree_Set& left, Bplustree_Set& right)
      noexcept(std::is_nothrow_swappable_v<Compare>) requires std::swappable<Compare> {
    using std::swap;
    swap(left.compare_, right.compare_);
    swap(static_cast<Parent&>(left), static_cast<Parent&>(right));
  }
  const Compare& key_comp() const { return compare_; }
  template <std::input_iterator It>
  void assign(It first, It last) {
    std::vector<Key> values;
    for (; first != last; ++first) values.push_back(*first);
    auto compare = std::ref(compare_);
    if (!std::is_sorted(values.begin(), values.end(), compare))
      std::sort(values.begin(), values.end(), compare);
    if constexpr (Type == SetType::Set) {
      auto same = [&](const Key& a, const Key& b) { return equivalent(a, b); };
      values.erase(std::unique(values.begin(), values.end(), same), values.end());
    }
    this->build(values);
  }
  iterator lower_bound(const Key& key) const { return nth(bound_index(key, false)); }
  iterator upper_bound(const Key& key) const { return nth(bound_index(key, true)); }
  std::pair<iterator, iterator> equal_range(const Key& key) const {
    return {lower_bound(key), upper_bound(key)};
  }
  iterator min_ge(const Key& key) const { return lower_bound(key); }
  iterator min_gt(const Key& key) const { return upper_bound(key); }
  iterator max_lt(const Key& key) const { return lower_bound(key) - 1; }
  iterator max_le(const Key& key) const { return upper_bound(key) - 1; }
  iterator find(const Key& key) const {
    auto it = lower_bound(key);
    return it != end() && equivalent(*it, key) ? it : end();
  }
  bool contains(const Key& key) const { return find(key) != end(); }
  size_type count(const Key& key) const {
    return bound_index(key, true) - bound_index(key, false);
  }
  std::pair<iterator, bool> insert(const Key& key) {
    auto index = bound_index(key, false);
    if constexpr (Type == SetType::Set)
      if (index < size() && equivalent(*nth(index), key)) return {nth(index), false};
    return {Parent::insert(index, key), true};
  }
  template <std::input_iterator It>
  void insert(It first, It last) {
    std::vector<Key> values;
    for (; first != last; ++first) values.push_back(*first);
    for (const auto& key : values) insert(key);
  }
  size_type erase(const Key& key, bool single_erase = false) {
    const auto first = bound_index(key, false), last = bound_index(key, true);
    const auto removed = single_erase ? std::min<size_type>(last - first, 1) : last - first;
    Parent::erase(first, first + removed);
    return removed;
  }
  iterator erase(iterator position) { return Parent::erase(position); }
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
