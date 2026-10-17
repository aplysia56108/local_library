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
#include <iostream>
#include <initializer_list>
#include <numeric>

#include "common/monoid.hpp"

/**
 * @brief B+ tree 上に構築した動的セグメント木。
 * @tparam Monoid モノイドを表す型。
 * `value_type`, `op(lhs, rhs)`, `e()` を持つ必要がある。
 * @tparam NodeCapacity 1 Node が保持できる要素または子 Node の最大数。
 *
 * @details
 * 要素を順序付き列として保持し、添字アクセス、任意位置への挿入・削除、
 * 区間反転、区間積、点更新、セグ木上の二分探索をサポートする。
 *
 * 要素の集約値として `Monoid::op()` による積を各 Node に保持する。
 * 非可換モノイドにも対応し、区間反転のために正順・逆順の積をそれぞれ管理する。
 *
 * `operator[]` および Iterator から得られる要素は読み取り専用である。
 * 点更新には `set()` または `operator()` が返す Setter を使用する。
 *
 * `NodeCapacity` を大きくすると木は浅くなりやすい一方、
 * 各 Node に対する操作量と Node 自体のサイズは増加する。
 */
template <class Monoid, std::size_t NodeCapacity = 64>
class Bplustree_Segtree {
  using T = Monoid::value_type;
  static_assert(NodeCapacity >= 3);

  static constexpr std::size_t kNodeCapacity = NodeCapacity;
  static constexpr std::size_t kNodeHalf = kNodeCapacity / 2;

  enum class NodeKind { kLeaf, kInternal };

  struct Leaf;
  struct Internal;

  struct Node {
    Internal* parent = nullptr;
    T evaluated_monoid = Monoid::e();
    T reverse_evaluated_monoid = Monoid::e();
    std::size_t position = 0;
    std::size_t size = 0;
    std::size_t count = 0;
    bool is_reversed = false;
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

    std::pair<Leaf*, std::size_t> nth(std::size_t index) {
      assert(index <= size);
      if (is_leaf()) return static_cast<Leaf*>(this)->nth(index);
      return static_cast<Internal*>(this)->nth(index);
    }

    T node_prod(std::size_t first, std::size_t last) {
      assert(first <= last && last <= this->size);
      if (first == last) return Monoid::e();
      if (first == 0 && last == this->size) return evaluated_monoid;
      push_reverse();
      if (is_leaf()) return static_cast<Leaf*>(this)->prod(first, last);
      return static_cast<Internal*>(this)->prod(first, last);
    }

    T node_reverse_prod(std::size_t first, std::size_t last) {
      assert(first <= last && last <= this->size);
      if (first == last) return Monoid::e();
      if (first == 0 && last == this->size) return reverse_evaluated_monoid;
      push_reverse();
      if (is_leaf()) return static_cast<Leaf*>(this)->reverse_prod(first, last);
      return static_cast<Internal*>(this)->reverse_prod(first, last);
    }

    void apply_reverse() {
      if (is_leaf()) return static_cast<Leaf*>(this)->apply_reverse();
      return static_cast<Internal*>(this)->apply_reverse();
    }

    void push_reverse() {
      if (is_leaf()) return static_cast<Leaf*>(this)->push_reverse();
      return static_cast<Internal*>(this)->push_reverse();
    }
  };

  struct Leaf final : Node {
    inline static long long leaf_count = 0;
    inline static long long peak_leaf_count = 0;

    std::array<T, kNodeCapacity> values{};

    Leaf() : Node(NodeKind::kLeaf) {
      ++leaf_count;
      peak_leaf_count = std::max(peak_leaf_count, leaf_count);
    }

    ~Leaf() {
      --leaf_count;
    }

    bool insert(T value, std::size_t index) {
      assert(index <= this->count);
      assert(this->count < kNodeCapacity);
      for (std::size_t i = this->count; i > index; --i) {
        values[i] = std::move(values[i - 1]);
      }
      values[index] = std::move(value);

      ++this->count;
      sync();

      return this->count == kNodeCapacity;
    }

    std::unique_ptr<Leaf> split() {
      assert(this->count == kNodeCapacity);

      const std::size_t left_count = kNodeHalf;
      const std::size_t right_count = kNodeCapacity - left_count;
      auto right = std::make_unique<Leaf>();

      right->count = right_count;

      for (std::size_t i = 0; i < right_count; ++i) {
        right->values[i] = std::move(values[left_count + i]);
      }
      this->count = left_count;
      sync();
      right->sync();

      return right;
    }

    bool erase(std::size_t index) {
      assert(this->count < kNodeCapacity);
      assert(index < this->count);
      for (std::size_t i = index + 1; i <= this->count; ++i) {
        values[i - 1] = std::move(values[i]);
      }
      --this->count;
      sync();
      return this->count == 0;
    }

    void apply_reverse() {
      this->is_reversed ^= true;
      using std::swap;
      swap(this->evaluated_monoid, this->reverse_evaluated_monoid);
    }

    void push_reverse() {
      if (!this->is_reversed) return;
      this->is_reversed = false;
      std::reverse(values.begin(), values.begin() + this->count);
    }

    std::pair<Leaf*, std::size_t> nth(std::size_t index) {
      push_reverse();
      return {this, index};
    }

    T prod(std::size_t first, std::size_t last) {
      T result = Monoid::e();
      for (std::size_t i = first; i < last; ++i)
        result = Monoid::op(result, values[i]);
      return result;
    }

    T reverse_prod(std::size_t first, std::size_t last) {
      T result = Monoid::e();
      for (std::size_t i = last; i > first; --i)
        result = Monoid::op(result, values[i - 1]);
      return result;
    }

    void sync() {
      this->size = this->count;
      this->evaluated_monoid = Monoid::e();
      this->reverse_evaluated_monoid = Monoid::e();

      for (std::size_t i = 0; i < this->count; ++i) {
        this->evaluated_monoid = Monoid::op(this->evaluated_monoid, values[i]);
      }
      for (std::size_t i = this->count; i > 0; --i) {
        this->reverse_evaluated_monoid =
            Monoid::op(this->reverse_evaluated_monoid, values[i - 1]);
      }
    }
  };

  struct Internal final : Node {
    inline static long long internal_count = 0;
    inline static long long peak_internal_count = 0;
    std::array<std::unique_ptr<Node>, kNodeCapacity> children{};

    Internal() : Node(NodeKind::kInternal) {
      ++internal_count;
      peak_internal_count = std::max(peak_internal_count, internal_count);
    }
    ~Internal() {
      --internal_count;
    }

    void sync() {
      this->size = 0;
      this->evaluated_monoid = Monoid::e();
      this->reverse_evaluated_monoid = Monoid::e();

      for (std::size_t i = 0; i < this->count; ++i) {
        assert(children[i] != nullptr);
        children[i]->parent = this;
        children[i]->position = i;
        this->size += children[i]->size;
        this->evaluated_monoid =
            Monoid::op(this->evaluated_monoid, children[i]->evaluated_monoid);
      }
      for (std::size_t i = this->count; i > 0; --i) {
        this->reverse_evaluated_monoid =
            Monoid::op(this->reverse_evaluated_monoid,
                       children[i - 1]->reverse_evaluated_monoid);
      }
    }

