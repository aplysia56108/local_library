#pragma once

#include <iostream>
#include <optional>

#include "data-structure/btree/components/bplustree-base.hpp"

namespace Bplustree {

// The non-lazy tree shares aggregation and traversal with the lazy tree.
struct NoAction {
  using action_type = int;
};

template <class Monoid, class Action, bool ReverseInvariant, std::size_t Capacity>
struct AggregateTraits {
  using T = typename Monoid::value_type;
  using F = typename Action::action_type;
  static constexpr std::size_t kNodeCapacity = Capacity;
  static constexpr bool lazy = !std::same_as<Action, NoAction>;
  struct Node;
  struct Leaf;
  struct Internal;
};

template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Node
    : Bplustree::Node<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Bplustree::Node<Traits>::Node;
  T forward = M::e(), backward = M::e();
  std::optional<F> pending;

  void apply_action(const F& f) requires(Traits::lazy) {
    if (this->size == 0) return;
    forward = A::mapping(f, forward, this->size);
    if constexpr (R) backward = forward;
    else backward = A::mapping(f, backward, this->size);
    pending = pending ? A::composition(f, *pending) : f;
  }
  // after_sync starts from physical values/children, then restores logical state.
  void evaluate() {
    if constexpr (R) backward = forward;
    else if (this->is_reversed) std::swap(forward, backward);
    if constexpr (Traits::lazy) {
      if (pending) {
        forward = A::mapping(*pending, forward, this->size);
        if constexpr (R) backward = forward;
        else backward = A::mapping(*pending, backward, this->size);
      }
    }
  }
};

template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Leaf
    : Bplustree::Leaf<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  void after_sync() {
    this->forward = this->backward = M::e();
    for (std::size_t i = 0; i < this->count; ++i)
      this->forward = M::op(this->forward, this->values[i]);
    if constexpr (!R)
      for (std::size_t i = this->count; i > 0; --i)
        this->backward = M::op(this->backward, this->values[i - 1]);
    this->evaluate();
  }
  void push_reverse() {
    Bplustree::Leaf<Traits>::push_reverse();
    if constexpr (!R) std::swap(this->forward, this->backward);
  }
  void apply_lazy() {
    if constexpr (Traits::lazy) {
      if (!this->pending) return;
      for (std::size_t i = 0; i < this->count; ++i)
        this->values[i] = A::mapping(*this->pending, this->values[i], 1);
      this->pending.reset();
    }
  }
};

template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Internal
    : Bplustree::Internal<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  void after_sync() {
    this->forward = this->backward = M::e();
    for (std::size_t i = 0; i < this->count; ++i)
      this->forward = M::op(this->forward, this->children[i]->forward);
    if constexpr (!R)
      for (std::size_t i = this->count; i > 0; --i)
        this->backward = M::op(this->backward, this->children[i - 1]->backward);
    this->evaluate();
  }
  void push_reverse() {
    Bplustree::Internal<Traits>::push_reverse();
    if constexpr (!R) std::swap(this->forward, this->backward);
  }
  void apply_lazy() {
    if constexpr (Traits::lazy) {
      if (!this->pending) return;
      for (std::size_t i = 0; i < this->count; ++i)
        this->children[i]->apply_action(*this->pending);
      this->pending.reset();
    }
  }
};

