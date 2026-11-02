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

// template <class Traits>
struct Node;
// template <class Traits>
struct Leaf;
// template <class Traits>
struct Internal;

// template <class Value, std::size_t Capacity = 32>
using value_type = long long;
constexpr std::size_t Capacity = 32;
struct Traits {
  static_assert(Capacity >= 3);
  using T = value_type;
  static constexpr std::size_t kNodeCapacity = Capacity;
  static constexpr std::size_t kNodeHalf = Capacity / 2;
  using Node = Bplustree::Node;
  using Leaf = Bplustree::Leaf;
  using Internal = Bplustree::Internal;
  static const bool IsReversible = true;
  static const bool IsMergable = true;
};

enum class NodeKind { kLeaf, kInternal };

// template <class Traits>
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

// template <class Traits>
struct Leaf : Traits::Node {
  using T = typename Traits::T;
  using N = typename Traits::Node;

 private:
  T value;

 public:
  inline static long long leaf_count = 0;
  inline static long long peak_leaf_count = 0;

  Leaf(const T& value) : N(NodeKind::kLeaf), value(value) {
    ++leaf_count;
    peak_leaf_count = std::max(peak_leaf_count, leaf_count);
  }

  ~Leaf() {
    --leaf_count;
  }

  void set(const T& v) {
    value = v;
  }

  const T& get() const {
    return value;
  }
};

// template <class Traits>
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
  inline static long long internal_count = 0;
  inline static long long peak_internal_count = 0;
  std::size_t count = 0;
  std::size_t size = 0;
  const std::size_t level;
  bool is_reversed = false;
  std::array<std::unique_ptr<N>, NodeCap> children{};

  Internal(std::size_t level = 0) : N(NodeKind::kInternal), level(level) {
    ++internal_count;
    peak_internal_count = std::max(peak_internal_count, internal_count);
  }
  ~Internal() {
    --internal_count;
  }

  Internal* get_child(std::size_t index) const {
    return static_cast<Internal*>(children[index].get());
  }

  std::size_t child_size(std::size_t index) const {
    if (level == 0) return 1;
    return get_child(index)->size;
  }

  std::size_t child_count(std::size_t index) const {
    if (level == 0) return 1;
    return get_child(index)->count;
  }

  void sync() {
    // insert / split の内部処理でも使うため、満杯の一時状態も同期する。
    size = 0;
    count = 0;
    for (std::size_t i = 0; i < NodeCap; ++i) {
      if (children[i] == nullptr) break;
      children[i]->parent = this;
      children[i]->position = i;
      if (level == 0)
        ++size;
      else
        size += child_size(i);
      ++count;
    }
  }

  void after_sync() {
    return;
  }

  void refresh() {
    sync();
    static_cast<typename Traits::Internal*>(this)->after_sync();
  }

  // reverseなど、位置を変更するような遅延伝搬について、実際に位置を変更する。
  // その後子Node全てにpushする。
  void apply_reverse() {
    if (!this->is_reversed) return;
    std::reverse(children.begin(), children.begin() + count);
    this->is_reversed = false;
    if (level)
      for (std::size_t i = 0; i < count; ++i) get_child(i)->push_reverse();
    refresh();
  }

  // reverseなど、位置を変更するような遅延伝搬のフラグをつける。実際の位置は変更しない。
  void push_reverse() {
    this->is_reversed = !this->is_reversed;
    static_cast<typename Traits::Internal*>(this)->push_reverse_impl();
  }

  void push_reverse_impl() {
    return;
  };

  // 位置を変更しない遅延伝搬について、実際に適用する。
  // その後何らかの処理のあと、子Node全てにpushする想定。
  void apply_lazy() {
    if (!(static_cast<typename Traits::Internal*>(this)->apply_lazy_impl()))
      return;
    if (level)
      for (std::size_t i = 0; i < count; ++i)
        static_cast<typename Traits::Internal*>(get_child(i))->push_lazy();
  }

  bool apply_lazy_impl() {
    return false;
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
    auto right = std::make_unique<Internal>(level);

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
    for (std::size_t i = child_index + 1; i < count; ++i)
      children[i - 1] = std::move(children[i]);

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

    if (level == 0) return static_cast<L*>(children[index].get());

    for (std::size_t i = 0; i < count; ++i) {
      if (index < child_size(i)) {
        return get_child(i)->locate<WithLazy>(index);
      }
      index -= child_size(i);
    }
    assert(false);
    return nullptr;
  }

  void integrate_child(std::size_t index) {
    if (index && child_count(index - 1) + child_count(index) < NodeCap) {
      auto left = get_child(index - 1);
      auto right = get_child(index);
      left->materialize();
      right->materialize();
      for (std::size_t i = 0; i < right->count; ++i)
        left->children[left->count + i] = std::move(right->children[i]);
      left->refresh();
    } else if (index + 1 < count &&
               child_count(index + 1) + child_count(index) < NodeCap) {
      auto left = get_child(index);
      auto right = get_child(index + 1);
      left->materialize();
      right->materialize();
      for (std::size_t i = right->count; i > 0; --i)
        right->children[left->count + i - 1] =
            std::move(right->children[i - 1]);
      for (std::size_t i = 0; i < left->count; ++i)
        right->children[i] = std::move(left->children[i]);
      right->refresh();
    } else
      return;

    erase(index);
  }
  // 左右のノードをみて、capacityが許すならくっつける。
  void integrate() {
    if (this->parent == nullptr) return;
    parent->integrate_child(position);
  };
};

}  // namespace Bplustree
