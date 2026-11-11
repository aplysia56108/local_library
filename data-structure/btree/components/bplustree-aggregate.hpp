#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <utility>

#include "data-structure/btree/components/bplustree-base.hpp"

namespace Bplustree {

struct NoAction {
  using action_type = long long;
};

template <class M, class A, bool R, std::size_t C>
struct AggregateTraits {
  using T = typename M::value_type;
  using F = typename A::action_type;
  static constexpr std::size_t kNodeCapacity = C;
  static constexpr bool IsReversible = true;
  static constexpr bool IsMergable = true;
  static constexpr bool lazy = !std::same_as<A, NoAction>;
  static T identity() {
    return M::e();
  }
  struct Node;
  struct Leaf;
  struct Internal;
};

/** @brief Leaf と Internal に共通する親・位置情報だけを持つ Node。 */
template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Node
    : Bplustree::Node<AggregateTraits<M, A, R, C>> {
  using Bplustree::Node<AggregateTraits<M, A, R, C>>::Node;
};

/**
 * @brief 1要素だけを保持する Leaf。
 * @details 集約値や遅延タグは持たず、作用を受けた時点で値を更新する。
 */
template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Leaf
    : Bplustree::Leaf<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Parent = Bplustree::Leaf<Traits>;
  using T = typename Traits::T;
  using F = typename Traits::F;

  using Parent::Parent;

  void push_lazy(const F& f)
    requires(Traits::lazy)
  {
    this->set(A::mapping(f, this->get(), 1));
  }
};

/**
 * @brief 順方向・逆方向の積と未伝搬の作用を持つ Internal。
 * @details level==0 の子は Leaf、それ以外は Internal。
 * Leaf に集約値・遅延タグを設けないため、Leaf の作用は即時反映する。
 */
template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Internal
    : Bplustree::Internal<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Parent = Bplustree::Internal<Traits>;
  using T = typename Traits::T;
  using F = typename Traits::F;
  using Leaf = typename Traits::Leaf;

  using Parent::Parent;

  T forward = M::e();
  T backward = M::e();
  std::optional<F> pending;

  /**
   * @brief 構造や子の値から積を再計算する。
   * @details is_reversed / pending が残っていれば、再計算した物理状態の積に
   * それらを適用して論理状態の積を維持する。
   */
  void after_sync() {
    forward = backward = M::e();
    if (this->level == 0) {
      for (std::size_t i = 0; i < this->count; ++i)
        forward = M::op(forward, this->get_leaf(i).get());
      if constexpr (R) {
        backward = forward;
      } else {
        for (std::size_t i = this->count; i > 0; --i)
          backward = M::op(backward, this->get_leaf(i - 1).get());
      }
    } else {
      for (std::size_t i = 0; i < this->count; ++i)
        forward = M::op(forward, this->get_child(i).forward);
      if constexpr (R) {
        backward = forward;
      } else {
        for (std::size_t i = this->count; i > 0; --i)
          backward = M::op(backward, this->get_child(i - 1).backward);
      }
    }

    if constexpr (!R) {
      if (this->is_reversed) std::swap(forward, backward);
    }
    if constexpr (Traits::lazy) {
      if (pending) {
        forward = A::mapping(*pending, forward, this->size);
        if constexpr (R)
          backward = forward;
        else
          backward = A::mapping(*pending, backward, this->size);
      }
    }
  }

  /** @brief ノード全体への作用を遅延登録し、積だけ即時更新する。 */
  void push_lazy(const F& f)
    requires(Traits::lazy)
  {
    forward = A::mapping(f, forward, this->size);
    if constexpr (R)
      backward = forward;
    else
      backward = A::mapping(f, backward, this->size);
    pending = pending ? A::composition(f, *pending) : f;
  }

  /** @brief 反転フラグの変更に合わせて順・逆方向の積を交換する。 */
  void push_reverse_impl() {
    if constexpr (!R) std::swap(forward, backward);
  }

  /** @brief 未伝搬の作用を子へ渡す。Leaf には直接適用する。 */
  void apply_lazy() {
    if constexpr (Traits::lazy) {
      if (!pending) return;
      F f = *pending;
      pending.reset();
      if (this->level == 0) {
        for (std::size_t i = 0; i < this->count; ++i)
          this->get_leaf(i).push_lazy(f);
      } else {
        for (std::size_t i = 0; i < this->count; ++i)
          this->get_child(i).push_lazy(f);
      }
    }
  }
};

