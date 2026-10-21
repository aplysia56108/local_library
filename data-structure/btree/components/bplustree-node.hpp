#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace Bplustree {

template <class Traits> struct Node;
template <class Traits> struct Leaf;
template <class Traits> struct Internal;

template <class Value, std::size_t Capacity = 32>
struct Traits {
  static_assert(Capacity >= 3);
  using T = Value;
  static constexpr std::size_t kNodeCapacity = Capacity;
  static constexpr std::size_t kNodeHalf = Capacity / 2;
  using Node = Bplustree::Node<Traits>;
  using Leaf = Bplustree::Leaf<Traits>;
  using Internal = Bplustree::Internal<Traits>;
};

enum class NodeKind { kLeaf, kInternal };

template <class Traits>
struct Node {
  using L = Traits::Leaf;
  using I = Traits::Internal;

  I* parent = nullptr;
  std::size_t position = 0;
  std::size_t size = 0;
  std::size_t count = 0;
  std::size_t level = 0;
  const NodeKind kind;

  inline static long long live_nodes = 0;
  inline static long long peak_nodes = 0;

  explicit Node(NodeKind kind) : kind(kind) {
    ++live_nodes;
    peak_nodes = std::max(peak_nodes, live_nodes);
  }
  virtual ~Node() {
    --live_nodes;
  }

  bool is_leaf() const {
    return kind == NodeKind::kLeaf;
  }

  // reverseなど、位置を変更するような遅延伝搬について、実際に位置を変更する。
  // その後何らかの処理のあと、子Node全てにpushする想定。
  void apply_reverse() {
    if (is_leaf()) return static_cast<L*>(this)->apply_reverse();
    static_cast<I*>(this)->apply_reverse();
  }

  // reverseなど、位置を変更するような遅延伝搬のフラグをつける。実際の位置は変更しない。
  void push_reverse() {
    if (is_leaf()) return static_cast<L*>(this)->push_reverse();
    static_cast<I*>(this)->push_reverse();
  }

  // 位置を変更しない遅延伝搬について、実際に適用する。
  // その後何らかの処理のあと、子Node全てにpushする想定。
  void apply_lazy() {
    if (is_leaf()) return static_cast<L*>(this)->apply_lazy();
    static_cast<I*>(this)->apply_lazy();
  }

  // 位置を変更しないような遅延伝搬のフラグをつける。実際には適用しない。
  void push_lazy() {
    if (is_leaf()) return static_cast<L*>(this)->push_lazy();
    static_cast<I*>(this)->push_lazy();
  }

  // 位置に関する遅延評価のみ適用しつつ、Node内のn番目を指すleafとindexを探索する。
  std::pair<L*, std::ptrdiff_t> locate(std::ptrdiff_t index) {
    assert(0 <= index && static_cast<std::size_t>(index) <= size);
    apply_reverse();
    if (is_leaf()) return static_cast<L*>(this)->locate(index);
    return static_cast<I*>(this)->locate(index);
  }

  // 遅延評価をすべて適用しつつ、Node内のn番目を指すleafとindexを探索する。
  std::pair<L*, std::ptrdiff_t> locate_with_lazy(std::ptrdiff_t index) {
    assert(0 <= index && static_cast<std::size_t>(index) <= size);
    apply_reverse();
    apply_lazy();
    if (is_leaf()) return static_cast<L*>(this)->locate(index);
    return static_cast<I*>(this)->locate_with_lazy(index);
  }
};

template <class Traits>
struct Leaf : Traits::Node {
  // Nodecapの考え方について。
  // ここでは、NodeCapはleafやNodeのcapasityであるが、満杯になったら必ず外でsplitを呼ぶことで、内部状態としてはNodeCap-1を保つことにする。この仕様は、split処理を分離するためである。
  static constexpr std::size_t NodeCap = Traits::kNodeCapacity;
  static constexpr std::size_t HalfNodeCap = NodeCap / 2;
  using T = typename Traits::T;
  using N = typename Traits::Node;
  using L = typename Traits::Leaf;
  using I = typename Traits::Internal;
  inline static long long leaf_count = 0;
  inline static long long peak_leaf_count = 0;

  std::array<T, NodeCap> values{};

  Leaf() : N(NodeKind::kLeaf) {
    ++leaf_count;
    peak_leaf_count = std::max(peak_leaf_count, leaf_count);
  }

  ~Leaf() {
    --leaf_count;
  }

  // もしNodeの状態を整合させるための処理。必要であれば後で実装する。
  void after_sync() {
    return;
  }

