#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <type_traits>
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
  static T identity() { return M::e(); }
  struct Node;
  struct Leaf;
  struct Internal;
};

/** @brief 順方向・逆方向の積と遅延作用を共有する Node。 */
template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Node
    : Bplustree::Node<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Bplustree::Node<Traits>::Node;

  T forward = M::e();
  T backward = M::e();
  std::optional<F> pending;

  /** @brief 部分木全体への作用。Leaf は値に直接反映する。 */
  void apply_action(const F& f) requires(Traits::lazy);
};

/** @brief 1要素の集約値を持つ Leaf。 */
template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Leaf
    : Bplustree::Leaf<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Parent = Bplustree::Leaf<Traits>;
  using T = typename Traits::T;

  explicit Leaf(const T& value) : Parent(value) {
    this->forward = this->backward = value;
  }

  void set(const T& value) {
    Parent::set(value);
    this->forward = this->backward = value;
  }
};

/** @brief 集約値の再計算・反転・遅延伝搬を提供する Internal。 */
template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Internal
    : Bplustree::Internal<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Parent = Bplustree::Internal<Traits>;

  using Parent::Parent;

  void after_sync() {
    this->forward = this->backward = M::e();
    for (std::size_t i = 0; i < this->count; ++i)
      this->forward = M::op(this->forward, this->children[i]->forward);
    if constexpr (R) {
      this->backward = this->forward;
    } else {
      for (std::size_t i = this->count; i > 0; --i)
        this->backward = M::op(this->backward, this->children[i - 1]->backward);
      if (this->is_reversed) std::swap(this->forward, this->backward);
    }
    if constexpr (Traits::lazy) {
      if (this->pending) {
        this->forward = A::mapping(*this->pending, this->forward, this->size);
        if constexpr (R)
          this->backward = this->forward;
        else
          this->backward = A::mapping(*this->pending, this->backward, this->size);
      }
    }
  }

  void push_reverse_impl() {
    if constexpr (!R) std::swap(this->forward, this->backward);
  }

  bool apply_lazy_impl() {
    if constexpr (Traits::lazy) {
      if (this->pending) {
        const F f = *this->pending;
        this->pending.reset();
        for (std::size_t i = 0; i < this->count; ++i)
          this->children[i]->apply_action(f);
      }
    }
    return false;
  }

 private:
  using F = typename Traits::F;
};

template <class M, class A, bool R, std::size_t C>
void AggregateTraits<M, A, R, C>::Node::apply_action(const F& f)
    requires(Traits::lazy) {
  if (this->is_leaf()) {
    auto* leaf = static_cast<typename Traits::Leaf*>(this);
    leaf->set(A::mapping(f, leaf->get(), 1));
    return;
  }
  auto* internal = static_cast<typename Traits::Internal*>(this);
  this->forward = A::mapping(f, this->forward, internal->size);
  if constexpr (R)
    this->backward = this->forward;
  else
    this->backward = A::mapping(f, this->backward, internal->size);
  this->pending = this->pending ? A::composition(f, *this->pending) : f;
}

/**
 * @brief 1要素1Leafの B+ tree にモノイド積と遅延作用を追加する。
 * @details 番兵の値はモノイド単位元とし、公開する位置は番兵を除く0-indexed。
 * 集約と遅延作用の反転は Traits::Internal のフックを利用する。
 */
