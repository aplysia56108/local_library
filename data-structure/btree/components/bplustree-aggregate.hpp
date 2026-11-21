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

/** @brief 遅延作用を使わない Segtree 用のタグ型。 */
struct NoAction {
  using action_type = long long;
};

/**
 * @brief 集約モノイドと任意の遅延作用を追加する Traits。
 * @tparam M モノイド（value_type、op、e を提供）。
 * @tparam A 作用（action_type、mapping、composition を提供）または NoAction。
 * @tparam R 反転で積が不変なら true。
 * @tparam C Internal の子 ID 配列容量。
 */
template <class M, class A, bool R, std::size_t C>
struct AggregateTraits {
  using T = typename M::value_type;
  using F = typename A::action_type;
  static constexpr std::size_t kNodeCapacity = C;
  static constexpr pos_type kNodeHalf = C / 2;
  static constexpr bool lazy = !std::same_as<A, NoAction>;
  static T identity() {
    return M::e();
  }
  using NodeBase = Bplustree::NodeBase<AggregateTraits>;
  struct Node;
  struct Leaf;
  using UpperNode = Bplustree::UpperNode<AggregateTraits>;
  using BottomNode = Bplustree::BottomNode<AggregateTraits>;
  using Internal = Node;  // 旧 API が想定する共通メタデータの型
};

/**
 * @brief 1個の実要素（または番兵値）を保持する Leaf。
 * @details 自身では区間積・遅延タグを保持しない。作用の伝搬を受けたら
 * mapping(f, value, 1) により値を更新する。
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
 * @brief 順方向・逆方向の積と未伝搬の作用を保持する Internal。
 * @details level==0 の子は Leaf、それ以外は Internal。
 * R=true では逆方向積が順方向積と等しい前提で演算を省略する。
 * 反転フラグ・pending が残っていても積は論理状態と一致させる。
 */