// CRTP keeps setters and split results typed as the public container.
template <class Derived, class M, class A, bool R, std::size_t C>
class AggregateTree : public Base<AggregateTraits<M, A, R, C>, true> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Parent = Base<Traits, true>;
  using typename Parent::Node;
  using typename Parent::Leaf;
  using typename Parent::Internal;
  using T = typename M::value_type;
  using F = typename A::action_type;

  static T product(Node* node, std::size_t first, std::size_t last, bool reverse) {
    if (first == last) return M::e();
    if (first == 0 && last == node->size)
      return reverse ? node->backward : node->forward;
    Parent::materialize(node);
    T result = M::e();
    if (node->is_leaf()) {
      auto* leaf = static_cast<Leaf*>(node);
      for (std::size_t i = first; i < last; ++i) {
        const T& value = leaf->values[reverse ? last - 1 - (i - first) : i];
        result = M::op(result, value);
      }
    } else {
      auto* internal = static_cast<Internal*>(node);
      std::size_t offset = 0;
      for (std::size_t i = 0; i < node->count; ++i) {
        Node* child = internal->children[i].get();
        if (first < offset + child->size && offset < last) {
          T value = product(child, first > offset ? first - offset : 0,
                            std::min(last - offset, child->size), reverse);
          result = reverse ? M::op(value, result) : M::op(result, value);
        }
        offset += child->size;
        if (offset >= last) break;
      }
    }
    return result;
  }

  static void update(Node* node, std::size_t first, std::size_t last, const F& f)
      requires(Traits::lazy) {
    if (first == last) return;
    if (first == 0 && last == node->size) return node->apply_action(f);
    Parent::materialize(node);
    if (node->is_leaf()) {
      auto* leaf = static_cast<Leaf*>(node);
      for (std::size_t i = first; i < last; ++i)
        leaf->values[i] = A::mapping(f, leaf->values[i], 1);
    } else {
      auto* internal = static_cast<Internal*>(node);
      std::size_t offset = 0;
      for (std::size_t i = 0; i < node->count; ++i) {
        Node* child = internal->children[i].get();
        if (first < offset + child->size && offset < last)
          update(child, first > offset ? first - offset : 0,
                 std::min(last - offset, child->size), f);
        offset += child->size;
        if (offset >= last) break;
      }
    }
    Parent::refresh(node);
  }

  // Skip accepted subtrees and descend only at the failing boundary.
  template <bool Leftward, class Predicate>
  static bool search(Node* node, std::size_t offset, std::size_t boundary,
                     T& value, Predicate& predicate, std::size_t& result) {
    const std::size_t end = offset + node->size;
    if constexpr (Leftward) {
      if (offset >= boundary) return true;
    } else {
      if (end <= boundary) return true;
    }
    const bool whole = Leftward ? end <= boundary : offset >= boundary;
    if (whole) {
      T next = Leftward ? M::op(node->forward, value) : M::op(value, node->forward);
      if (predicate(next)) {
        value = std::move(next);
        result = Leftward ? offset : end;
        return true;
      }
    }
    Parent::materialize(node);
    if (node->is_leaf()) {
      auto* leaf = static_cast<Leaf*>(node);
      for (std::size_t j = 0; j < node->count; ++j) {
        std::size_t i = Leftward ? node->count - 1 - j : j;
        if (Leftward ? offset + i >= boundary : offset + i < boundary) continue;
        T next = Leftward ? M::op(leaf->values[i], value) : M::op(value, leaf->values[i]);
        if (!predicate(next)) return false;
        value = std::move(next);
        result = offset + i + (Leftward ? 0 : 1);
      }
    } else {
      auto* internal = static_cast<Internal*>(node);
      std::size_t position = Leftward ? end : offset;
      for (std::size_t j = 0; j < node->count; ++j) {
        Node* child = internal->children[Leftward ? node->count - 1 - j : j].get();
        if constexpr (Leftward) position -= child->size;
        if (!search<Leftward>(child, position, boundary, value, predicate, result))
          return false;
        if constexpr (!Leftward) position += child->size;
      }
    }
    return true;
  }

 public:
  using typename Parent::size_type;
  using typename Parent::iterator;
  using Parent::size;
  using Parent::get;
  using Parent::set;
  using Parent::begin;
  using setter_type = Setter<Derived, T>;
  AggregateTree() = default;
  template <std::input_iterator It>
  AggregateTree(It first, It last) : Parent(first, last) {}
  explicit AggregateTree(size_type count) : Parent(count, M::e()) {}
  AggregateTree(size_type count, const T& value) : Parent(count, value) {}
  AggregateTree(std::initializer_list<T> values) : Parent(values) {}
  template <std::input_iterator It>
  void assign(It first, It last) { Parent::assign(first, last); }
  void assign(size_type count, const T& value) { Parent::assign(count, value); }
  void assign(size_type count) { Parent::assign(count, M::e()); }
  void resize(size_type count, const T& value = M::e()) { Parent::resize(count, value); }
  setter_type operator()(size_type index) {
    assert(index < size());
    return {static_cast<Derived&>(*this), index};
  }
  void prod_right(size_type index, const T& value) {
    set(index, M::op(get(index), value));
  }
  void prod_left(size_type index, const T& value) {
    set(index, M::op(value, get(index)));
  }
  void apply_right(size_type index, const T& value) { prod_right(index, value); }
  void apply_left(size_type index, const T& value) { prod_left(index, value); }
  void apply(size_type index, const T& value) requires(!Traits::lazy) {
    prod_right(index, value);
  }
  void apply(size_type index, const F& f) requires(Traits::lazy) {
    assert(index < size());
    update(this->root_.get(), index, index + 1, f);
  }
  void apply(size_type first, size_type last, const F& f) requires(Traits::lazy) {
    assert(first <= last && last <= size());
    update(this->root_.get(), first, last, f);
  }
  void all_apply(const F& f) requires(Traits::lazy) { this->root_->apply_action(f); }
  T prod(size_type first, size_type last) const {
    assert(first <= last && last <= size());
    return product(this->root_.get(), first, last, false);
  }
  T reverse_prod(size_type first, size_type last) const {
    assert(first <= last && last <= size());
    return product(this->root_.get(), first, last, true);
  }
  T all_prod() const { return this->root_->forward; }
  template <class Predicate>
  size_type max_right(size_type left, Predicate&& predicate) const {
    assert(left <= size());
    if (left == size()) return left;
    T value = M::e();
    size_type result = left;
    search<false>(this->root_.get(), 0, left, value, predicate, result);
    return result;
  }
  template <class Predicate>
  size_type min_left(size_type right, Predicate&& predicate) const {
    assert(right <= size());
    if (right == 0) return 0;
    T value = M::e();
    size_type result = right;
    search<true>(this->root_.get(), 0, right, value, predicate, result);
    return result;
  }
  template <class Predicate>
  size_type right_bound(size_type left, Predicate&& f) const {
    return max_right(left, std::forward<Predicate>(f));
  }
  template <class Predicate>
  size_type left_bound(size_type right, Predicate&& f) const {
    return min_left(right, std::forward<Predicate>(f));
  }
  Derived split(size_type index) {
    Derived right;
    static_cast<Parent&>(right) = Parent::split(index);
    return right;
  }
  Derived split(iterator position) { return split(position - begin()); }
  void merge(Derived& right) { Parent::merge(right); }
  void debug_node_stats() const {
    std::cerr << "nodes: " << Node::live_nodes << " / " << Node::peak_nodes << '\n';
    std::cerr << "leaf: " << Leaf::leaf_count << " / " << Leaf::peak_leaf_count << '\n';
    std::cerr << "internal: " << Internal::internal_count << " / " << Internal::peak_internal_count << '\n';
  }
};
}  // namespace Bplustree
