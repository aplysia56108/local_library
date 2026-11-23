#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "data-structure/btree/components/bplustree-set-traits.hpp"

enum class SetType { Set, MultiSet };

/**
 * @brief 比較器の順序でキーを保持する B+tree Set / Multiset。
 * @tparam Key キー型。
 * @tparam Type Set（等価キーを重複排除）または MultiSet（重複を保持）。
 * @tparam Compare 厳密弱順序を与える比較器。
 * @tparam NodeCapacity Internal の子 ID 配列の容量。
 * @details Base を protected 継承して位置による更新・反転を隠す。
 * iterator は const、添字参照も読み取り専用。境界探索は Internal の
 * min_leaf_id を使って1本の探索経路を降りる。
 *
 * ### 計算量
 * lower_bound / upper_bound: O(NodeCapacity * 木の高さ)。
 * insert / erase: 上記探索と Base の構造変更。
 */
template <class Key = long long, SetType Type = SetType::Set,
          class Compare = std::less<Key>, std::size_t NodeCapacity = 64>
class Bplustree_Set
    : protected Bplustree::Base<Bplustree::SetTraits<Key, NodeCapacity>> {
  using Parent = Bplustree::Base<Bplustree::SetTraits<Key, NodeCapacity>>;
  using base_iterator = typename Parent::iterator;

  static_assert(std::strict_weak_order<Compare&, const Key&, const Key&>);
  [[no_unique_address]] Compare compare_;
  using Pools = typename Parent::Pools;
  using id_type = typename Pools::id_type;

  /**
   * @brief 番兵を含む Leaf ID とキー b を Compare の順序で比較する。
   * @details bbegin は全キーより前、end は全キーより後と扱う。
   * Reversed は比較器の引数を逆にする（降順キー専用の指定ではない）。
   * 番兵に一致する分岐は通常の検索では稀なので unlikely とする。
   */
  template <bool Reversed = false>
  bool inner_comp(const id_type& id, const Key& b) const {
    if (id == this->bbegin_) [[unlikely]]
      return true ^ Reversed;
    if (id == this->end_) [[unlikely]]
      return false ^ Reversed;

    if constexpr (Reversed) return compare_(b, Parent::get_leaf(id).get());
    return compare_(Parent::get_leaf(id).get(), b);
  }

  template <bool Equal>
  bool comp(const id_type& id, const Key& b) const {
    if constexpr (Equal) return !inner_comp<true>(id, b);
    return inner_comp(id, b);
  }

  bool equivalent(const id_type& id, const Key& b) const {
    return !inner_comp(id, b) && !inner_comp<true>(id, b);
  }

  using Leaf = typename Parent::Leaf;
  /**
   * @brief 最右の「key より小さい／以下」の Leaf ID を探索する。
   * @details Equal=false では <、true では <=。先頭番兵を下限として
   * 各 Internal の子の先頭キーを比較し、1経路を下降する。
   */
  template <bool Equal>
  id_type max_l(id_type node_id, const Key& key) const {
    const auto& node =
        Bplustree::NodeAccess<Bplustree::SetTraits<Key, NodeCapacity>>::get(
            node_id);
    std::size_t l = 0;
    std::size_t r = node.count;

    while (l + 1 < r) {
      const std::size_t m = (l + r) / 2;
      const id_type id =
          node.level ? Pools::with_node(node.children[m],
                                        [](const auto& child) -> id_type {
                                          return child.min_leaf_id;
                                        })
                     : node.children[m];
      if (comp<Equal>(id, key))
        l = m;
      else
        r = m;
    }
    if (node.level) return max_l<Equal>(node.children[l], key);
    return node.children[l];
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

  /** @brief 要素を複製して構築する。比較器もコピーされる。 */
  Bplustree_Set(const Bplustree_Set&) = default;
  /** @brief Base の木を空の新規オブジェクトへ移す。比較器もムーブする。 */
  Bplustree_Set(Bplustree_Set&&) = default;
  Bplustree_Set& operator=(const Bplustree_Set&) = default;
  /**
   * @brief Base 部分は swap ベースでムーブ代入する。
   * @details 移動元は必ずしも空にならず、代入先の旧要素を保持する。
   * 比較器は Compare 自身のムーブ代入の意味論に従う。
   */
  Bplustree_Set& operator=(Bplustree_Set&&) = default;

  /** @brief 比較器と要素木を交換する。各オブジェクトの番兵 ID は固定。 */
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
  /** @brief 最小キーの直前にある固定番兵。値を読み取らないこと。 */
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
  /** @brief 0-indexed の index 番目（index==size() は end）を返す。 */
  iterator nth(size_type index) const {
    return Parent::nth(index);
  }

  void clear() {
    Parent::clear();
  }
  void rebuild() {
    Parent::rebuild();
  }

  /**
   * @brief 比較器に従って範囲をソートし、Set なら等価キーを除去して再構築。
   * @details 旧実要素の iterator は失効し、固定番兵 ID は変わらない。
   * ### 計算量
   * ソート済み入力は O(N)、その他は O(N log N)。
   */
  template <std::input_iterator It>
  void assign(It first, It last) {
    std::vector<Key> values;
    for (; first != last; ++first) values.push_back(*first);
    if (!std::is_sorted(values.begin(), values.end(), compare_))
      std::sort(values.begin(), values.end(), compare_);
    if constexpr (Type == SetType::Set) {
      values.erase(std::unique(values.begin(), values.end(),
                               [&](const Key& a, const Key& b) {
                                 return !compare_(a, b) && !compare_(b, a);
                               }),
                   values.end());
    }
    this->build(values);
  }

  /**
   * @brief pos 以降の要素を新しい Set / Multiset に分離する。
   * @details
   * 自身には [begin(), pos)、返り値には [pos, end()) を残す。
   * 固定番兵 ID はそれぞれの木に残り、実要素 Leaf と iterator は維持する。
   * 返り値には比較器をコピーするため、Compare のコピー構築が必要。
   * ### 計算量
   * O(NodeCapacity * log N)
   */
  Bplustree_Set split(iterator pos)
    requires std::copy_constructible<Compare>
  {
    Bplustree_Set right(compare_);
    static_cast<Parent&>(right) = Parent::split(this->mutable_iterator(pos));
    return right;
  }

  /**
   * @brief 先頭 index 要素を自身に残し、残りを返す。
   * ### 計算量
   * O(NodeCapacity * log N)
   */
  Bplustree_Set split_by_index(size_type index)
    requires std::copy_constructible<Compare>
  {
    return split(nth(index));
  }

  /**
   * @brief key 未満を自身に残し、key 以上を返す。
   * @details 比較器の順序に従う。Multiset の等価キーはすべて右側に入る。
   * ### 計算量
   * O(NodeCapacity * log N)
   */
  Bplustree_Set split_by_key(const Key& key)
    requires std::copy_constructible<Compare>
  {
    return split(lower_bound(key));
  }

  /**
   * @brief right の全要素を自身の末尾へ連結し、right を空にする。
   * @details
   * std::set::merge と異なり、一般の要素を再配置する操作ではない。
   * 両側が同じ順序関係でソートされていることを前提とする。
   * Compare の状態が異なり順序関係も異なる場合は呼び出せない。
   * 境界の大小は assert で確認するが、比較器全体の一致は検証しない。
   * Set は max(left) < min(right)、Multiset は
   * max(left) <= min(right) を要する (compare_ による順序)。
   * 実要素の Leaf ID と iterator の同一性は維持する。
   * ### 計算量
   * O(NodeCapacity * log(N + M))
   */
  void merge(Bplustree_Set& right) {
    assert(this != &right);
    if (!empty() && !right.empty()) {
      const Key& left_last = *std::prev(end());
      const Key& right_first = *right.begin();
      if constexpr (Type == SetType::Set)
        assert(compare_(left_last, right_first));
      else
        assert(!compare_(right_first, left_last));
    }
    Parent::merge(static_cast<Parent&>(right));
  }

  /** @brief 比較器順で key 以上の最初の要素。なければ end()。 */
  iterator lower_bound(const Key& key) const {
    return max_lt(key) + 1;
  }
  /** @brief 比較器順で key より後の最初の要素。なければ end()。 */
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
  /** @brief 比較器順で key より前の最大要素。なければ bbegin()。 */
  iterator max_lt(const Key& key) const {
    return iterator{max_l<false>(this->root_, key)};
  }
  /** @brief 比較器順で key 以下の最大要素。なければ bbegin()。 */
  iterator max_le(const Key& key) const {
    return iterator{max_l<true>(this->root_, key)};
  }

  iterator find(const Key& key) const {
    auto pos = lower_bound(key);
    return pos != end() && equivalent(Parent::get_id(pos), key) ? pos : end();
  }
  bool contains(const Key& key) const {
    return find(key) != end();
  }
  size_type count(const Key& key) const {
    return upper_bound(key) - lower_bound(key);
  }

  /** @brief キーを挿入する。Set の既存等価キーなら false、Multiset は常に
   * true。 */
  std::pair<iterator, bool> insert(const Key& key) {
    auto it = this->mutable_iterator(lower_bound(key));
    if constexpr (Type == SetType::Set) {
      if (equivalent(Parent::get_id(it), key)) return {iterator{it}, false};
    }
    return {iterator{Parent::insert(it, key)}, true};
  }
  template <std::input_iterator It>
  void insert(It first, It last) {
    for (; first != last; ++first) insert(*first);
  }
  /**
   * @brief key と等価なキーを削除し、削除個数を返す。
   * @details MultiSet では通常すべて削除し、single_erase=true は1個だけ削除。
   */
  size_type erase(const Key& key, bool single_erase = false) {
    const base_iterator first = this->mutable_iterator(lower_bound(key));
    if (!equivalent(Parent::get_id(first), key)) return 0;
    if constexpr (Type == SetType::Set) {
      Parent::erase(first);
      return 1;
    }
    if (single_erase) {
      Parent::erase(first);
      return 1;
    }
    const base_iterator last = this->mutable_iterator(upper_bound(key));
    const std::size_t res = last - first;
    Parent::erase(first, last);
    return res;
  }
  iterator erase(iterator pos) {
    return Parent::erase(this->mutable_iterator(pos));
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
