#pragma once

#include "data-structure/btree/components/bplustree-aggregate.hpp"
#include "common/monoid.hpp"

/** Dynamic lazy segment tree. Action::composition(f,g) applies g then f.
 * mapping(f,x,n) must distribute over the monoid, including reversed products.
 * ReverseInvariant allows the reverse aggregate to reuse the forward aggregate.
 */
template <class Monoid, class Action, bool ReverseInvariant = false,
          std::size_t NodeCapacity = 15>
class Bplustree_LazySegtree
    : public Bplustree::AggregateTree<
          Bplustree_LazySegtree<Monoid, Action, ReverseInvariant, NodeCapacity>,
          Monoid, Action, ReverseInvariant, NodeCapacity> {
  using Parent = Bplustree::AggregateTree<Bplustree_LazySegtree, Monoid, Action,
                                          ReverseInvariant, NodeCapacity>;
 public:
  using Parent::Parent;
};
template <class T, class F, auto Mapping, auto Composition>
struct Action {
  using action_type = F;

  static T mapping(const F& f, const T& x, std::size_t size) {
    return Mapping(f, x, size);
  }

  static F composition(const F& f, const F& g) {
    return Composition(f, g);
  }
};

/**
 * @brief `Op` と `E` を直接指定して構築する B+ tree セグメント木。
 */
template <class T, class F, auto Op, auto E, auto Mapping, auto Composition,
          bool ReverseInvariant = false, std::size_t NodeCapacity = 64>
using BLazySegtree =
    Bplustree_LazySegtree<Monoid<T, Op, E>, Action<T, F, Mapping, Composition>,
                          ReverseInvariant, NodeCapacity>;

/**
 * @brief 単位元を指定せずに構築する B+ tree セグメント木。
 * @details
 * 各要素を通常の `T` として扱いつつ、内部では人工的な単位元を使用する。
 */
template <class T, class F, auto Op, auto Mapping, auto Composition,
          bool ReverseInvariant = false, std::size_t NodeCapacity = 15>
using BLazySegtreeWithoutE =
    Bplustree_LazySegtree<MonoidWithoutE<T, Op>,
                          Action<T, F, Mapping, Composition>, ReverseInvariant,
                          NodeCapacity>;

inline constexpr auto AddMapping = [](const long long& f, const long long& x,
                                      std::size_t size) {
  return size == 0 ? x : x + f;
};

inline constexpr auto ChangeMapping = [](const long long& f, const long long& x,
                                         std::size_t size) {
  return size == 0 ? x : f;
};

inline constexpr auto AddSumMapping = [](const long long& f, const long long& x,
                                         std::size_t size) {
  return x + f * static_cast<long long>(size);
};

inline constexpr auto ChangeSumMapping =
    [](const long long& f, const long long&, std::size_t size) {
      return f * static_cast<long long>(size);
    };

inline constexpr auto AddComposition = [](const long long& f,
                                          const long long& g) {
  return f + g;
};

inline constexpr auto ChangeComposition = [](const long long& f,
                                             const long long&) {
  return f;
};

using AddMinLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return std::min(a, b);
                 },
                 []() {
                   return library_inf;
                 },
                 AddMapping, AddComposition, true>;

using AddMaxLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return std::max(a, b);
                 },
                 []() {
                   return -library_inf;
                 },
                 AddMapping, AddComposition, true>;

using ChangeMinLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return std::min(a, b);
                 },
                 []() {
                   return library_inf;
                 },
                 ChangeMapping, ChangeComposition, true>;

using ChangeMaxLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return std::max(a, b);
                 },
                 []() {
                   return -library_inf;
                 },
                 ChangeMapping, ChangeComposition, true>;

using AddSumLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return a + b;
                 },
                 []() {
                   return 0LL;
                 },
                 AddSumMapping, AddComposition, true>;

using ChangeSumLazySegtree =
    BLazySegtree<long long, long long,
                 [](const long long& a, const long long& b) {
                   return a + b;
                 },
                 []() {
                   return 0LL;
                 },
                 ChangeSumMapping, ChangeComposition, true>;

template <class mint, std::size_t NodeCapacity = 15>
using AffineSumLazySegtree =
    BLazySegtree<mint, std::pair<mint, mint>,
                 [](auto a, auto b) {
                   return a + b;
                 },
                 []() {
                   return 0;
                 },
                 [](auto f, auto x, std::size_t size) {
                   return f.first * x + size * f.second;
                 },
                 [](auto f, auto g) -> std::pair<mint, mint> {
                   return {f.first * g.first, f.first * g.second + f.second};
                 },
                 true, NodeCapacity>;