    bool insert(std::unique_ptr<Node> child, std::size_t child_index) {
      assert(child != nullptr);
      assert(child_index <= this->count);
      assert(this->count < kNodeCapacity);

      for (std::size_t i = this->count; i > child_index; --i) {
        children[i] = std::move(children[i - 1]);
      }
      children[child_index] = std::move(child);
      ++this->count;
      sync();
      return this->count == kNodeCapacity;
    }

    std::unique_ptr<Internal> split() {
      assert(this->count == kNodeCapacity);

      const std::size_t left_count = kNodeCapacity / 2;
      const std::size_t right_count = kNodeCapacity - left_count;
      auto right = std::make_unique<Internal>();

      right->count = right_count;

      for (std::size_t i = 0; i < right_count; ++i) {
        right->children[i] = std::move(children[left_count + i]);
      }
      this->count = left_count;

      sync();
      right->sync();

      return right;
    }

    bool erase(std::size_t child_index) {
      assert(this->count < kNodeCapacity);
      assert(child_index < this->count);
      for (std::size_t i = child_index + 1; i <= this->count; ++i) {
        children[i - 1] = std::move(children[i]);
      }
      --this->count;

      sync();
      return this->count == 0;
    }

    void apply_reverse() {
      this->is_reversed ^= true;
      using std::swap;
      swap(this->evaluated_monoid, this->reverse_evaluated_monoid);
    }

    void push_reverse() {
      if (!this->is_reversed) return;

      this->is_reversed = false;
      std::reverse(children.begin(), children.begin() + this->count);
      for (std::size_t i = 0; i < this->count; ++i) {
        children[i]->apply_reverse();
        children[i]->position = i;
      }
    }

    /**
     * @brief 部分木内の index 番目の要素の位置を返す。
     * @param index この Internal を根とする部分木内での 0-indexed の順位。
     * @return 要素を格納する Leaf と Leaf 内の位置。
     *
     * ### 計算量
     * O(NodeCapacity log_{NodeCapacity} N)
     */
    std::pair<Leaf*, std::size_t> nth(std::size_t index) {
      push_reverse();
      if (index == this->size)
        return children[this->count - 1]->nth(children[this->count - 1]->size);

      for (std::size_t i = 0; i < this->count; ++i) {
        if (index < children[i]->size) {
          return children[i]->nth(index);
        }
        index -= children[i]->size;
      }
      assert(false);
      return {nullptr, 0};
    }

    T prod(std::size_t first, std::size_t last) {
      T result = Monoid::e();
      std::size_t offset = 0;
      for (std::size_t i = 0; i < this->count; ++i) {
        Node* child = children[i].get();
        const std::size_t child_first = offset;
        const std::size_t child_last = offset + child->size;
        if (last <= child_first) break;

        if (child_last <= first) {
          offset = child_last;
          continue;
        }
        const std::size_t l = std::max(first, child_first) - child_first;
        const std::size_t r = std::min(last, child_last) - child_first;
        result = Monoid::op(result, child->node_prod(l, r));
        offset = child_last;
      }
      return result;
    }

    T reverse_prod(std::size_t first, std::size_t last) {
      T result = Monoid::e();
      std::size_t offset = this->size;
      for (std::size_t i = this->count; i > 0; --i) {
        Node* child = children[i - 1].get();
        const std::size_t child_last = offset;
        const std::size_t child_first = offset - child->size;
        if (child_last <= first) break;

        if (last <= child_first) {
          offset = child_first;
          continue;
        }
        const std::size_t l = std::max(first, child_first) - child_first;
        const std::size_t r = std::min(last, child_last) - child_first;
        result = Monoid::op(result, child->node_reverse_prod(l, r));
        offset = child_first;
      }
      return result;
    }

    // 端の部分木のchildを再帰的にまとめる。
    // split時に疎なnodeをなるべく作らないための工夫
    void shrink(const bool right_end) {
      push_reverse();
      if (this->count == 1) {
        if (children[0]->is_leaf()) return;
        return static_cast<Internal*>(children[0].get())->shrink(right_end);
      }
      std::size_t i = right_end ? this->count - 2 : 0;
      Node* left_ = children[i].get();
      Node* right_ = children[i + 1].get();

      if (left_->count + right_->count >= kNodeCapacity) {
        if (left_->is_leaf()) return;
        auto* edge = right_end ? right_ : left_;
        return static_cast<Internal*>(edge)->shrink(right_end);
      }

      left_->push_reverse();
      right_->push_reverse();

      if (left_->is_leaf()) {
        auto* left = static_cast<Leaf*>(left_);
        auto* right = static_cast<Leaf*>(right_);
        for (std::size_t i = 0; i < right->count; ++i)
          left->values[i + left->count] = std::move(right->values[i]);
        left->count += right->count;
        left->sync();
      } else {
        auto* left = static_cast<Internal*>(left_);
        auto* right = static_cast<Internal*>(right_);
        for (std::size_t i = 0; i < right->count; ++i)
          left->children[i + left->count] = std::move(right->children[i]);
        left->count += right->count;
        left->sync();
      }
      erase(i + 1);

      if (!left_->is_leaf()) {
        static_cast<Internal*>(left_)->shrink(right_end);
      }
    }
  };

  /**
   * @brief B+ tree 上の要素を指すイテレータ。
   * @details 通常要素は `(leaf_, index_)` で表す。最右 Leaf の
   * `index_ == leaf_->count` は `end()` を表す。
   *
   * C++20 の `std::random_access_iterator` コンセプトを満たす。
   * ただし木構造上、任意距離の移動やイテレータ差分は厳密には O(1) ではなく
   * O(NodeCapacity log_{NodeCapacity} N) となり得る。
   */
  template <bool IsConst>
  class Iterator {
    friend class Bplustree_Segtree;

   private:
    Leaf* leaf_ = nullptr;
    std::size_t index_ = 0;

    std::pair<Node*, std::ptrdiff_t> root_position() const {
      if (leaf_->parent == nullptr) return {leaf_, index_};

      std::ptrdiff_t index = static_cast<std::ptrdiff_t>(index_);
      Internal* parent = leaf_->parent;
      std::size_t position = leaf_->position;

      while (true) {
        for (std::size_t i = 0; i < position; ++i)
          index += static_cast<std::ptrdiff_t>(parent->children[i]->size);

        if (parent->parent == nullptr) break;

        position = parent->position;
        parent = parent->parent;
      }
      return {parent, index};
    }

    void advance(std::ptrdiff_t offset) {
      std::ptrdiff_t to_index = static_cast<std::ptrdiff_t>(index_) + offset;
      if (0 <= to_index &&
          to_index < static_cast<std::ptrdiff_t>(leaf_->count)) {
        index_ = static_cast<std::size_t>(to_index);
        return;
      }
      if (leaf_->parent == nullptr) {
        assert(to_index == static_cast<std::ptrdiff_t>(leaf_->count));
        index_ = static_cast<std::size_t>(to_index);
        return;
      }
      auto [root, root_index] = root_position();
      auto [leaf, index] =
          root->nth(static_cast<std::size_t>(root_index + offset));
      leaf_ = leaf;
      index_ = index;
    }

   public:
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;

    Iterator() = default;
    Iterator(Leaf* node, std::size_t index) : leaf_(node), index_(index) {}

