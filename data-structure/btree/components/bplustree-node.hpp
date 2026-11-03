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

// using value_type = long long;
// constexpr std::size_t Capacity = 32;
template <class value_type, std::size_t Capacity = 32>
struct Traits {
  static_assert(Capacity >= 3);
  using T = value_type;
  static constexpr std::size_t kNodeCapacity = Capacity;
  static constexpr std::size_t kNodeHalf = Capacity / 2;
  using Node = Bplustree::Node<Traits>;
  using Leaf = Bplustree::Leaf<Traits>;
  using Internal = Bplustree::Internal<Traits>;
  static const bool IsReversible = true;
  static const bool IsMergable = true;
};

/** @brief 子 Node の実体型を識別する。 */
// enum class NodeKind { kLeaf, kInternal };

/**
 * @brief Leaf / Internal が共有する親へのリンクとノード種別。
 * @details parent と position は親 Internal の children 配列での所属を表す。
 * 根だけは parent == nullptr とする。構造変更後は Internal::sync() で更新する。
 */
template <class Traits>
struct Node {
  using L = Traits::Leaf;
  using I = Traits::Internal;

  I* parent = nullptr;
  std::size_t position = 0;
  // const NodeKind kind;

  inline static long long live_nodes = 0;
  inline static long long peak_nodes = 0;

  explicit Node() {
    ++live_nodes;
    peak_nodes = std::max(peak_nodes, live_nodes);
  }
  virtual ~Node() {
    --live_nodes;
  }
};

/**
 * @brief 1要素を保持する Leaf。要素ごとに Node を分け、Leaf*
 * を安定した識別子にする。
 * @details Internal の子ポインタを並べ替えても Leaf
 * 自身のアドレスは変わらない。 ただし、その Leaf
 * を消去した場合や木を再構築した場合は無効になる。
 */
template <class Traits>
struct Leaf : Traits::Node {
  using T = typename Traits::T;
  using N = typename Traits::Node;

 private:
  T value;

 public:
  inline static long long leaf_count = 0;
  inline static long long peak_leaf_count = 0;

  Leaf(const T& value) : N(), value(value) {
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

  T& get() {
    return value;
  }

  void push_lazy() {
    return;
  }
};

/**
 * @brief 子 Node の所有権と部分木の構造情報を管理する内部ノード。
 * @details level == 0 の子は Leaf、それ以外の子は Internal。
 * level は構築時に確定し、通常状態では count < NodeCap を保つ。
 * 子の所有権は children の unique_ptr が持つ。
 */
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
  using I = typename Traits::Internal;
  inline static long long internal_count = 0;
  inline static long long peak_internal_count = 0;
  std::size_t count = 0;
  std::size_t size = 0;
  const std::size_t level;
  bool is_reversed = false;
  std::array<std::unique_ptr<N>, NodeCap> children{};

  Internal(std::size_t level = 0) : N(), level(level) {
    ++internal_count;
    peak_internal_count = std::max(peak_internal_count, internal_count);
  }
  ~Internal() {
    --internal_count;
  }

  I* get_child(std::size_t index) const {
    return static_cast<typename Traits::Internal*>(children[index].get());
  }

  std::size_t child_size(std::size_t index) const {
    if (level == 0) return 1;
    return get_child(index)->size;
  }

  std::size_t child_count(std::size_t index) const {
    if (level == 0) return 1;
    return get_child(index)->count;
  }

  /**
   * @brief 子配列から size / count と各子の parent / position を再計算する。
   * @details children[0, count) は null のない連続領域とする。
   * 挿入直後の count == NodeCap の一時状態にも対応する。
   */
  void sync() {
    // insert / split の内部処理でも使うため、満杯の一時状態も同期する。
    size = 0;
    count = 0;
    for (std::size_t i = 0; i < NodeCap; ++i) {
      if (children[i] == nullptr) break;
      children[i]->parent = static_cast<I*>(this);
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

  /**
   * @brief 構造情報を同期し、派生 Internal の追加情報も更新する。
   * @details after_sync() は Traits::Internal へ静的にディスパッチする。
   */
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
    static_cast<typename Traits::Internal*>(this)->apply_lazy_impl();
  }

  void apply_lazy_impl() {
    return;
  }

  // 位置を変更しないような遅延伝搬のフラグをつける。実際には適用しない。
  void push_lazy() {
    return;
  }

  /**
   * @brief この Node に残った reverse / lazy を子へ伝搬する。
   * @details 子配列の順序や所有権を変更する前に呼び出す。
   * 派生クラスの集約値は論理状態と整合していることを前提とする。
   */
  void materialize() {
    apply_reverse();
    apply_lazy();
  }

  /**
   * @brief child を child_index に挿入する。
   * @details 呼び出し側は必要な遅延情報を事前に伝搬しておく。
   * @return 満杯になり、直ちに split() する必要がある場合 true。
   * @warning true を返した直後は count == NodeCap の一時状態となる。
   */
  bool insert(std::unique_ptr<N> child, std::size_t child_index) {
    assert(child != nullptr);
    assert(child_index <= this->count);
    assert(this->count < NodeCap);

    for (std::size_t i = count; i > child_index; --i) {
      children[i] = std::move(children[i - 1]);
    }
    children[child_index] = std::move(child);
    ++count;
    if (count == NodeCap) return true;
    refresh();
    return false;
  }

  /**
   * @brief 先頭 left_count 個の子を自身に残し、残りを同じ level
   * の右兄弟へ移す。
   * @details 子の移動前に呼び出し側が必要な遅延情報を伝搬する。
   * 分割処理中は片側の count が 0 になることも許容する。
   * 完成した木に空 Internal を残さないことは上位の処理が保証する。
   * @return 分割後の右 Internal。左側はこの Node を再利用する。
   */
  std::unique_ptr<I> split(std::size_t left_count = HalfNodeCap) {
    assert(count <= NodeCap);
    assert(left_count <= count);
    assert(left_count < NodeCap && count - left_count < NodeCap);
    const std::size_t right_count = count - left_count;
    auto right = std::make_unique<I>(level);

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

    children[count - 1].reset();
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
  template <bool WithLazy = false>
  L* locate(std::size_t index) {
    apply_reverse();
    if constexpr (WithLazy) apply_lazy();

    if (level == 0) return static_cast<L*>(children[index].get());

    for (std::size_t i = 0; i < count; ++i) {
      if (index < child_size(i)) {
        return get_child(i)->template locate<WithLazy>(index);
      }
      index -= child_size(i);
    }
    assert(false);
    return nullptr;
  }

  /**
   * @brief index 番目の Internal 子を、容量に余裕のある隣接兄弟へ吸収する。
   * @details level > 0、index <
   * count、親の配置変更に関する遅延は伝搬済みとする。
   * 統合可能なら指定した子だけを破棄し、兄弟の生存を保証する。
   * 指定した子の子孫は兄弟へ移動するため、子孫 Node 自体は破棄しない。
   * 統合できない場合は何もしない。統合後は子の position が変わりうる。
   */
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
  /**
   * @brief 容量が許す場合、自身を左右いずれかの兄弟 Internal に統合する。
   * @details 統合に成功した場合、この Node 自身は破棄されうる。
   * この呼び出しによって、ほかの Node は破棄しない。
   * 親・兄弟・子孫へのポインタは維持されるが、position は更新されうる。
   * 呼び出し後は this にアクセスしないこと。
   * 根に対しては何もしない。
   */
  void integrate() {
    if (this->parent == nullptr) return;
    this->parent->integrate_child(this->position);
  };
};

}  // namespace Bplustree