  bool insert(T value, std::size_t index) {
    assert(index <= this->count);
    assert(this->count < NodeCap);
    for (std::size_t i = this->count; i > index; --i) {
      values[i] = std::move(values[i - 1]);
    }
    values[index] = std::move(value);

    ++this->count;
    ++this->size;

    after_sync();

    return this->count == NodeCap;
  }

  std::unique_ptr<L> split(std::size_t left_count = HalfNodeCap) {
    assert(left_count <= this->count);
    const std::size_t right_count = this->count - left_count;
    auto right = std::make_unique<L>();

    right->count = right_count;
    right->size = right_count;

    for (std::size_t i = 0; i < right_count; ++i) {
      right->values[i] = std::move(values[left_count + i]);
    }
    this->count = left_count;
    this->size = left_count;

    after_sync();
    right->after_sync();

    return right;
  }

  bool erase(std::size_t index) {
    assert(this->count <= NodeCap);
    assert(index < this->count);
    for (std::size_t i = index + 1; i < this->count; ++i) {
      values[i - 1] = std::move(values[i]);
    }
    --this->count;
    --this->size;

    after_sync();
    return this->count == 0;
  }

  // reverseなど、位置を変更するような遅延伝搬について、実際に位置を変更する。
  // その後何らかの処理のあと、子Node全てにpushする想定。
  void apply_reverse() {
    return;
  }

  // reverseなど、位置を変更するような遅延伝搬のフラグをつける。実際の位置は変更しない。
  void push_reverse() {
    return;
  }

  // 位置を変更しない遅延伝搬について、実際に適用する。
  // その後何らかの処理のあと、子Node全てにpushする想定。
  void apply_lazy() {
    return;
  }

  // 位置を変更しないような遅延伝搬のフラグをつける。実際には適用しない。
  void push_lazy() {
    return;
  }

  std::pair<L*, std::ptrdiff_t> locate(std::ptrdiff_t index) {
    assert(0 <= index && static_cast<std::size_t>(index) <= this->size);
    return {static_cast<L*>(this), index};
  }
};

template <class Traits>
struct Internal : Traits::Node {
  static constexpr std::size_t NodeCap = Traits::kNodeCapacity;
  static constexpr std::size_t HalfNodeCap = NodeCap / 2;
  using T = typename Traits::T;
  using N = typename Traits::Node;
  using L = typename Traits::Leaf;
  using I = typename Traits::Internal;
  inline static long long internal_count = 0;
  inline static long long peak_internal_count = 0;
  std::array<std::unique_ptr<N>, NodeCap> children{};

  Internal() : N(NodeKind::kInternal) {
    this->level = 1;
    ++internal_count;
    peak_internal_count = std::max(peak_internal_count, internal_count);
  }
  ~Internal() {
    --internal_count;
  }

  void sync() {
    this->size = 0;
    this->count = 0;
    for (std::size_t i = 0; i < NodeCap; ++i) {
      if (children[i] == nullptr) break;
      children[i]->parent = static_cast<I*>(this);
      children[i]->position = i;
      this->size += children[i]->size;
      this->count++;
    }
    this->level = this->count == 0 ? 1 : children[0]->level + 1;
  }

  void after_sync() {
    return;
  }

  // reverseなど、位置を変更するような遅延伝搬について、実際に位置を変更する。
  // その後子Node全てにpushする。
  void apply_reverse() {
    return;
  }

  // reverseなど、位置を変更するような遅延伝搬のフラグをつける。実際の位置は変更しない。
  void push_reverse() {
    return;
  }

  // 位置を変更しない遅延伝搬について、実際に適用する。
  // その後何らかの処理のあと、子Node全てにpushする想定。
  void apply_lazy() {
    return;
  }

  // 位置を変更しないような遅延伝搬のフラグをつける。実際には適用しない。
  void push_lazy() {
    return;
  }

  bool insert(std::unique_ptr<N> child, std::size_t child_index) {
    assert(child != nullptr);
    assert(child_index <= this->count);
    assert(this->count < NodeCap);

    for (std::size_t i = this->count; i > child_index; --i) {
      children[i] = std::move(children[i - 1]);
    }
    children[child_index] = std::move(child);

    sync();
    after_sync();
    return this->count == NodeCap;
  }