    reference operator*() const {
      assert(leaf_ != nullptr);
      assert(index_ < leaf_->count);
      return leaf_->values[index_];
    }

    pointer operator->() const {
      return &operator*();
    }

    // iterator -> const_iterator の変換
    operator Iterator<true>() const
      requires(!IsConst)
    {
      return {leaf_, index_};
    }

    Iterator& operator++() {
      advance(1);
      return *this;
    }

    Iterator operator++(signed) {
      Iterator result = *this;
      ++*this;
      return result;
    }

    Iterator& operator--() {
      advance(-1);
      return *this;
    }

    Iterator operator--(signed) {
      Iterator result = *this;
      --*this;
      return result;
    }

    Iterator& operator+=(std::ptrdiff_t offset) {
      advance(offset);
      return *this;
    }

    Iterator& operator-=(std::ptrdiff_t offset) {
      return *this += -offset;
    }

    friend Iterator operator+(Iterator it, std::ptrdiff_t offset) {
      return it += offset;
    }

    friend Iterator operator+(std::ptrdiff_t offset, Iterator it) {
      return it += offset;
    }

    friend Iterator operator-(Iterator it, std::ptrdiff_t offset) {
      return it -= offset;
    }

    friend std::ptrdiff_t operator-(const Iterator& lhs, const Iterator& rhs) {
      return lhs.root_position().second - rhs.root_position().second;
    }

    reference operator[](difference_type offset) const {
      return *(*this + offset);
    }

    friend std::strong_ordering operator<=>(const Iterator& lhs,
                                            const Iterator& rhs) {
      return (lhs - rhs) <=> 0;
    }

    friend bool operator==(const Iterator&, const Iterator&) = default;
  };

  /**
   * @brief 1要素を更新するための一時的な Proxy。
   * @details
   * `operator()(index)`
   * から取得し、代入またはモノイド演算による点更新に使用する。
   *
   * コピー・ムーブはできず、一時オブジェクトとしてのみ使用できる。
   *
   * @code
   * seg(i) = value;
   * seg(i).apply(value);
   * seg(i).apply_left(value);
   * @endcode
   */
  class Setter {
    friend class Bplustree_Segtree;

   private:
    Bplustree_Segtree* tree_;
    std::size_t index_;

    Setter(Bplustree_Segtree* tree, std::size_t index)
        : tree_(tree), index_(index) {}

   public:
    Setter(const Setter&) = delete;
    Setter& operator=(const Setter&) = delete;
    Setter(Setter&&) = delete;
    Setter& operator=(Setter&&) = delete;

    /**
     * @brief index 番目の要素を `value` に置き換える。
     * @param value 新しい値。
     *
     * ### 計算量
     * O(NodeCapacity log_{NodeCapacity} N)
     */
    void operator=(const T& value) && {
      tree_->set(index_, value);
    }

    /**
     * @brief index 番目の要素の右側から `value` を作用させる。
     * @param value 作用させる値。
     * @details
     * 現在値を `x` とすると、`x = Monoid::op(x, value)` と更新する。
     *
     * ### 計算量
     * O(NodeCapacity log_{NodeCapacity} N)
     */
    void apply(const T& value) && {
      tree_->apply_right(index_, value);
    }

    /**
     * @brief index 番目の要素の右側から `value` を作用させる。
     * @param value 作用させる値。
     * @details
     * 現在値を `x` とすると、`x = Monoid::op(x, value)` と更新する。
     *
     * ### 計算量
     * O(NodeCapacity log_{NodeCapacity} N)
     */
    void apply_right(const T& value) && {
      tree_->apply_right(index_, value);
    }

    /**
     * @brief index 番目の要素の左側から `value` を作用させる。
     * @param value 作用させる値。
     * @details
     * 現在値を `x` とすると、`x = Monoid::op(value, x)` と更新する。
     *
     * ### 計算量
     * O(NodeCapacity log_{NodeCapacity} N)
     */
    void apply_left(const T& value) && {
      tree_->apply_left(index_, value);
    }
  };

 public:
  // 値の更新にはsetを経由する必要があるため、更新可能なiteratorは提供しない
  using iterator = Iterator<true>;
  using const_iterator = Iterator<true>;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  static_assert(std::random_access_iterator<iterator>);
  static_assert(std::random_access_iterator<const_iterator>);

 private:
  std::size_t size_ = 0;
  std::size_t level_ = 0;
  std::unique_ptr<Node> root_ = nullptr;

  void init() {
    size_ = 0;
    level_ = 0;
    root_ = std::make_unique<Leaf>();
  }

  void cut_invalid_root() {
    if (size_ == 0) return init();

    while (true) {
      if (root_->is_leaf()) return;
      Internal* root = static_cast<Internal*>(root_.get());
      if (root->count > 1) return;
      --level_;
      auto new_root = std::move(root->children[0]);
      new_root->parent = nullptr;
      new_root->position = 0;
      root_ = std::move(new_root);
    }
  }

  void build(std::vector<T>& values) {
    if (values.empty()) {
      init();
      return;
    }

    size_ = values.size();
    level_ = 0;
    constexpr std::size_t kMaxCount = kNodeCapacity - 1;
    const std::size_t leaf_count = (values.size() + kMaxCount - 1) / kMaxCount;
    const std::size_t base_leaf_size = values.size() / leaf_count;
    const std::size_t extra_leaf_count = values.size() % leaf_count;

    std::vector<std::unique_ptr<Node>> level;
    level.reserve(leaf_count);

    std::size_t value_index = 0;
    for (std::size_t i = 0; i < leaf_count; ++i) {
      const std::size_t count = base_leaf_size + (i < extra_leaf_count ? 1 : 0);
      auto leaf = std::make_unique<Leaf>();
      leaf->count = count;
      leaf->size = count;

      for (std::size_t j = 0; j < count; ++j)
        leaf->values[j] = std::move(values[value_index++]);
      leaf->sync();
      level.push_back(std::move(leaf));
    }

    while (level.size() > 1) {
      ++level_;
      const std::size_t parent_count =
          (level.size() + kMaxCount - 1) / kMaxCount;
      const std::size_t base_child_count = level.size() / parent_count;
      const std::size_t extra_parent_count = level.size() % parent_count;

      std::vector<std::unique_ptr<Node>> next_level;
      next_level.reserve(parent_count);
      std::size_t child_index = 0;

      for (std::size_t i = 0; i < parent_count; ++i) {
        const std::size_t count =
            base_child_count + (i < extra_parent_count ? 1 : 0);

        auto parent = std::make_unique<Internal>();
        parent->count = count;
        for (std::size_t j = 0; j < count; ++j)
          parent->children[j] = std::move(level[child_index++]);

        parent->sync();
        next_level.push_back(std::move(parent));
      }

      level = std::move(next_level);
    }

    root_ = std::move(level[0]);
    root_->parent = nullptr;
    root_->position = 0;
  }

  void swap_state(Bplustree_Segtree& other) noexcept {
    using std::swap;
    swap(this->size_, other.size_);
    swap(this->level_, other.level_);
    swap(this->root_, other.root_);
  }