template <class Derived, class M, class A, bool R, std::size_t C>
class AggregateTree : public Base<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Parent = Base<Traits>;
  using Node = typename Traits::Node;
  using Leaf = typename Traits::Leaf;
  using Internal = typename Traits::Internal;
  using T = typename Traits::T;
  using F = typename Traits::F;

  static std::size_t node_size(const Node* node) {
    return node->is_leaf() ? 1 : static_cast<const Internal*>(node)->size;
  }

  static T product(Node* node, std::size_t first, std::size_t last,
                   bool reverse) {
    if (first == last) return M::e();
    if (first == 0 && last == node_size(node))
      return reverse ? node->backward : node->forward;
    if (node->is_leaf()) return static_cast<Leaf*>(node)->get();

    auto* internal = static_cast<Internal*>(node);
    internal->materialize();
    T result = M::e();
    std::size_t offset = 0;
    if (!reverse) {
      for (std::size_t i = 0; i < internal->count; ++i) {
        Node* child = internal->children[i].get();
        const std::size_t length = node_size(child);
        if (first < offset + length && offset < last)
          result = M::op(result, product(child,
              first > offset ? first - offset : 0,
              std::min(last - offset, length), false));
        offset += length;
        if (offset >= last) break;
      }
    } else {
      offset = internal->size;
      for (std::size_t i = internal->count; i > 0; --i) {
        Node* child = internal->children[i - 1].get();
        const std::size_t length = node_size(child);
        offset -= length;
        if (first < offset + length && offset < last)
          result = M::op(result, product(child,
              first > offset ? first - offset : 0,
              std::min(last - offset, length), true));
        if (offset <= first) break;
      }
    }
    return result;
  }

  static void update(Node* node, std::size_t first, std::size_t last,
                     const F& f) requires(Traits::lazy) {
    if (first == last) return;
    if (first == 0 && last == node_size(node)) {
      node->apply_action(f);
      return;
    }
    auto* internal = static_cast<Internal*>(node);
    internal->materialize();
    std::size_t offset = 0;
    for (std::size_t i = 0; i < internal->count; ++i) {
      Node* child = internal->children[i].get();
      const std::size_t length = node_size(child);
      if (first < offset + length && offset < last)
        update(child, first > offset ? first - offset : 0,
               std::min(last - offset, length), f);
      offset += length;
      if (offset >= last) break;
    }
    internal->refresh();
  }

  // Scan a logical interval, accepting whole subtrees when possible.
  template <bool Leftward, class Predicate>
  static bool search(Node* node, std::size_t offset,
                     std::size_t first, std::size_t last,
                     T& value, Predicate& predicate, std::size_t& result) {
    const std::size_t end = offset + node_size(node);
    if (end <= first || last <= offset) return true;
    if (first <= offset && end <= last) {
      T next = Leftward ? M::op(node->forward, value)
                        : M::op(value, node->forward);
      if (predicate(next)) {
        value = std::move(next);
        result = Leftward ? offset : end;
        return true;
      }
    }

    if (node->is_leaf()) return false;
    auto* internal = static_cast<Internal*>(node);
    internal->materialize();
    if constexpr (Leftward) {
      std::size_t pos = end;
      for (std::size_t i = internal->count; i > 0; --i) {
        Node* child = internal->children[i - 1].get();
        pos -= node_size(child);
        if (!search<Leftward>(child, pos, first, last,
                              value, predicate, result)) return false;
      }
    } else {
      std::size_t pos = offset;
      for (std::size_t i = 0; i < internal->count; ++i) {
        Node* child = internal->children[i].get();
        if (!search<Leftward>(child, pos, first, last,
                              value, predicate, result)) return false;
        pos += node_size(child);
      }
    }
    return true;
  }

 public:
  using Monoid = M;
  using Action = A;
  using typename Parent::size_type;
  using typename Parent::iterator;
  using Parent::Parent;
  using Parent::size;

  AggregateTree() = default;

  /** @brief すべての要素の積。O(1)。 */
  T all_prod() const { return this->root_->forward; }

  /** @brief 半開区間 [first,last) の順方向の積。 */
  T prod(size_type first, size_type last) const {
    assert(first <= last && last <= size());
    return product(this->root_.get(), first + 1, last + 1, false);
  }

  /** @brief 半開区間 [first,last) の逆方向の積。 */
  T reverse_prod(size_type first, size_type last) const {
    assert(first <= last && last <= size());
    return product(this->root_.get(), first + 1, last + 1, true);
  }

  void prod_right(size_type index, const T& value) {
    this->set(index, M::op(this->get(index), value));
  }
  void prod_left(size_type index, const T& value) {
    this->set(index, M::op(value, this->get(index)));
  }
  void apply_right(size_type index, const T& value) {
    prod_right(index, value);
  }
  void apply_left(size_type index, const T& value) {
    prod_left(index, value);
  }
  void apply(size_type index, const T& value) requires(!Traits::lazy) {
    prod_right(index, value);
  }

  /** @brief 1要素への作用。 */
  void apply(size_type index, const F& f) requires(Traits::lazy) {
    apply(index, index + 1, f);
  }
  /** @brief 半開区間 [first,last) への作用。 */
  void apply(size_type first, size_type last, const F& f)
      requires(Traits::lazy) {
    assert(first <= last && last <= size());
    update(this->root_.get(), first + 1, last + 1, f);
  }
  void all_apply(const F& f) requires(Traits::lazy) {
    apply(0, size(), f);
  }

  /** @brief predicate(prod(left,r)) が真である最大の r。 */
  template <class Predicate>
  size_type max_right(size_type left, Predicate&& predicate) const {
    assert(left <= size());
    assert(predicate(M::e()));
    T value = M::e();
    size_type result = left + 1;
    search<false>(this->root_.get(), 0, left + 1, size() + 1,
                  value, predicate, result);
    return result - 1;
  }

  /** @brief predicate(prod(l,right)) が真である最小の l。 */
  template <class Predicate>
  size_type min_left(size_type right, Predicate&& predicate) const {
    assert(right <= size());
    assert(predicate(M::e()));
    T value = M::e();
    size_type result = right + 1;
    search<true>(this->root_.get(), 0, 1, right + 1,
                 value, predicate, result);
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
  using Parent::split;

  Derived split(size_type index) {
    Derived right;
    static_cast<Parent&>(right) = Parent::split(index);
    return right;
  }
  Derived split(iterator position) {
    return split(static_cast<size_type>(position - this->begin()));
  }
  void merge(Derived& right) { Parent::merge(right); }

  /** @brief Monoid対応の Setter を返す。 */
  Setter<Derived> operator()(size_type index) {
    assert(index < size());
    return {this->nth(index)};
  }

  void debug_node_stats() const {
    std::cerr << "nodes: " << Node::live_nodes << " / " << Node::peak_nodes << '\n';
    std::cerr << "leaf: " << Leaf::leaf_count << " / " << Leaf::peak_leaf_count << '\n';
    std::cerr << "internal: " << Internal::internal_count << " / " << Internal::peak_internal_count << '\n';
  }
};

}  // namespace Bplustree