template <class M, class A, bool R, std::size_t C>
struct AggregateTraits<M, A, R, C>::Node
    : Bplustree::Node<AggregateTraits<M, A, R, C>> {
  using Traits = AggregateTraits<M, A, R, C>;
  using Parent = Bplustree::Node<Traits>;
  using Pools = Bplustree::NodePools<Traits>;
  using T = typename Traits::T;
  using F = typename Traits::F;
  using Leaf = typename Traits::Leaf;

  using Parent::Parent;

  T forward = M::e();
  T backward = M::e();
  std::optional<F> pending;

  /**
   * @brief 子の値・区間積を順方向 / 逆方向に畳み込んで再計算する。
   * @details 未伝搬の is_reversed / pending が残る場合も、
   * それらを反映した論理状態の積を更新する。size は変更しない。
   */
  void refresh_metadata() {
    forward = backward = M::e();
    if (this->level == 0) {
      for (std::size_t i = 0; i < this->count; ++i)
        forward = M::op(forward, Pools::leaves()[this->children[i]].get());
      if constexpr (R) {
        backward = forward;
      } else {
        for (std::size_t i = this->count; i > 0; --i)
          backward = M::op(backward, Pools::leaves()[this->children[i - 1]].get());
      }
    } else {
      for (std::size_t i = 0; i < this->count; ++i)
        forward = M::op(forward, Pools::with_internal(this->children[i],
                              [](const auto& child) { return child.forward; }));
      if constexpr (R) {
        backward = forward;
      } else {
        for (std::size_t i = this->count; i > 0; --i)
          backward = M::op(backward, Pools::with_internal(this->children[i - 1],
                              [](const auto& child) { return child.backward; }));
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

  /**
   * @brief ノード全体の作用を pending に合成し、集約値だけ即時更新する。
   * @details pending は既存作用の後に f を適用するため composition(f, old)。
   */
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

  /** @brief 反転時に順方向積と逆方向積を交換する（R=true なら不要）。 */
  void push_reverse_impl() {
    if constexpr (!R) std::swap(forward, backward);
  }

  /**
   * @brief pending を子へ渡し、Leaf なら値を直接更新する。
   * @details 作用は合成順序を維持して伝搬し、伝搬後 pending を解除する。
   */
  void apply_lazy() {
    if constexpr (Traits::lazy) {
      if (!pending) return;
      F f = *pending;
      pending.reset();
      if (this->level == 0) {
        for (std::size_t i = 0; i < this->count; ++i)
          Pools::leaves()[this->children[i]].push_lazy(f);
      } else {
        for (std::size_t i = 0; i < this->count; ++i)
          Pools::with_internal(this->children[i],
              [&](auto& child) { child.push_lazy(f); });
      }
    }
  }
};


/**
 * @brief 1要素1Leaf の Base に区間積・境界探索・任意の遅延作用を追加する。
 * @tparam Derived 公開派生コンテナの型（split の返り値と Setter に使用）。
 * @tparam M モノイド型。
 * @tparam A 作用型、または NoAction。
 * @tparam R true なら逆順の積も順方向積と一致することを仮定する。
 * @tparam C Internal の子 ID 配列容量。
 * @details 前後番兵の値は M::e()。集約値は Internal のみが保持する。
 * Leaf は作用を伝搬された時点で値を書き換える。
 *
 * ### 計算量
 * 区間積・区間作用・境界探索: O(C * 木の高さ)（演算は O(1)）。
 * all_prod / all_reverse_prod: O(1)。
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
  using Epoch = IteratorEpoch<Traits>;

  using Access = NodeAccess<Traits>;

  /** @brief 具象型へ1回分岐して区間積を再帰計算する。 */
  template <bool Reversed = false>
  static T prod_recursive(id_type node_id, std::size_t first, std::size_t last) {
    if (first == last) return M::e();
    return Access::with(node_id, [&](auto& node) -> T {
      if (first == 0 && last == node.size) {
        if constexpr (Reversed) return node.backward;
        return node.forward;
      }
      node.materialize();
      T result = M::e();
      if (node.level == 0) {
        if constexpr (Reversed) {
          for (std::size_t i = last; i > first; --i)
            result = M::op(result, Pools::leaves()[node.children[i - 1]].get());
        } else {
          for (std::size_t i = first; i < last; ++i)
            result = M::op(result, Pools::leaves()[node.children[i]].get());
        }
        return result;
      }
      if constexpr (Reversed) {
        std::size_t offset = node.size;
        for (std::size_t i = node.count; i > 0; --i) {
          const id_type child_id = node.children[i - 1];
          const std::size_t length = Access::get(child_id).size;
          offset -= length;
          if (first < offset + length && offset < last)
            result = M::op(result, prod_recursive<true>(
                child_id, first > offset ? first - offset : 0,
                std::min(last - offset, length)));
          if (offset <= first) break;
        }
      } else {
        std::size_t offset = 0;
        for (std::size_t i = 0; i < node.count; ++i) {
          const id_type child_id = node.children[i];
          const std::size_t length = Access::get(child_id).size;
          if (first < offset + length && offset < last)
            result = M::op(result, prod_recursive(
                child_id, first > offset ? first - offset : 0,
                std::min(last - offset, length)));
          offset += length;
          if (offset >= last) break;
        }
      }
      return result;
    });
  }

  /** @brief 作用を具象ノードへ再帰適用し、メタデータを更新する。 */
  static void apply_recursive(id_type node_id, std::size_t first,
                              std::size_t last, const F& f)
    requires(Traits::lazy)
  {
    if (first == last) return;
    Access::with(node_id, [&](auto& node) {
      if (first == 0 && last == node.size) {
        node.push_lazy(f);
        return;
      }
      node.materialize();
      if (node.level == 0) {
        for (std::size_t i = first; i < last; ++i)
          Pools::leaves()[node.children[i]].push_lazy(f);
      } else {
        std::size_t offset = 0;
        for (std::size_t i = 0; i < node.count; ++i) {
          const id_type child_id = node.children[i];
          const std::size_t length = Access::get(child_id).size;
          if (first < offset + length && offset < last)
            apply_recursive(child_id, first > offset ? first - offset : 0,
                            std::min(last - offset, length), f);
          offset += length;
          if (offset >= last) break;
        }
      }
      node.refresh_metadata();
    });
  }

  // predicate が初めて偽になる部分木だけを深掘りして境界を探す。
  template <bool Leftward, class Predicate>
  static bool search(id_type node_id, std::size_t offset,
                     std::size_t first, std::size_t last, T& value,
                     Predicate& predicate, std::size_t& result) {
    return Access::with(node_id, [&](auto& node) -> bool {
      const std::size_t end = offset + node.size;
      if (end <= first || last <= offset) return true;
      if (first <= offset && end <= last) {
        T next = Leftward ? M::op(node.forward, value)
                          : M::op(value, node.forward);
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
          const T& x = Pools::leaves()[node.children[i]].get();
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
          const id_type child_id = node.children[i - 1];
          pos -= Access::get(child_id).size;
          if (!search<true>(child_id, pos, first, last,
                            value, predicate, result))
            return false;
        }
      } else {
        std::size_t pos = offset;
        for (std::size_t i = 0; i < node.count; ++i) {
          const id_type child_id = node.children[i];
          if (!search<false>(child_id, pos, first, last,
                             value, predicate, result))
            return false;
          pos += Access::get(child_id).size;
        }
      }
      return true;
    });
  }

 public:
  using Monoid = M;
  using Action = A;
  using Parent::Parent;
  using Parent::size;
  using typename Parent::iterator;
  using typename Parent::size_type;

  AggregateTree() = default;
  /** @brief count 個の実要素をモノイド単位元 M::e() で初期化する。 */
  explicit AggregateTree(size_type count) : Parent(count, M::e()) {}

  /** @brief 全実要素の順方向の積を取得する。O(1)。番兵は単位元。 */
  T all_prod() const {
    return Access::with(this->root_, [](const auto& node) -> T { return node.forward; });
  }

  /** @brief 半開区間 [first,last) の順方向のモノイド積。空区間は M::e()。 */
  T prod(size_type first, size_type last) const {
    assert(first <= last && last <= size());
    return prod_recursive(this->root_, first + 1, last + 1);
  }

  /** @brief 全実要素の逆方向の積。O(1)。非可換モノイドにも対応。 */
  T all_reverse_prod() const {
    return Access::with(this->root_, [](const auto& node) -> T { return node.backward; });
  }

  /** @brief 半開区間 [first,last) を逆方向に畳み込んだ積。 */
  T reverse_prod(size_type first, size_type last) const {
    assert(first <= last && last <= size());
    return prod_recursive<true>(this->root_, first + 1, last + 1);
  }

  /** @brief 要素 index を op(現在値, value) に点更新する。 */
  void prod_right(size_type index, const T& value) {
    this->set(index, M::op(this->get(index), value));
  }
  /** @brief 要素 index を op(value, 現在値) に点更新する。 */
  void prod_left(size_type index, const T& value) {
    this->set(index, M::op(value, this->get(index)));
  }

  /** @brief index の1要素に作用を適用する（Lazy 対応時のみ）。 */
  void apply(size_type index, const F& f)
    requires(Traits::lazy)
  {
    apply(index, index + 1, f);
  }
  /** @brief 半開区間 [first,last) への作用。空区間なら何もしない。 */
  void apply(size_type first, size_type last, const F& f)
    requires(Traits::lazy)
  {
    assert(first <= last && last <= size());
    if (first == last) return;
    apply_recursive(this->root_, first + 1, last + 1, f);
    Epoch::invalidate_value();
  }
  /** @brief 全実要素へ作用を適用する（Lazy 対応時のみ）。 */
  void all_apply(const F& f)
    requires(Traits::lazy)
  {
    apply(0, size(), f);
  }

  /**
   * @brief predicate(prod(left,r)) が真である最大の r。
   * @details 非空区間の積に対して、区間を伸ばすと真から偽へのみ
   * 変化する述語を要求する。predicate(M::e()) は呼び出さない。
   * ### 計算量
   * O(C * 木の高さ)（モノイドと述語は O(1)）。
   */
  template <class Predicate>
  size_type max_right(size_type left, Predicate&& predicate) const {
    assert(left <= size());

    T value = M::e();
    size_type result = left + 1;
    search<false>(this->root_, 0, left + 1, size() + 1, value,
                  predicate, result);
    return result - 1;
  }

  /**
   * @brief predicate(prod(l,right)) が真である最小の l。
   * @details 非空区間の積に対して、区間を左へ伸ばすと真から偽へのみ
   * 変化する述語を要求する。predicate(M::e()) は呼び出さない。
   * ### 計算量
   * O(C * 木の高さ)（モノイドと述語は O(1)）。
   */
  template <class Predicate>
  size_type min_left(size_type right, Predicate&& predicate) const {
    assert(right <= size());

    T value = M::e();
    size_type result = right + 1;
    search<true>(this->root_, 0, 1, right + 1, value,
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

  /** @brief index 以降を分離して Derived 型で返す。実要素 Leaf ID は維持。 */
  Derived split(size_type index) {
    Derived right;
    static_cast<Parent&>(right) = Parent::split(index);
    return right;
  }
  /** @brief position 以降を分離して Derived 型で返す。 */
  Derived split(iterator position) {
    return split(static_cast<size_type>(position - this->begin()));
  }
  /**
   * @brief right の実要素を末尾へ連結して空にする。
   * @details 実要素 Leaf をコピー・再挿入しない。構造変更された内部ノードの
   * 集約値は必要に応じて再計算される。
   */
  void merge(Derived& right) {
    Parent::merge(right);
  }

  /** @brief index 番目の要素用の Monoid / Action 対応 Setter を返す。 */
  Setter<Derived> operator()(size_type index) {
    assert(index < size());
    return {this->nth(index)};
  }
};

}  // namespace Bplustree