  void internal_insert(Internal* parent, std::unique_ptr<Node> child,
                       std::size_t index) {
    while (parent != nullptr) {
      if (child != nullptr) {
        if (parent->insert(std::move(child), index))
          child = parent->split();
        else
          child = nullptr;
      } else
        parent->sync();

      index = parent->position + 1;
      parent = parent->parent;
    }
    if (child != nullptr) {
      auto new_root = std::make_unique<Internal>();
      new_root->insert(std::move(root_), 0);
      new_root->insert(std::move(child), 1);
      root_ = std::move(new_root);
      ++level_;
    }
  }

  // 子Nodeを一括でinsertしてマージする。
  // この操作でsplitが必要にならないことを条件とする
  // lds と rdsは同じlevelであることに注意
  void merge_node(Node* lds_, Node* rds_, const bool rds_on_right) {
    assert(lds_->kind == rds_->kind);
    assert(lds_->count + rds_->count < kNodeCapacity);
    lds_->push_reverse();
    rds_->push_reverse();

    std::size_t start_from = 0;
    if (rds_->is_leaf()) {
      auto* lds = static_cast<Leaf*>(lds_);
      auto* rds = static_cast<Leaf*>(rds_);
      if (!rds_on_right)
        for (std::size_t i = lds->count; i > 0; --i)
          lds->values[i + rds->count - 1] = std::move(lds->values[i - 1]);
      else
        start_from = lds->count;

      for (std::size_t i = 0; i < rds->count; ++i)
        lds->values[start_from + i] = std::move(rds->values[i]);

      lds->count += rds->count;
      lds->size += rds->size;
      lds->sync();
    } else {
      auto* lds = static_cast<Internal*>(lds_);
      auto* rds = static_cast<Internal*>(rds_);
      if (!rds_on_right)
        for (std::size_t i = lds->count; i > 0; --i)
          lds->children[i + rds->count - 1] = std::move(lds->children[i - 1]);
      else
        start_from = lds->count;

      for (std::size_t i = 0; i < rds->count; ++i)
        lds->children[start_from + i] = std::move(rds->children[i]);

      lds->count += rds->count;
      lds->sync();
    }
    lds_ = lds_->parent;
    while (lds_ != nullptr) {
      static_cast<Internal*>(lds_)->sync();
      lds_ = lds_->parent;
    }
  }

  void sync_to_root(Leaf* leaf) {
    leaf->sync();

    Internal* node = leaf->parent;
    while (node != nullptr) {
      node->sync();
      node = node->parent;
    }
  }

 public:
  /**
   * @brief 空の B+ tree を構築する。
   * @details
   *
   * ### 計算量
   * O(1)
   */
  Bplustree_Segtree() {
    init();
  }

  /**
   * @brief イテレータ範囲 `[first, last)` の要素からVectorを構築する。
   * @tparam InputIt 入力イテレータ型。
   * @param first 範囲の先頭。
   * @param last 範囲の終端。
   * @details
   * B+ tree を直接一括構築する。
   *
   * ### 計算量
   * O(N)。
   */
  template <std::input_iterator InputIt>
  Bplustree_Segtree(InputIt first, InputIt last) {
    assign(first, last);
  }

  /**
   * @brief `count` 個の値初期化された要素を持つ B+ tree を構築する。
   * @param count 要素数。
   * @details
   * 各要素は `e()` で初期化される。
   *
   * ### 計算量
   * O(count)
   */
  explicit Bplustree_Segtree(size_type count) {
    assign(count);
  };

  /**
   * @brief `value` を `count` 個持つ B+ tree を構築する。
   * @param count 要素数。
   * @param value 各要素の初期値。
   *
   * ### 計算量
   * O(count)
   */
  Bplustree_Segtree(size_type count, const T& value) {
    assign(count, value);
  };

  /**
   * @brief 初期化子リストの要素から B+ tree を構築する。
   * @param init 初期要素。
   *
   * ### 計算量
   * O(init.size())
   */
  Bplustree_Segtree(std::initializer_list<T> init) {
    assign(init.begin(), init.end());
  };

  /**
   * @brief 別の B+ tree をコピーして構築する。
   * @param other コピー元。
   * @details
   * 全要素をコピーし、木構造を一括構築し直す。
   * `other` を指す Iterator は無効化されない。
   *
   * ### 計算量
   * O(other.size_)
   */
  Bplustree_Segtree(const Bplustree_Segtree& other)
      : Bplustree_Segtree(other.begin(), other.end()) {}

  /**
   * @brief 別の B+ tree の内容をコピー代入する。
   * @param other コピー元の B+ tree。
   * @return `*this`。
   * @details
   * `other` の全要素をコピーして新しい木を構築し、
   * 現在の内容と置き換える。
   * 例外安全性は特に保証しない。
   * 通常の競プロ用途では例外を投げる T を想定しない。
   *
   * `*this` を指していた既存の Iterator はすべて無効化される。
   * `other` を指す Iterator は無効化されない。
   *
   * ### 計算量
   * 代入前の要素数を N、`other` の要素数を M として O(N + M)。
   */
  Bplustree_Segtree& operator=(const Bplustree_Segtree& other) {
    if (this == &other) return *this;
    assign(other.begin(), other.end());
    return *this;
  }

  /**
   * @brief 別の B+ tree の内容をムーブして構築する。
   * @param other 移動元。
   * @details
   * 木構造の所有権を引き継ぐ。
   * 移動後の `other`
   * に対して保証する操作は破棄と再代入のみ。 `other` を指していた Iterator
   * はすべて無効化される。
   *
   * ### 計算量
   * O(1)
   */
  Bplustree_Segtree(Bplustree_Segtree&& other)
      : size_(other.size_),
        level_(other.level_),
        root_(std::move(other.root_)) {}

  /**
   * @brief 別の B+ tree の内容をムーブ代入する。
   * @param other 移動元の B+ tree。
   * @return `*this`。
   * @details
   * `other` の木構造の所有権を引き継ぐ。
   * 移動後の `other` に対して保証する操作は
   * 破棄と再代入のみ。
   *
   * 代入前に `*this` が所有していた木構造は `other` に移される。
   * `*this` および `other` を指していた既存の Iterator はすべて無効化される。
   *
   * ### 計算量
   * O(1)。
   * ただし、代入前の `*this` の要素数を N とすると、
   * `other` の破棄または再代入時にその木の破棄に O(N) を要する。
   */
  Bplustree_Segtree& operator=(Bplustree_Segtree&& other) {
    if (this == &other) return *this;

    swap_state(other);
    return *this;
  }

  /**
   * @brief 2つの B+ tree の内容を交換する。
   * @details
   * 全状態を交換する。
   * 両コンテナの既存 Iterator はすべて無効化される。
   *
   * ### 計算量
   * O(1)
   */
  friend void swap(Bplustree_Segtree& lhs, Bplustree_Segtree& rhs) noexcept {
    lhs.swap_state(rhs);
  }

  /**
   * @brief イテレータ範囲 `[first, last)` の要素で内容を置き換える。
   * @tparam InputIt 入力イテレータ型。
   * @param first 範囲の先頭。
   * @param last 範囲の終端。
   * @details
   * 現在格納されている要素をすべて破棄し、指定された範囲から
   * B+ tree を一括構築する。
   *
   * この操作によって既存の Iterator はすべて無効化される。
   *
   * ### 計算量
   * 代入前の要素数を N、入力範囲の要素数を M として、
   * O(N + M)。
   */
  template <std::input_iterator InputIt>
  void assign(InputIt first, InputIt last) {
    std::vector<T> values;
    for (; first != last; ++first) values.push_back(*first);
    build(values);
  }