  std::unique_ptr<I> split(std::size_t left_count = HalfNodeCap) {
    assert(left_count <= this->count);
    const std::size_t right_count = this->count - left_count;
    auto right = std::make_unique<I>();

    right->count = right_count;

    for (std::size_t i = 0; i < right_count; ++i) {
      right->children[i] = std::move(children[left_count + i]);
    }
    this->count = left_count;

    sync();
    after_sync();
    right->sync();
    right->after_sync();

    return right;
  }

  bool erase(std::size_t child_index) {
    assert(this->count <= NodeCap);
    assert(child_index < this->count);
    for (std::size_t i = child_index + 1; i < this->count; ++i) {
      children[i - 1] = std::move(children[i]);
    }
    children[this->count - 1].reset();
    --this->count;

    sync();
    after_sync();
    return this->count == 0;
  }

  /**
   * @brief 部分木内の index 番目の要素の位置を返す。
   * @param index この Internal を根とする部分木内での 0-indexed の順位。
   * @return 要素を格納する Leaf と Leaf 内の位置。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  template <bool WithLazy>
  std::pair<L*, std::ptrdiff_t> locate_impl(std::ptrdiff_t index) {
    assert(0 <= index && static_cast<std::size_t>(index) <= this->size);
    assert(this->count > 0);
    const auto locate_child = [](N* child, std::ptrdiff_t offset) {
      if constexpr (WithLazy) return child->locate_with_lazy(offset);
      else return child->locate(offset);
    };
    if (static_cast<std::size_t>(index) == this->size)
      return locate_child(children[this->count - 1].get(),
                          children[this->count - 1]->size);

    for (std::size_t i = 0; i < this->count; ++i) {
      if (static_cast<std::size_t>(index) < children[i]->size) {
        return locate_child(children[i].get(), index);
      }
      index -= static_cast<std::ptrdiff_t>(children[i]->size);
    }
    assert(false);
    return {nullptr, 0};
  }

  std::pair<L*, std::ptrdiff_t> locate(std::ptrdiff_t index) {
    return locate_impl<false>(index);
  }

  std::pair<L*, std::ptrdiff_t> locate_with_lazy(std::ptrdiff_t index) {
    return locate_impl<true>(index);
  }

  // indexの左右子ノードをみて、capacityが許すならくっつける。
  void merge_child(std::size_t index) {
    assert(index < this->count);
    N* left_child = nullptr;
    N* right_child = nullptr;
    std::size_t right_index;
    if (index &&
        children[index - 1]->count + children[index]->count < NodeCap) {
      left_child = children[index - 1].get();
      right_child = children[index].get();
      right_index = index;
    } else if (index + 1 < this->count &&
               children[index + 1]->count + children[index]->count < NodeCap) {
      left_child = children[index].get();
      right_child = children[index + 1].get();
      right_index = index + 1;
    } else
      return;

    left_child->apply_reverse();
    left_child->apply_lazy();
    right_child->apply_reverse();
    right_child->apply_lazy();

    assert(left_child->kind == right_child->kind);
    assert(left_child->level == right_child->level);
    if (left_child->is_leaf()) {
      for (std::size_t i = 0; i < right_child->count; ++i)
        static_cast<L*>(left_child)->values[left_child->count + i] = std::move(
            static_cast<L*>(right_child)->values[i]);
      left_child->count += right_child->count;
      left_child->size = left_child->count;
      static_cast<L*>(left_child)->after_sync();
    } else {
      for (std::size_t i = 0; i < right_child->count; ++i)
        static_cast<Internal*>(left_child)->children[left_child->count + i] =
            std::move(static_cast<Internal*>(right_child)
                          ->children[i]);
      auto* left = static_cast<Internal*>(left_child);
      left->sync();
      left->after_sync();
    }

    erase(right_index);
  };

  // 端の部分木のchildを再帰的にまとめる。
  // split時に疎なnodeをなるべく作らないための工夫
  void shrink(const bool right_end) {
    if (this->count == 0) return;
    if (this->count == 1) {
      if (children[0]->is_leaf()) return;
      return static_cast<Internal*>(children[0].get())->shrink(right_end);
    }
    std::size_t i = right_end ? this->count - 2 : 0;
    N* left_ = children[i].get();
    N* right_ = children[i + 1].get();

    if (left_->count + right_->count >= NodeCap) {
      if (left_->is_leaf()) return;
      auto* edge = right_end ? right_ : left_;
      return static_cast<Internal*>(edge)->shrink(right_end);
    }

    merge_child(i + 1);

    if (!left_->is_leaf()) {
      static_cast<Internal*>(left_)->shrink(right_end);
    }
  }
};
}  // namespace Bplustree
