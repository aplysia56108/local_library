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

template <class Traits>
struct Node;
template <class Traits>
struct Leaf;
template <class Traits>
struct Internal;

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
};

template <class Traits>
struct Leaf : Traits::Node {
  using T = typename Traits::T;
  using N = typename Traits::Node;

 private:
  T value;

 public:
  inline static long long leaf_count = 0;
  inline static long long peak_leaf_count = 0;

  Leaf(T& value) : N(NodeKind::kLeaf), value(value) {
    ++leaf_count;
    peak_leaf_count = std::max(peak_leaf_count, leaf_count);
  }

  ~Leaf() {
    --leaf_count;
  }

  void set(T& v) {
    value = v;
  }

  const T& get() {
    return value;
  }
};

template <class Traits>
struct Internal : Traits::Node {
  // 通常状態では count < NodeCap を保つ。
  // insert が true を返した直後だけ count == NodeCap になり、
  // 呼び出し側は他の操作を行う前に必ず split する。
  // split は両側を count < NodeCap に戻す。空き1枠は挿入時の一時領域。
  static constexpr std::size_t NodeCap = Traits::kNodeCapacity;
  static constexpr std::size_t HalfNodeCap = NodeCap / 2;
  using T = typename Traits::T;
  using N = typename Traits::Node;
  using L = typename Traits::Leaf;
  const bool is_bottom;
  inline static long long internal_count = 0;
  inline static long long peak_internal_count = 0;
  std::size_t count = 0;
  std::size_t size = 0;
  std::size_t level = 0;
  bool is_reversed = false;
  std::array<std::unique_ptr<N>, NodeCap> children{};

  Internal(bool is_bottom) : N(NodeKind::kInternal), is_bottom(is_bottom) {
    ++internal_count;
    peak_internal_count = std::max(peak_internal_count, internal_count);
  }
  ~Internal() {
    --internal_count;
  }

  std::size_t child_size(std::size_t index) {
    if (is_bottom) return 1;
    return const_cast<Internal*>(children[index].get())->size;
  }

  std::size_t child_count(std::size_t index) {
    if (is_bottom) return 1;
    return const_cast<Internal*>(children[index].get())->count;
  }

  void sync() {
    // insert / split の内部処理でも使うため、満杯の一時状態も同期する。
    size = 0;
    count = 0;
    for (std::size_t i = 0; i < NodeCap; ++i) {
      if (children[i] == nullptr) break;
      children[i]->parent = this;
      children[i]->position = i;
      if (is_bottom)
        ++size;
      else
        size += child_size(i);
      ++count;
    }
    if (!is_bottom) level = const_cast<Internal*>(children[0])->level + 1;
  }

  void after_sync() {
    return;
  }

  void refresh() {
    sync();
    return after_sync();
  }

  // reverseなど、位置を変更するような遅延伝搬について、実際に位置を変更する。
  // その後子Node全てにpushする。
  void apply_reverse() {
    if (!this->is_reversed) return;
    std::reverse(children.begin(), children.begin() + count);
    this->is_reversed = false;
    for (std::size_t i = 0; i < count; ++i) children[i]->push_reverse();
    refresh();
  }

  // reverseなど、位置を変更するような遅延伝搬のフラグをつける。実際の位置は変更しない。
  void push_reverse() {
    this->is_reversed = !this->is_reversed;
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

  void materialize() {
    apply_reverse();
    apply_lazy();
  }

  bool insert(std::unique_ptr<N> child, std::size_t child_index) {
    assert(child != nullptr);
    assert(child_index <= this->count);
    assert(this->count < NodeCap);

    for (std::size_t i = count; i > child_index; --i) {
      children[i] = std::move(children[i - 1]);
    }
    children[child_index] = std::move(child);
    if (count + 1 == NodeCap) return true;
    refresh();
    return false;
  }

  std::unique_ptr<Internal> split(std::size_t left_count = HalfNodeCap) {
    assert(count <= NodeCap);
    assert(left_count <= count);
    assert(left_count < NodeCap && count - left_count < NodeCap);
    const std::size_t right_count = count - left_count;
    auto right = std::make_unique<Internal>();

    for (std::size_t i = 0; i < right_count; ++i) {
      right->children[i] = std::move(children[left_count + i]);
    }
    refresh();
    right->refresh();

    return right;
  }

  bool erase(std::size_t child_index) {
    assert(this->count < NodeCap);
    assert(child_index < count);
    for (std::size_t i = child_index + 1; i < count; ++i) {
      children[i - 1] = std::move(children[i]);
    }
    refresh();
    return this->count == 0;
  }

  /**
   * @brief 部分木内の index 番目の要素の位置を返す。
   * @param index この Internal を根とする部分木内での順位。[-1, size]。
   * -1 は最左Leafの先頭直前、size は最右Leafの末尾を表す。
   * @return 要素を格納する Leaf と Leaf 内の位置。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  template <bool WithLazy>
  L* locate(std::size_t index) {
    apply_reverse();
    if constexpr (WithLazy) apply_lazy();

    if (is_bottom) return children[index];

    for (std::size_t i = 0; i < count; ++i) {
      if (index < child_size(i)) {
        return children[i]->locate<WithLazy>(index);
      }
      index -= child_size(i);
    }
    assert(false);
    return nullptr;
  }

  // 左右のノードをみて、capacityが許すならくっつける。
  void merge_child(std::size_t index) {
    Internal* left_child = nullptr;
    Internal* right_child = nullptr;
    std::size_t right_index;
    if (index && child_count(index - 1) + child_count(index) < NodeCap) {
      left_child = children[index - 1].get();
      right_child = children[index].get();
      right_index = index;
    } else if (index + 1 < this->count &&
               child_count(index + 1) + child_count(index) < NodeCap) {
      left_child = children[index].get();
      right_child = children[index + 1].get();
      right_index = index + 1;
    } else
      return;

    left_child->materialize();
    right_child->materialize();

    assert(left_child->kind == right_child->kind);
    assert(left_child->level == right_child->level);

    for (std::size_t i = 0; i < right_child->count; ++i)
      left_child->children[left_child->count + i] =
          std::move(right_child->children[i]);
    left_child->refresh();

    erase(right_index);
  };
};

}  // namespace Bplustree