  /**
   * @brief `value` を `count` 個持つ列で内容を置き換える。
   * @param count 新しい要素数。
   * @param value 各要素の値。
   * @details
   * この操作によって既存の Iterator はすべて無効化される。
   *
   * ### 計算量
   * 変更前の要素数を N として O(N + count)。
   */
  void assign(std::size_t count, const T& value) {
    std::vector<T> values(count, value);
    build(values);
  }

  /**
   * @brief `count` 個の値初期化された要素を持つ列で内容を置き換える。
   * @param count 新しい要素数。
   * @details
   * 各要素は `e()` で初期化される。
   * この操作によって既存の Iterator はすべて無効化される。
   *
   * ### 計算量
   * 変更前の要素数を N として O(N + count)。
   */
  void assign(std::size_t count) {
    std::vector<T> values(count, Monoid::e());
    build(values);
  }

  /**
   * @brief 要素数を `count` に変更する。
   * @param count 変更後の要素数。
   * @param value 拡張時に追加される要素の値。
   * @details
   * `count < size()` の場合は末尾の要素を削除する。
   * `count > size()` の場合は末尾に `value` を追加する。
   *
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * 追加または削除する要素数を K として、
   * O(K + NodeCapacity log_{NodeCapacity} N)。
   */
  void resize(std::size_t size, const T& value) {
    if (size < size_) {
      split(size);
      return;
    }

    Bplustree_Segtree right(size - size_, value);
    merge(right);
  }

  /**
   * @brief 要素数を `count` に変更する。
   * @param count 変更後の要素数。
   * @details
   * 拡張する場合、新しい要素は `e()` で初期化される。
   *
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * 追加または削除する要素数を K として、
   * O(K + NodeCapacity log_{NodeCapacity} N)。
   */
  void resize(size_type size) {
    resize(size, Monoid::e());
  }

  /**
   * @brief 先頭要素を指すイテレータを返す。
   * @return 空なら `end()`、そうでなければ先頭要素を指すイテレータ。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  iterator begin() const {
    return nth(0);
  }

  /**
   * @brief 末尾の次を表すイテレータを返す。
   * @return 末尾イテレータ。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  iterator end() const {
    return nth(size_);
  }

  /**
   * @brief `begin()` と同じイテレータを返す。
   * @return 先頭イテレータ。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  iterator cbegin() const {
    return begin();
  }

  /**
   * @brief `end()` と同じイテレータを返す。
   * @return 末尾イテレータ。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  iterator cend() const {
    return end();
  }

  /**
   * @brief 末尾要素から逆順に走査するための先頭イテレータを返す。
   * @return 空なら `rend()`、そうでなければ末尾要素を指す逆イテレータ。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  /**
   * @brief 逆順走査の末尾イテレータを返す。
   * @return `begin()` の直前を表す逆イテレータ。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  /**
   * @brief `rbegin()` と同じ逆イテレータを返す。
   * @return 逆順走査の先頭イテレータ。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }

  /**
   * @brief `rend()` と同じ逆イテレータを返す。
   * @return 逆順走査の末尾イテレータ。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  /**
   * @brief 要素数を返す。
   * @return 格納されている要素数。
   * ### 計算量
   * O(1)
   */
  size_type size() const noexcept {
    return size_;
  }

  /**
   * @brief Vectorが空か判定する。
   * @return 空なら `true`、そうでなければ `false`。
   * ### 計算量
   * O(1)
   */
  bool empty() const noexcept {
    return size_ == 0;
  }

  /**
   * @brief `value` と等価な要素を検索する。
   * @param value 検索するキー。
   * @return
   * 見つかればその要素を指す先頭に最も近いイテレータ、見つからなければ
   * `end()`。
   * ### 計算量
   * O(N)
   */
  iterator find(const T& value) const {
    auto last = end();
    for (auto it = begin(); it != last; ++it) {
      if (*it == value) return it;
    }
    return last;
  }

  /**
   * @brief `value` と等価な要素数を返す。
   * @param value 数えるキー。
   * @return `value` と等価な要素数。
   * ### 計算量
   * O(N)
   */
  size_type count(const T& value) const {
    std::size_t cnt = 0;
    auto last = end();
    for (auto it = begin(); it != last; ++it) {
      cnt += (*it == value);
    }
    return cnt;
  }

  /**
   * @brief `value` と等価な要素が存在するか判定する。
   * @param value 検索するキー。
   * @return 存在すれば `true`、そうでなければ `false`。
   * ### 計算量
   * O(N)
   */
  bool contains(const T& value) const {
    return find(value) != end();
  }

  /**
   * @brief `index` 番目の要素を `value` に置き換える。
   * @param index 更新する要素の位置。
   * @param value 新しい値。
   * @details `index < size()` を満たす必要がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void set(std::size_t index, const T& value) {
    auto [leaf, index_] = root_->nth(index);
    leaf->values[index_] = value;
    sync_to_root(leaf);
  }

  /**
   * @brief `index` 番目の要素の右側から `value` を作用させる。
   * @param index 更新する要素の位置。
   * @param value 作用させる値。
   * @details
   * 現在値を `x` とすると、`x = Monoid::op(x, value)` と更新する。
   * `index < size()` を満たす必要がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void apply_right(std::size_t index, const T& value) {
    auto [leaf, index_] = root_->nth(index);
    leaf->values[index_] = Monoid::op(leaf->values[index_], value);
    sync_to_root(leaf);
  }

  /**
   * @brief `index` 番目の要素の左側から `value` を作用させる。
   * @param index 更新する要素の位置。
   * @param value 作用させる値。
   * @details
   * 現在値を `x` とすると、`x = Monoid::op(value, x)` と更新する。
   * `index < size()` を満たす必要がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void apply_left(std::size_t index, const T& value) {
    auto [leaf, index_] = root_->nth(index);
    leaf->values[index_] = Monoid::op(value, leaf->values[index_]);
    sync_to_root(leaf);
  }

  /**
   * @brief 区間 `[first, last)` のモノイド積を返す。
   * @param first 区間の先頭。
   * @param last 区間の終端。
   * @return
   * `Monoid::op(values[first], ..., values[last - 1])` を添字順に計算した値。
   * 空区間の場合は `Monoid::e()`。
   * @details `first <= last <= size()` を満たす必要がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  T prod(std::size_t first, std::size_t last) {
    return root_->node_prod(first, last);
  }

  /**
   * @brief 区間 `[first, last)` を逆順に計算したモノイド積を返す。
   * @param first 区間の先頭。
   * @param last 区間の終端。
   * @return
   * `Monoid::op(values[last - 1], ..., values[first])`
   * を添字の逆順に計算した値。 空区間の場合は `Monoid::e()`。
   * @details `first <= last <= size()` を満たす必要がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  T reverse_prod(std::size_t first, std::size_t last) {
    return root_->node_reverse_prod(first, last);
  }

  /**
   * @brief 全要素のモノイド積を返す。
   * @return `prod(0, size())` と等価な値。
   *
   * ### 計算量
   * O(1)
   */
  T all_prod() {
    return root_->evaluated_monoid;
  }