/**
 * @brief 1要素1Leafの B+ tree にモノイド積と遅延作用を追加する。
 * @details 番兵の値はモノイド単位元。Leaf の集約値を持たず、
 * level==0 で Leaf を読み、その他のレベルでは Internal の積を使う。
 */
template <class Derived, class M, class A, bool R, std::size_t C>
class AggregateTree : public Base<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Parent = Base<Traits>;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  using T = typename Traits::T;
  using F = typename Traits::F;
  using id_type = typename Parent::id_type;
  using Pools = NodePools<Traits>;

  template <bool Reversed = false>
  static T prod_recursive(Internal& node, std::size_t first, std::size_t last) {
    if (first == last) return M::e();
    if (first == 0 && last == node.size) {
      if constexpr (Reversed) return node.backward;
      return node.forward;
    }

    node.materialize();
    T result = M::e();

    if (node.level == 0) {
      if constexpr (Reversed) {
        for (std::size_t i = last; i > first; --i)
          result = M::op(result, node.get_leaf(i - 1).get());
      } else {
        for (std::size_t i = first; i < last; ++i)
          result = M::op(result, node.get_leaf(i).get());
      }
      return result;
    }

    if constexpr (Reversed) {
      std::size_t offset = node.size;
      for (std::size_t i = node.count; i > 0; --i) {
        auto& child = node.get_child(i - 1);
        const std::size_t length = child.size;
        offset -= length;
        if (first < offset + length && offset < last)
          result = M::op(result, prod_recursive<true>(
                                     child, first > offset ? first - offset : 0,
                                     std::min(last - offset, length)));
        if (offset <= first) break;
      }
    } else {
      std::size_t offset = 0;
      for (std::size_t i = 0; i < node.count; ++i) {
        auto& child = node.get_child(i);
        const std::size_t length = child.size;
        if (first < offset + length && offset < last)
          result = M::op(
              result, prod_recursive(child, first > offset ? first - offset : 0,
                                     std::min(last - offset, length)));
        offset += length;
        if (offset >= last) break;
      }
    }
    return result;
  }

  static void apply_recursive(Internal& node, std::size_t first,
                              std::size_t last, const F& f)
    requires(Traits::lazy)
  {
    if (first == last) return;
    if (first == 0 && last == node.size) {
      node.push_lazy(f);
      return;
    }

    node.materialize();
    if (node.level == 0) {
      for (std::size_t i = first; i < last; ++i) node.get_leaf(i).push_lazy(f);

    } else {
      std::size_t offset = 0;
      for (std::size_t i = 0; i < node.count; ++i) {
        auto& child = node.get_child(i);
        const std::size_t length = child.size;
        if (first < offset + length && offset < last)
          apply_recursive(child, first > offset ? first - offset : 0,
                          std::min(last - offset, length), f);
        offset += length;
        if (offset >= last) break;
      }
    }
    node.refresh();
  }

  // predicate が破れた部分木だけを再帰的に探索する。
  template <bool Leftward, class Predicate>
  static bool search(Internal& node, std::size_t offset, std::size_t first,
                     std::size_t last, T& value, Predicate& predicate,
                     std::size_t& result) {
    const std::size_t end = offset + node.size;
    if (end <= first || last <= offset) return true;
    if (first <= offset && end <= last) {
      T next =
          Leftward ? M::op(node.forward, value) : M::op(value, node.forward);
      if (predicate(next)) {
        value = std::move(next);
        result = Leftward ? offset : end;
        return true;
      }
    }

    node.materialize();
    if (node.level == 0) {
      for (std::size_t j = 0; j < node.count; ++j) {
        const std::size_t i = Leftward ? node.count - 1 - j : j;
        const std::size_t pos = offset + i;
        if (pos < first || pos >= last) continue;
        const T& x = node.get_leaf(i).get();
        T next = Leftward ? M::op(x, value) : M::op(value, x);
        if (!predicate(next)) return false;
        value = std::move(next);
        result = Leftward ? pos : pos + 1;
      }
      return true;
    }
    if constexpr (Leftward) {
      std::size_t pos = end;
      for (std::size_t i = node.count; i > 0; --i) {
        auto& child = node.get_child(i - 1);
        pos -= child.size;
        if (!search<true>(child, pos, first, last, value, predicate, result))
          return false;
      }
    } else {
      std::size_t pos = offset;
      for (std::size_t i = 0; i < node.count; ++i) {
        auto& child = node.get_child(i);
        if (!search<false>(child, pos, first, last, value, predicate, result))
          return false;
        pos += child.size;
      }
    }
    return true;
  }

 public:
  using Monoid = M;
  using Action = A;
  using Parent::Parent;
  using Parent::size;
  using typename Parent::iterator;
  using typename Parent::size_type;

  AggregateTree() = default;
  /** @brief モノイド単位元で count 要素を初期化する。 */
  explicit AggregateTree(size_type count) : Parent(count, M::e()) {}

  /** @brief 全要素の積。O(1)。 */
  T all_prod() const {
    return Parent::get_node(this->root_).forward;
  }

  /** @brief 半開区間 [first,last) の順方向の積。 */
  T prod(size_type first, size_type last) const {
    assert(first <= last && last <= size());
    return prod_recursive(Parent::get_node(this->root_), first + 1, last + 1);
  }

  /** @brief 全要素の逆方向の積。O(1)。 */
  T all_reverse_prod() const {
    return Parent::get_node(this->root_).backward;
  }

  /** @brief 半開区間 [first,last) の逆方向の積。 */
  T reverse_prod(size_type first, size_type last) const {
    assert(first <= last && last <= size());
    return prod_recursive<true>(Parent::get_node(this->root_), first + 1,
                                last + 1);
  }

  void prod_right(size_type index, const T& value) {
    this->set(index, M::op(this->get(index), value));
  }
  void prod_left(size_type index, const T& value) {
    this->set(index, M::op(value, this->get(index)));
  }

  /** @brief 1要素への作用。 */
  void apply(size_type index, const F& f)
    requires(Traits::lazy)
  {
    apply(index, index + 1, f);
  }
  /** @brief 半開区間 [first,last) への作用。 */
  void apply(size_type first, size_type last, const F& f)
    requires(Traits::lazy)
  {
    assert(first <= last && last <= size());
    apply_recursive(Parent::get_node(this->root_), first + 1, last + 1, f);
  }
  void all_apply(const F& f)
    requires(Traits::lazy)
  {
    apply(0, size(), f);
  }

  /**
   * @brief predicate(prod(left,r)) が真である最大の r。
   * @details 非空区間の積に対して、区間を伸ばしたときに真から偽へのみ
   * 変化することを要求する。predicate(M::e()) は真でなくてもよい。
   */
  template <class Predicate>
  size_type max_right(size_type left, Predicate&& predicate) const {
    assert(left <= size());

    T value = M::e();
    size_type result = left + 1;
    search<false>(Parent::get_node(this->root_), 0, left + 1, size() + 1, value,
                  predicate, result);
    return result - 1;
  }

  /**
   * @brief predicate(prod(l,right)) が真である最小の l。
   * @details 非空区間の積に対して、区間を伸ばしたときに真から偽へのみ
   * 変化することを要求する。predicate(M::e()) は真でなくてもよい。
   */
  template <class Predicate>
  size_type min_left(size_type right, Predicate&& predicate) const {
    assert(right <= size());

    T value = M::e();
    size_type result = right + 1;
    search<true>(Parent::get_node(this->root_), 0, 1, right + 1, value,
                 predicate, result);
    return result - 1;
  }

  template <class Predicate>
  size_type right_bound(size_type left, Predicate&& predicate) const {
    return max_right(left, std::forward<Predicate>(predicate));
  }
  template <class Predicate>
  size_type left_bound(size_type right, Predicate&& predicate) const {
    return min_left(right, std::forward<Predicate>(predicate));
  }

  void assign(size_type count, const T& value = M::e()) {
    Parent::assign(count, value);
  }
  void resize(size_type count, const T& value = M::e()) {
    Parent::resize(count, value);
  }
  using Parent::assign;

  Derived split(size_type index) {
    Derived right;
    static_cast<Parent&>(right) = Parent::split(index);
    return right;
  }
  Derived split(iterator position) {
    return split(static_cast<size_type>(position - this->begin()));
  }
  void merge(Derived& right) {
    Parent::merge(right);
  }

  /** @brief Monoid対応の Setter を返す。 */
  Setter<Derived> operator()(size_type index) {
    assert(index < size());
    return {this->nth(index)};
  }
};

}  // namespace Bplustree