  /**
   * @brief 0-indexed で index 番目の要素を指す Iterator を返す。
   * @param index 要素の順位。
   * @return index 番目の要素を指す Iterator。
   *
   * ### 計算量
   * O(kNodeCapacity log N)
   */
  iterator nth(std::size_t index) const {
    auto [leaf, idx] = root_->nth(index);
    return {leaf, idx};
  }

  /**
   * @brief 0-indexed で `index` 番目の要素を読み取る。
   * @param index 要素の位置。
   * @return `index` 番目の要素への const 参照。
   * @details
   * `index < size()` を満たす必要がある。
   * 要素の更新には `operator()` または `set()` を使用する。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  const T& operator[](std::size_t index) const {
    return *nth(index);
  }

  /**
   * @brief 0-indexed で `index` 番目の要素を更新するための Setter を返す。
   * @param index 更新する要素の位置。
   * @return `index` 番目の要素を更新するための Setter。
   * @details
   * `index < size()` を満たす必要がある。
   *
   * @code
   * seg(i) = value;
   * seg(i).apply(value);
   * seg(i).apply_left(value);
   * @endcode
   *
   * ### 計算量
   * O(1)
   *
   * Setter を用いた実際の更新は
   * O(NodeCapacity log_{NodeCapacity} N)。
   */
  [[nodiscard]] Setter operator()(std::size_t index) {
    assert(index < size_);
    return Setter(this, index);
  }

  /**
   * @brief 先頭要素への const 参照を返す。
   * @return 先頭要素への const 参照。
   * @details `empty() == false` を満たす必要がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  const T& front() const {
    return *nth(0);
  }

  /**
   * @brief 末尾要素への const 参照を返す。
   * @return 末尾要素への const 参照。
   * @details `empty() == false` を満たす必要がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  const T& back() const {
    return *nth(size_ - 1);
  }

  /**
   * @brief `left` から右方向へモノイド積を取り、条件を満たす最大の右端を返す。
   * @tparam F モノイド積に対する述語の型。
   * @param left 探索を開始する左端。
   * @param f モノイド積を受け取り `bool` を返す述語。
   * @return
   * `f(prod(left, right))` が成り立つ最大の `right`。
   * `left <= right <= size()`。
   *
   * @details
   * `left <= size()` を満たす必要がある。
   *
   * `[left, r)` を右へ拡張したとき、`f(prod(left, r))` は
   * `true` から `false` へ高々1回だけ変化する必要がある。
   *
   * `left == size()` の場合は `size()` を返す。
   * `f(Monoid::e())` は呼び出さない。
   *
   * ### 計算量
   * `Monoid::op()` と `f()` を O(1) として
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  template <class F>
  std::size_t max_right(std::size_t left, F&& f) {
    assert(left <= size_);
    if (left == size_) return size_;

    auto [leaf, index] = nth(left);

    T current_value = Monoid::e();
    Node* current = leaf;
    std::size_t result = left;
    while (true) {
      current->push_reverse();
      if (current->is_leaf()) {
        auto* leaf = static_cast<Leaf*>(current);
        for (std::size_t i = index; i < leaf->count; ++i) {
          T next_value = Monoid::op(current_value, leaf->values[i]);
          if (!f(next_value)) return result;

          current_value = std::move(next_value);
          ++result;
        }
      } else {
        auto* internal = static_cast<Internal*>(current);
        bool descend = false;

        for (std::size_t i = index; i < internal->count; ++i) {
          Node* child = internal->children[i].get();
          T next_value = Monoid::op(current_value, child->evaluated_monoid);

          if (!f(next_value)) {
            current = child;
            index = 0;
            descend = true;
            break;
          }

          current_value = std::move(next_value);
          result += child->size;
        }

        if (descend) continue;
      }
      if (result == size_) return size_;

      index = current->position + 1;
      current = current->parent;
    }
    assert(false);
    return 0;
  }

  // max_rightと同じ。fがtrueを返す範囲の右側の境界を返す。
  template <class F>
  std::size_t right_bound(std::size_t left, F&& f) {
    return max_right(left, f);
  }

  /**
   * @brief `right` から左方向へモノイド積を取り、条件を満たす最小の左端を返す。
   * @tparam F モノイド積に対する述語の型。
   * @param right 探索を開始する右端。
   * @param f モノイド積を受け取り `bool` を返す述語。
   * @return
   * `f(prod(left, right))` が成り立つ最小の `left`。
   * `0 <= left <= right`。
   *
   * @details
   * `right <= size()` を満たす必要がある。
   *
   * `[l, right)` を左へ拡張したとき、`f(prod(l, right))` は
   * `true` から `false` へ高々1回だけ変化する必要がある。
   *
   * `right == 0` の場合は `0` を返す。
   * `f(Monoid::e())` は呼び出さない。
   *
   * 非可換モノイドでも要素順を保って積を計算する。
   *
   * ### 計算量
   * `Monoid::op()` と `f()` を O(1) として
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  template <class F>
  std::size_t min_left(std::size_t right, F&& f) {
    assert(right <= size_);
    if (right == 0) return 0;

    auto [leaf, index] = nth(right);

    T current_value = Monoid::e();
    Node* current = leaf;
    std::size_t result = right;
    while (true) {
      current->push_reverse();
      if (current->is_leaf()) {
        auto* leaf = static_cast<Leaf*>(current);
        for (std::size_t i = index; i > 0; --i) {
          T next_value = Monoid::op(leaf->values[i - 1], current_value);
          if (!f(next_value)) return result;

          current_value = std::move(next_value);
          --result;
        }
      } else {
        auto* internal = static_cast<Internal*>(current);
        bool descend = false;

        for (std::size_t i = index; i > 0; --i) {
          Node* child = internal->children[i - 1].get();
          T next_value = Monoid::op(child->evaluated_monoid, current_value);

          if (!f(next_value)) {
            current = child;
            index = child->count;
            descend = true;
            break;
          }

          current_value = std::move(next_value);
          result -= child->size;
        }

        if (descend) continue;
      }
      if (result == 0) return 0;

      index = current->position;
      current = current->parent;
    }
    assert(false);
    return 0;
  }

  // min_leftと同じ。fがtrueを返す範囲の左側の境界を返す。
  template <class F>
  std::size_t left_bound(std::size_t right, F&& f) {
    return min_left(right, f);
  }

  /**
   * @brief 要素をi番目に挿入する。挿入後に `value` は `index` の位置になる。
   * @details 更新により既存イテレータは
   * 無効化される場合がある。
   * @param index 挿入するindexを表す整数。(0-indexed)
   * @param value 挿入するキー。
   * @return 挿入位置のイテレータ。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  iterator insert(const size_type index, const T& value) {
    assert(index <= size_);
    auto it = index == size_ ? end() : nth(index);
    Leaf* leaf = it.leaf_;
    std::size_t idx = it.index_;

    ++size_;

    Leaf* inserted_leaf = leaf;
    std::size_t inserted_index = idx;

    std::unique_ptr<Node> right = nullptr;

    if (leaf->insert(value, idx)) {
      auto right_leaf = leaf->split();
      Leaf* right_leaf_ptr = right_leaf.get();

      if (idx >= kNodeHalf) {
        inserted_leaf = right_leaf_ptr;
        inserted_index = idx - kNodeHalf;
      }
      right = std::move(right_leaf);
    }
    internal_insert(leaf->parent, std::move(right), leaf->position + 1);
    return {inserted_leaf, inserted_index};
  }

  /**
   * @brief `pos` の直前に `value` を挿入する。
   * @param pos 挿入位置を表す Iterator。
   * @param value 挿入する値。
   * @return 挿入された要素を指す Iterator。
   * @details
   * `pos` は `*this` に属する Iterator である必要がある。
   * `end()` も指定できる。
   *
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  iterator insert(iterator pos, const T& value) {
    return insert(static_cast<std::size_t>(pos - begin()), value);
  }

  /**
   * @brief 先頭に要素を追加する。
   * @param value 追加する値。
   * @details 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void push_front(const T& value) {
    insert(0, value);
  }

  /**
   * @brief 末尾に要素を追加する。
   * @param value 追加する値。
   * @details
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void push_back(const T& value) {
    insert(size_, value);
  }

  /**
   * @brief 指定した位置の要素を1つ削除する。
   * @param it 削除する要素を指すイテレータ。`end()` は指定できない。
   * @return 削除した要素の次を指すイテレータ。
   * @details 更新により `it` を含む既存イテレータは無効化される場合がある。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  iterator erase(const iterator pos) {
    --size_;
    if (size_ == 0) {
      init();
      return begin();
    }

    Leaf* leaf = pos.leaf_;
    std::size_t index = pos.index_;

    iterator result = pos;
    if (index == leaf->count - 1) ++result;

    const bool result_is_end = result.index_ == result.leaf_->count;
    bool need_erase = leaf->erase(index);

    Internal* parent = leaf->parent;
    Node* child = leaf;
    while (parent != nullptr) {
      if (need_erase)
        need_erase = parent->erase(child->position);
      else
        parent->sync();

      child = parent;
      parent = child->parent;
    }
    cut_invalid_root();

    if (result_is_end) return end();
    return result;
  }

  /**
   * @brief 0-indexed で `index` 番目の要素を削除する。
   * @param index 削除する要素の位置。`index < size()` を満たす必要がある。
   * @return 削除した要素の次を指す Iterator。
   * @details
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  iterator erase(size_type index) {
    return erase(nth(index));
  }

  /**
   * @brief 添字範囲 `[first, last)` の要素を削除する。
   * @param first 削除範囲の先頭。
   * @param last 削除範囲の終端。
   * @return 削除後に `first` の位置となる要素を指す Iterator。
   * 末尾まで削除した場合は `end()`。
   * @details
   * `first <= last <= size()` を満たす必要がある。
   *
   * 内部では範囲の前後で木を分割し、削除範囲の木を破棄して再結合する。
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * 削除される要素数を K として、
   * O(K + NodeCapacity log_{NodeCapacity} N)。
   */
  iterator erase(size_type first, size_type last) {
    assert(first <= last);
    assert(last <= size_);
    auto right = split(last);
    split(first);
    merge(right);
    return nth(first);
  };

  /**
   * @brief Iterator 範囲 `[first, last)` の要素を削除する。
   * @param first 削除範囲の先頭。
   * @param last 削除範囲の終端。
   * @return 削除後に `first` の位置となる要素を指す Iterator。
   * 末尾まで削除した場合は `end()`。
   * @details
   * `first` と `last` は `*this` に属し、`first <= last` を満たす必要がある。
   *
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * 削除される要素数を K として、
   * O(K + NodeCapacity log_{NodeCapacity} N)。
   */
  iterator erase(iterator first, iterator last) {
    std::size_t lidx = first - begin();
    std::size_t ridx = last - begin();

    return erase(lidx, ridx);
  };

  /**
   * @brief 先頭の要素を削除する。
   * @details
   * `empty() == false` を満たす必要がある。
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void pop_front() {
    erase(begin());
  }

  /**
   * @brief 末尾の要素を削除する。
   * @details
   * `empty() == false` を満たす必要がある。
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void pop_back() {
    erase(--end());
  }

  /**
   * @brief 指定した位置で B+ tree を2つに分割する。
   * @param index 分割位置。`index <= size()` を満たす必要がある。
   * @return 分割前の `[index, size())` を保持する B+ tree。
   * @details
   * 操作後、`*this` には `[0, index)` の要素が残る。
   *
   * `index == 0` の場合は全要素を戻り値へ移し、
   * `index == size()` の場合は空の B+ tree を返す。
   *
   * 分割境界付近では、疎な Node を可能な範囲で統合する。
   * この操作によって既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  Bplustree_Segtree split(std::size_t index) {
    assert(index <= size_);

    if (index == size_) return {};
    if (index == 0) {
      Bplustree_Segtree right;
      swap_state(right);
      return right;
    }

    return split(nth(index));
  };

  /**
   * @brief 指定した Iterator の位置で B+ tree を2つに分割する。
   * @param it 戻り値側の先頭を表す Iterator。
   * @return 分割前の `[it, end())` を保持する B+ tree。
   * @details
   * `it` は `*this` に属する Iterator である必要がある。
   *
   * 操作後、`*this` には `[begin(), it)` に存在した要素が残る。
   * `it == begin()` および `it == end()` も指定できる。
   *
   * 分割境界付近では、疎な Node を可能な範囲で統合する。
   * この操作によって既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  Bplustree_Segtree split(iterator pos) {
    Bplustree_Segtree right;
    auto right_leaf = std::make_unique<Leaf>();
    if (pos == begin()) {
      swap_state(right);
      return right;
    }
    if (pos == end()) return right;

    Leaf* leaf = pos.leaf_;
    std::size_t index = pos.index_;
    if (index == 0) {
      leaf = (--pos).leaf_;
      index = leaf->count;
    } else {
      for (std::size_t i = index; i < leaf->count; ++i) {
        right_leaf->values[i - index] = std::move(leaf->values[i]);
      }
      right_leaf->count = leaf->count - index;
      right_leaf->sync();
      leaf->count = index;
      leaf->sync();
    }

    std::unique_ptr<Node> right_child = std::move(right_leaf);
    std::unique_ptr<Internal> parent = nullptr;
    Internal* node = leaf->parent;
    index = leaf->position + 1;
    while (node != nullptr) {
      parent = std::make_unique<Internal>();

      if (right_child->count) {
        parent->children[0] = std::move(right_child);
        ++parent->count;
      }
      for (std::size_t i = index; i < node->count; ++i) {
        parent->children[parent->count] = std::move(node->children[i]);
        ++parent->count;
      }
      node->count = index;
      node->sync();
      parent->sync();

      right_child = std::move(parent);
      index = node->position + 1;
      node = node->parent;
    }
    right.size_ = right_child->size;
    right.root_ = std::move(right_child);

    size_ = root_->size;

    right.level_ = level_;
    if (!root_->is_leaf()) static_cast<Internal*>(root_.get())->shrink(true);
    cut_invalid_root();

    if (!right.root_->is_leaf())
      static_cast<Internal*>(right.root_.get())->shrink(false);
    right.cut_invalid_root();

    return right;
  };

  /**
   * @brief `right` の全要素を末尾に連結する。
   * @param right 右側に連結する B+ tree。
   * @details
   * 操作前の列をそれぞれ `L`, `R` とすると、
   * 操作後の `*this` は `L + R` となり、`right` は空になる。
   *
   * Node は可能な場合に境界で統合される。
   * `this != &right` を満たす必要がある。
   *
   * この操作によって `*this` および `right` の既存の Iterator は
   * 無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void merge(Bplustree_Segtree& right) {
    assert(this != &right);
    if (!root_->is_leaf()) static_cast<Internal*>(root_.get())->shrink(true);
    if (!right.root_->is_leaf())
      static_cast<Internal*>(right.root_.get())->shrink(false);

    if (size_ == 0) {
      swap_state(right);
      return;
    }
    if (right.size_ == 0) return;

    const std::size_t merged_size = size_ + right.size_;

    std::size_t left_level = level_;
    std::size_t right_level = right.level_;

    if (left_level > right_level) {
      Internal* target = static_cast<Internal*>(root_.get());
      while (left_level > right_level + 1) {
        target->push_reverse();
        target =
            static_cast<Internal*>(target->children[target->count - 1].get());
        --left_level;
      }
      target->push_reverse();

      if (target->children[target->count - 1]->count + right.root_->count <
          kNodeCapacity)
        merge_node(target->children[target->count - 1].get(), right.root_.get(),
                   true);
      else
        internal_insert(target, std::move(right.root_), target->count);
    } else if (left_level < right_level) {
      Internal* target = static_cast<Internal*>(right.root_.get());
      while (left_level + 1 < right_level) {
        target->push_reverse();
        target = static_cast<Internal*>(target->children[0].get());
        --right_level;
      }
      target->push_reverse();

      if (target->children[0]->count + root_->count < kNodeCapacity)
        merge_node(target->children[0].get(), root_.get(), false);
      else
        right.internal_insert(target, std::move(root_), 0);
      root_ = std::move(right.root_);
      level_ = right.level_;
    } else {
      if (right.root_->count + root_->count < kNodeCapacity)
        merge_node(root_.get(), right.root_.get(), true);
      else {
        auto new_root = std::make_unique<Internal>();
        new_root->insert(std::move(root_), 0);
        new_root->insert(std::move(right.root_), 1);

        root_ = std::move(new_root);
        ++level_;
      }
    }
    right.init();
    size_ = merged_size;
  }

  /**
   * @brief Iterator 範囲 `[first, last)` の要素順を反転する。
   * @param first 反転範囲の先頭。
   * @param last 反転範囲の終端。
   * @details
   * `first` と `last` は `*this` に属し、`first <= last` を満たす必要がある。
   *
   * 内部で Iterator を添字へ変換した後、添字版 `reverse()` を使用する。
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  void reverse(iterator first, iterator last) {
    auto first_ = begin();

    const std::size_t lidx = static_cast<std::size_t>(first - first_);
    const std::size_t ridx = static_cast<std::size_t>(last - first_);

    reverse(lidx, ridx);
  }

  /**
   * @brief 全要素の順序を反転する。
   * @details
   * root の遅延反転フラグのみを更新する。
   * この操作によって既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * O(1)
   */
  void reverse() {
    root_->apply_reverse();
  }

  /**
   * @brief 添字範囲 `[first, last)` の要素順を反転する。
   * @param first 反転する範囲の先頭。
   * @param last 反転する範囲の終端。
   * @details
   * `0 <= first <= last <= size()` を満たす必要がある。
   *
   * 全要素を反転する場合は lazy flag の反転のみで処理する。
   * 更新により既存の Iterator は無効化される場合がある。
   *
   * ### 計算量
   * 通常は O(NodeCapacity log_{NodeCapacity} N)。
   * `[0, size())` 全体の反転は O(1)。
   */
  void reverse(std::size_t first, std::size_t last) {
    assert(first <= last);
    assert(last <= size_);
    if (last - first < 2) return;
    if (first == 0 && last == size_) {
      root_->apply_reverse();
      return;
    }
    auto right = split(last);
    auto mid = split(first);

    mid.root_->apply_reverse();

    merge(mid);
    merge(right);
  }

  /**
   * @brief 現在の要素から B+ tree 全体を再構築する。
   * @details
   * lazy erase などによって要素数の少なくなった Leaf / Internal を破棄し、
   * Node が可能な限り均等に詰まるよう木全体を再構築する。
   *
   * 要素の値と順序は変化しない。
   *
   * この操作によって既存の Iterator はすべて無効化される。
   *
   * ### 計算量
   * O(N)
   */
  void rebuild() {
    std::vector<T> values(begin(), end());
    build(values);
  }

  /**
   * @brief 全要素を削除して空にする。
   * ### 計算量
   * O(N)
   */
  void clear() {
    init();
  }

  /**
   * @brief Node の使用状況を標準エラー出力へ出力する。
   * @details
   * 現在および最大同時生存 Node 数、Leaf / Internal の Node 数、
   * この木から到達可能な Node 数、`count == 1` の Node 数を出力する。
   *
   * `split()` や lazy erase によって木が過度に疎になっていないかを
   * デバッグするために使用する。
   *
   * `live_nodes` などの static counter は、同じテンプレート特殊化を持つ
   * 全インスタンスの合計値であることに注意する。
   *
   * ### 計算量
   * O(Node 数)
   */
  void debug_node_stats() {
    std::cerr << "node: " << Node::live_nodes << " / " << Node::peak_nodes
              << '\n';

    std::cerr << "leaf: " << Leaf::leaf_count << " / " << Leaf::peak_leaf_count
              << '\n';

    std::cerr << "internal: " << Internal::internal_count << " / "
              << Internal::peak_internal_count << '\n';

    std::size_t cnt = 0;
    std::size_t cnt_one = 0;
    auto f = [&](auto f, Node* now) -> void {
      if (now->count == 1) cnt_one++;
      cnt++;

      if (!now->is_leaf())
        for (int i = 0; i < now->count; ++i) {
          f(f, (static_cast<Internal*>(now)->children[i]).get());
        }
    };
    f(f, root_.get());
    std::cerr << "recursive node count: " << cnt << std::endl;
    std::cerr << "recursive node count count is one: " << cnt_one << std::endl;
  }
};

using SumSegtree = Bplustree_Segtree<SumMonoid>;
using MinSegtree = Bplustree_Segtree<MinMonoid>;
using MaxSegtree = Bplustree_Segtree<MaxMonoid>;
using GcdSegtree = Bplustree_Segtree<GcdMonoid>;
template <class mint>
using AffineSegtree = Bplustree_Segtree<AffineMonoid<mint>>;

/**
 * @brief `Op` と `E` を直接指定して構築する B+ tree セグメント木。
 */
template <class T, auto Op, auto E, std::size_t NodeCapacity = 64>
using BSegtree = Bplustree_Segtree<Monoid<T, Op, E>, NodeCapacity>;

/**
 * @brief 単位元を指定せずに構築する B+ tree セグメント木。
 * @details
 * 各要素を通常の `T` として扱いつつ、内部では人工的な単位元を使用する。
 */
template <class T, auto Op, std::size_t NodeCapacity = 64>
using BSegtreeWithoutE = Bplustree_Segtree<MonoidWithoutE<T, Op>, NodeCapacity>;
