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

enum class SetType { Set, MultiSet };

template <class Key = long long, SetType Type = SetType::Set,
          class Compare = std::less<Key>, std::size_t NodeCapacity = 64>
class Bplustree_Set {
  static_assert(NodeCapacity >= 3);
  static_assert(std::default_initializable<Key>);

  static constexpr std::size_t kNodeCapacity = NodeCapacity;
  static constexpr std::size_t kNodeHalf = kNodeCapacity / 2;

  static constexpr bool kIsMultiSet = Type == SetType::MultiSet;

  [[no_unique_address]] Compare compare_;

  static_assert(std::strict_weak_order<const Compare&, const Key&, const Key&>);

  enum class NodeKind { kLeaf, kInternal };

  struct Internal;

  struct Node {
    Internal* parent = nullptr;
    std::size_t position = 0;
    std::size_t size = 0;
    std::size_t count = 0;
    const NodeKind kind;
    const Key* min_key = nullptr;

    explicit Node(NodeKind kind) : kind(kind) {}
    virtual ~Node() = default;

    bool is_leaf() const {
      return kind == NodeKind::kLeaf;
    }
  };

  struct Leaf final : Node {
    std::array<Key, kNodeCapacity> keys{};
    Leaf* prev = nullptr;
    Leaf* next = nullptr;

    Leaf() : Node(NodeKind::kLeaf) {}

    void set_min_key() {
      this->min_key = this->count ? &keys[0] : nullptr;
    }

    bool insert(Key key, std::size_t key_index) {
      assert(key_index <= this->count);
      assert(this->count < kNodeCapacity);
      for (std::size_t i = this->count; i > key_index; --i) {
        keys[i] = std::move(keys[i - 1]);
      }
      keys[key_index] = std::move(key);

      ++this->count;
      ++this->size;
      set_min_key();

      return this->count == kNodeCapacity;
    }

    std::unique_ptr<Leaf> split() {
      assert(this->count == kNodeCapacity);

      const std::size_t left_count = kNodeHalf;
      const std::size_t right_count = kNodeCapacity - left_count;
      auto right = std::make_unique<Leaf>();

      right->count = right_count;
      right->size = right_count;

      for (std::size_t i = 0; i < right_count; ++i) {
        right->keys[i] = std::move(keys[left_count + i]);
      }
      this->count = left_count;
      this->size = left_count;

      set_min_key();
      right->set_min_key();

      right->prev = this;
      right->next = this->next;
      if (this->next != nullptr) {
        this->next->prev = right.get();
      }
      this->next = right.get();

      return right;
    }

    bool erase(std::size_t key_index) {
      assert(this->count < kNodeCapacity);
      assert(key_index < this->count);
      for (std::size_t i = key_index + 1; i <= this->count; ++i) {
        keys[i - 1] = std::move(keys[i]);
      }
      --this->count;
      --this->size;

      set_min_key();
      if (this->count == 0) {
        auto prev = this->prev;
        auto next = this->next;
        if (prev != nullptr) prev->next = next;
        if (next != nullptr) next->prev = prev;
        return true;
      }
      return false;
    }

    std::ptrdiff_t max_lt(const Key& key, const Compare& compare_) const {
      std::ptrdiff_t ok = -1;
      std::ptrdiff_t ng = this->count;

      while (ng - ok > 1) {
        std::ptrdiff_t mid = (ok + ng) / 2;
        if (compare_(keys[mid], key))
          ok = mid;
        else
          ng = mid;
      }
      return ok;
    }

    std::ptrdiff_t max_le(const Key& key, const Compare& compare_) const {
      std::ptrdiff_t ok = -1;
      std::ptrdiff_t ng = this->count;

      while (ng - ok > 1) {
        std::ptrdiff_t mid = (ok + ng) / 2;
        if (compare_(key, keys[mid]))
          ng = mid;
        else
          ok = mid;
      }
      return ok;
    }
  };

  struct Internal final : Node {
    std::array<std::unique_ptr<Node>, kNodeCapacity> children{};

    Internal() : Node(NodeKind::kInternal) {}

    void set_min_key() {
      this->min_key = this->count ? children[0]->min_key : nullptr;
    }

    void sync() {
      this->size = 0;
      for (std::size_t i = 0; i < this->count; ++i) {
        assert(children[i] != nullptr);
        children[i]->parent = this;
        children[i]->position = i;
        this->size += children[i]->size;
      }
      set_min_key();
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

    std::ptrdiff_t max_lt(const Key& key, const Compare& compare_) const {
      std::ptrdiff_t ok = -1;
      std::ptrdiff_t ng = this->count;

      while (ng - ok > 1) {
        std::ptrdiff_t mid = (ok + ng) / 2;
        if (compare_(*children[mid]->min_key, key))
          ok = mid;
        else
          ng = mid;
      }
      return ok;
    }

    std::ptrdiff_t max_le(const Key& key, const Compare& compare_) const {
      std::ptrdiff_t ok = -1;
      std::ptrdiff_t ng = this->count;

      while (ng - ok > 1) {
        std::ptrdiff_t mid = (ok + ng) / 2;
        if (compare_(key, *children[mid]->min_key))
          ng = mid;
        else
          ok = mid;
      }
      return ok;
    }

    /**
     * @brief 部分木内の index 番目の要素の位置を返す。
     * @param index この Internal を根とする部分木内での 0-indexed の順位。
     * @return 要素を格納する Leaf と Leaf 内の位置。
     *
     * ### 計算量
     * O(NodeCapacity log_{NodeCapacity} N)
     */
    std::pair<Leaf*, std::ptrdiff_t> nth(std::size_t index) const {
      assert(index < this->size);

      Node* node = this;
      while (!node->is_leaf()) {
        auto* internal = static_cast<Internal*>(node);
        for (std::size_t i = 0; i < internal->count; ++i) {
          if (index < internal->children[i]->size) {
            node = internal->children[i].get();
            break;
          }
          index -= internal->children[i]->size;
        }
      }
      return {static_cast<Leaf*>(node), static_cast<std::ptrdiff_t>(index)};
    }
  };

  /**
   * @brief B+ tree 上の要素を指すイテレータ。
   * @details 通常要素は `(leaf_, key_index_)` で表す。最左 Leaf の
   * `key_index_ == -1` は `bbegin()`、最右 Leaf の
   * `key_index_ == leaf_->count` は `end()` を表す。
   *
   * C++20 の `std::random_access_iterator` コンセプトを満たす。
   * ただし木構造上、任意距離の移動やイテレータ差分は厳密には O(1) ではなく
   * O(NodeCapacity log_{NodeCapacity} N) となり得る。
   */
  struct Iterator {
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using pointer = const Key*;
    using reference = const Key&;

    Leaf* leaf_ = nullptr;
    std::ptrdiff_t key_index_ = 0;

    Iterator() = default;
    Iterator(Leaf* node, std::ptrdiff_t key_index)
        : leaf_(node), key_index_(key_index) {}

    const Key& operator*() const {
      assert(leaf_ != nullptr);
      assert(0 <= key_index_ &&
             key_index_ < static_cast<std::ptrdiff_t>(leaf_->count));
      return leaf_->keys[key_index_];
    }

    const Key* operator->() const {
      return &operator*();
    }

    Iterator& operator++() {
      assert(!(leaf_->next == nullptr &&
               static_cast<std::ptrdiff_t>(leaf_->count) == key_index_));
      ++key_index_;
      if (leaf_->next != nullptr &&
          static_cast<std::ptrdiff_t>(leaf_->count) == key_index_) {
        leaf_ = leaf_->next;
        key_index_ = 0;
      }
      return *this;
    }

    Iterator operator++(signed) {
      Iterator result = *this;
      ++*this;
      return result;
    }

    Iterator& operator--() {
      assert(leaf_ != nullptr);
      assert(key_index_ >= 0);
      --key_index_;

      if (key_index_ == -1 && leaf_->prev != nullptr) {
        leaf_ = leaf_->prev;
        key_index_ = static_cast<std::ptrdiff_t>(leaf_->count) - 1;
      }

      return *this;
    }

    Iterator operator--(signed) {
      Iterator result = *this;
      --*this;
      return result;
    }

    Iterator& operator+=(std::ptrdiff_t offset) {
      if (std::abs(offset) <= static_cast<std::ptrdiff_t>(kNodeCapacity))
        advance_small(offset);
      else
        advance_large(offset);
      return *this;
    }

    void advance_large(std::ptrdiff_t offset) {
      key_index_ += offset;
      assert(leaf_->parent != nullptr);
      for (std::size_t i = 0; i < leaf_->position; ++i) {
        key_index_ += leaf_->parent->children[i]->size;
      }
      Internal* node = leaf_->parent;

      while (!(0 <= key_index_ &&
               key_index_ < static_cast<std::ptrdiff_t>(node->size))) {
        if (node->parent == nullptr) {
          if (key_index_ == -1) {
            leaf_ = node->nth(0).first;
            key_index_ = -1;
            return;
          }
          assert(key_index_ == static_cast<std::ptrdiff_t>(node->size));
          leaf_ = node->nth(node->size - 1).first;
          key_index_ = leaf_->count;
          return;
        }
        assert(node->parent != nullptr);
        for (std::size_t i = 0; i < node->position; ++i) {
          key_index_ += node->parent->children[i]->size;
        }
        node = node->parent;
      }

      auto [leaf, index] = node->nth(static_cast<std::size_t>(key_index_));
      leaf_ = leaf;
      key_index_ = index;
    }

    void advance_small(std::ptrdiff_t offset) {
      key_index_ += offset;

      while (!(0 <= key_index_ &&
               key_index_ < static_cast<std::ptrdiff_t>(leaf_->count))) {
        if (key_index_ < 0) {
          if (leaf_->prev == nullptr) {
            assert(key_index_ == -1);
            break;
          }
          leaf_ = leaf_->prev;
          key_index_ += leaf_->count;
        } else {
          if (leaf_->next == nullptr) {
            assert(key_index_ == static_cast<std::ptrdiff_t>(leaf_->count));
            break;
          }
          key_index_ -= leaf_->count;
          leaf_ = leaf_->next;
        }
        assert(leaf_ != nullptr);
      }
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
      if (lhs.leaf_ == rhs.leaf_) return lhs.key_index_ - rhs.key_index_;
      Internal* lnode = lhs.leaf_->parent;
      std::ptrdiff_t lindex = lhs.key_index_;
      for (std::size_t i = 0; i < lhs.leaf_->position; ++i)
        lindex += lnode->children[i]->size;
      Internal* rnode = rhs.leaf_->parent;
      std::ptrdiff_t rindex = rhs.key_index_;
      for (std::size_t i = 0; i < rhs.leaf_->position; ++i)
        rindex += rnode->children[i]->size;

      while (lnode != rnode) {
        for (std::size_t i = 0; i < lnode->position; ++i)
          lindex += lnode->parent->children[i]->size;
        lnode = lnode->parent;

        for (std::size_t i = 0; i < rnode->position; ++i)
          rindex += rnode->parent->children[i]->size;
        rnode = rnode->parent;
      }
      return lindex - rindex;
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

 public:
  using iterator = Iterator;
  using const_iterator = Iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using key_compare = Compare;

  static_assert(std::random_access_iterator<iterator>);

 private:
  std::size_t size_ = 0;
  struct Header {
    std::unique_ptr<Node> root_ = nullptr;
    Leaf* first_leaf_ = nullptr;
    Leaf* last_leaf_ = nullptr;
  } header_;

  void init() {
    size_ = 0;
    auto root = std::make_unique<Leaf>();
    Leaf* leaf = root.get();

    header_.root_ = std::move(root);
    header_.first_leaf_ = leaf;
    header_.last_leaf_ = leaf;
  }
  bool equivalent(const Key& lhs, const Key& rhs) const {
    return !compare_(lhs, rhs) && !compare_(rhs, lhs);
  }

  void cut_invalid_root() {
    if (size_ == 0) return init();

    while (true) {
      if (header_.root_->is_leaf()) return;
      Internal* root = static_cast<Internal*>(header_.root_.get());
      if (root->count > 1) return;
      auto new_root = std::move(root->children[0]);
      new_root->parent = nullptr;
      new_root->position = 0;
      header_.root_ = std::move(new_root);
    }
  }

  void build(std::vector<Key>& values) {
    if (values.empty()) {
      init();
      return;
    }

    size_ = values.size();
    constexpr std::size_t kMaxCount = kNodeCapacity - 1;
    const std::size_t leaf_count = (values.size() + kMaxCount - 1) / kMaxCount;
    const std::size_t base_leaf_size = values.size() / leaf_count;
    const std::size_t extra_leaf_count = values.size() % leaf_count;

    std::vector<std::unique_ptr<Node>> level;
    level.reserve(leaf_count);
    Leaf* first_leaf = nullptr;
    Leaf* prev_leaf = nullptr;

    std::size_t value_index = 0;
    for (std::size_t i = 0; i < leaf_count; ++i) {
      const std::size_t count = base_leaf_size + (i < extra_leaf_count ? 1 : 0);
      auto leaf = std::make_unique<Leaf>();
      leaf->count = count;
      leaf->size = count;

      for (std::size_t j = 0; j < count; ++j)
        leaf->keys[j] = std::move(values[value_index++]);

      leaf->set_min_key();
      if (first_leaf == nullptr) first_leaf = leaf.get();
      leaf->prev = prev_leaf;
      if (prev_leaf != nullptr) prev_leaf->next = leaf.get();

      prev_leaf = leaf.get();
      level.push_back(std::move(leaf));
    }
    Leaf* last_leaf = prev_leaf;

    while (level.size() > 1) {
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

    header_.root_ = std::move(level[0]);
    header_.root_->parent = nullptr;
    header_.root_->position = 0;
    header_.first_leaf_ = first_leaf;
    header_.last_leaf_ = last_leaf;
  }

  void swap_tree(Bplustree_Set& other) noexcept {
    using std::swap;
    swap(size_, other.size_);
    swap(header_, other.header_);
  }

 public:
  /**
   * @brief 空の B+ tree を構築する。
   * @details
   * 比較器はデフォルト構築される。
   *
   * ### 計算量
   * O(1)
   */
  Bplustree_Set()
    requires std::default_initializable<Compare>
      : compare_() {
    init();
  }

  /**
   * @brief 指定した比較器を用いて空の B+ tree を構築する。
   * @param compare 要素の順序を定義する比較器。
   *
   * ### 計算量
   * O(1)
   */
  explicit Bplustree_Set(Compare compare) : compare_(std::move(compare)) {
    init();
  }

  /**
   * @brief イテレータ範囲 `[first, last)` の要素から集合を構築する。
   * @tparam InputIt 入力イテレータ型。
   * @param first 範囲の先頭。
   * @param last 範囲の終端。
   * @details
   * 入力がすでに `Compare` に従ってソートされている場合は、
   * B+ tree を直接一括構築する。
   *
   * 入力がソートされていない場合は内部でソートしてから構築する。
   * `SetType::Set` の場合は重複要素を除去する。
   *
   * ### 計算量
   * ソート済みの場合 O(N)。
   * それ以外の場合 O(N log N)。
   */
  template <std::input_iterator InputIt>
  Bplustree_Set(InputIt first, InputIt last)
    requires std::default_initializable<Compare>
      : compare_() {
    assign(first, last);
  }

  template <std::input_iterator InputIt>
  Bplustree_Set(InputIt first, InputIt last, Compare compare)
      : compare_(std::move(compare)) {
    assign(first, last);
  }

  /**
   * @brief 別の B+ tree をコピーして構築する。
   * @param other コピー元。
   * @details
   * 比較器と全要素をコピーし、木構造を一括構築し直す。
   * `other` を指す Iterator は無効化されない。
   *
   * ### 計算量
   * O(other.size_)
   */
  Bplustree_Set(const Bplustree_Set& other)
    requires std::copy_constructible<Compare>
      : Bplustree_Set(other.begin(), other.end(), other.compare_) {}

  /**
   * @brief 別の B+ tree の内容をコピー代入する。
   * @param other コピー元の B+ tree。
   * @return `*this`。
   * @details
   * `other` の全要素をコピーして新しい木を構築し、
   * 現在の内容と置き換える。
   * 例外安全性は特に保証しない。
   * 通常の競プロ用途では例外を投げる Key / Compare を想定しない。
   *
   * `*this` を指していた既存の Iterator はすべて無効化される。
   * `other` を指す Iterator は無効化されない。
   *
   * ### 計算量
   * 代入前の要素数を N、`other` の要素数を M として O(N + M)。
   */
  Bplustree_Set& operator=(const Bplustree_Set& other)
    requires std::is_copy_assignable_v<Compare>
  {
    if (this == &other) return *this;

    compare_ = other.compare_;
    assign(other.begin(), other.end());
    return *this;
  }

  /**
   * @brief 別の B+ tree の内容をムーブして構築する。
   * @param other 移動元。
   * @details
   * 木構造の所有権を引き継ぐ。
   * 比較器もmoveするため、移動後の `other`
   * に対して保証する操作は破棄と再代入のみ。 `other` を指していた Iterator
   * はすべて無効化される。
   *
   * ### 計算量
   * O(1)
   */
  Bplustree_Set(Bplustree_Set&& other)
    requires std::move_constructible<Compare>
      : compare_(std::move(other.compare_)),
        size_(other.size_),
        header_(std::move(other.header_)) {}

  /**
   * @brief 別の B+ tree の内容をムーブ代入する。
   * @param other 移動元の B+ tree。
   * @return `*this`。
   * @details
   * `other` の木構造の所有権を引き継ぐ。
   * 比較器も move するため、移動後の `other` に対して保証する操作は
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
  Bplustree_Set& operator=(Bplustree_Set&& other)
    requires std::is_move_assignable_v<Compare>
  {
    if (this == &other) return *this;

    this->compare_ = std::move(other.compare_);
    swap_tree(other);
    return *this;
  }

  /**
   * @brief 2つの B+ tree の内容を交換する。
   * @details
   * 比較器を含む全状態を交換する。
   * 両コンテナの既存 Iterator はすべて無効化される。
   *
   * ### 計算量
   * O(1)
   */
  friend void swap(Bplustree_Set& lhs, Bplustree_Set& rhs) noexcept(
      std::is_nothrow_swappable_v<Compare>)
    requires std::swappable<Compare>
  {
    using std::swap;
    swap(lhs.compare_, rhs.compare_);
    lhs.swap_tree(rhs);
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
   * 入力が `Compare` に従ってソート済みの場合は線形時間で構築する。
   * ソートされていない場合は内部でソートする。
   * `SetType::Set` の場合は等価な要素を除去する。
   *
   * この操作によって既存の Iterator はすべて無効化される。
   *
   * ### 計算量
   * 代入前の要素数を N、入力範囲の要素数を M として、
   * 入力がソート済みの場合 O(N + M)。
   * それ以外の場合 O(N + M log M)。
   */
  template <std::input_iterator InputIt>
  void assign(InputIt first, InputIt last) {
    std::vector<Key> values;

    for (; first != last; ++first) {
      values.push_back(*first);
    }

    auto compare = std::ref(compare_);

    if (!std::is_sorted(values.begin(), values.end(), compare)) {
      std::sort(values.begin(), values.end(), compare);
    }

    if constexpr (!kIsMultiSet) {
      auto new_end = std::unique(values.begin(), values.end(),
                                 [&](const Key& lhs, const Key& rhs) {
                                   return equivalent(lhs, rhs);
                                 });
      values.erase(new_end, values.end());
    }
    build(values);
  }

  /**
   * @brief 使用中の比較器を返す。
   * @return 比較器への const 参照。
   *
   * ### 計算量
   * O(1)
   */
  const Compare& key_comp() const noexcept {
    return compare_;
  }

  /**
   * @brief `begin()` の1つ前を表す番兵イテレータを返す。
   * @details 主に `max_lt()` / `max_le()`
   * で該当要素が存在しない場合に使用する。
   * このイテレータを逆参照してはならない。
   * @return `begin()` の1つ前を表すイテレータ。
   * ### 計算量
   * O(1)
   */
  iterator bbegin() const {
    return {header_.first_leaf_, -1};
  }

  /**
   * @brief 最小要素を指すイテレータを返す。
   * @return 空なら `end()`、そうでなければ最小要素を指すイテレータ。
   * ### 計算量
   * O(1)
   */
  iterator begin() const noexcept {
    return {header_.first_leaf_, 0};
  }

  /**
   * @brief 末尾の次を表すイテレータを返す。
   * @return 末尾イテレータ。
   * ### 計算量
   * O(1)
   */
  iterator end() const noexcept {
    return {header_.last_leaf_,
            static_cast<std::ptrdiff_t>(header_.last_leaf_->count)};
  }

  /**
   * @brief `begin()` と同じイテレータを返す。
   * @return 先頭イテレータ。
   * ### 計算量
   * O(1)
   */
  const_iterator cbegin() const noexcept {
    return begin();
  }

  /**
   * @brief `end()` と同じイテレータを返す。
   * @return 末尾イテレータ。
   * ### 計算量
   * O(1)
   */
  const_iterator cend() const noexcept {
    return end();
  }

  /**
   * @brief 最大要素から逆順に走査するための先頭イテレータを返す。
   * @return 空なら `rend()`、そうでなければ最大要素を指す逆イテレータ。
   * ### 計算量
   * O(1)
   */
  reverse_iterator rbegin() const {
    return reverse_iterator(end());
  }

  /**
   * @brief 逆順走査の末尾イテレータを返す。
   * @return `begin()` の直前を表す逆イテレータ。
   * ### 計算量
   * O(1)
   */
  reverse_iterator rend() const {
    return reverse_iterator(begin());
  }

  /**
   * @brief `rbegin()` と同じ逆イテレータを返す。
   * @return 逆順走査の先頭イテレータ。
   * ### 計算量
   * O(1)
   */
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }

  /**
   * @brief `rend()` と同じ逆イテレータを返す。
   * @return 逆順走査の末尾イテレータ。
   * ### 計算量
   * O(1)
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
   * @brief 集合が空か判定する。
   * @return 空なら `true`、そうでなければ `false`。
   * ### 計算量
   * O(1)
   */
  bool empty() const noexcept {
    return size_ == 0;
  }

  /**
   * @brief `key` より小さい最大の要素を指すイテレータを返す。
   * @param key 境界となるキー。
   * @return 該当要素が存在すればそのイテレータ、存在しなければ `bbegin()`。
   * ### 計算量
   * O(log N)
   */
  iterator max_lt(const Key& key) const {
    Node* node = header_.root_.get();
    std::ptrdiff_t index;
    while (true) {
      if (node->is_leaf()) {
        index = static_cast<Leaf*>(node)->max_lt(key, compare_);
        break;
      } else {
        index = static_cast<Internal*>(node)->max_lt(key, compare_);
        if (index < 0) return bbegin();
        node = static_cast<Internal*>(node)->children[index].get();
      }
    }

    return {static_cast<Leaf*>(node), index};
  }

  /**
   * @brief `key` 以下の最大の要素を指すイテレータを返す。
   * @param key 境界となるキー。
   * @return 該当要素が存在すればそのイテレータ、存在しなければ `bbegin()`。
   * ### 計算量
   * O(log N)
   */
  iterator max_le(const Key& key) const {
    Node* node = header_.root_.get();
    std::ptrdiff_t index;
    while (true) {
      if (node->is_leaf()) {
        index = static_cast<Leaf*>(node)->max_le(key, compare_);
        break;
      } else {
        index = static_cast<Internal*>(node)->max_le(key, compare_);
        if (index < 0) return bbegin();
        node = static_cast<Internal*>(node)->children[index].get();
      }
    }

    return {static_cast<Leaf*>(node), index};
  }

  /**
   * @brief `key` 以上の最小の要素を指すイテレータを返す。
   * @param key 検索するキー。
   * @return 該当要素が存在すればそのイテレータ、存在しなければ `end()`。
   * ### 計算量
   * O(log N)
   */
  iterator lower_bound(const Key& key) const {
    auto it = max_lt(key);
    return ++it;
  }

  /**
   * @brief `key` より大きい最小の要素を指すイテレータを返す。
   * @param key 検索するキー。
   * @return 該当要素が存在すればそのイテレータ、存在しなければ `end()`。
   * ### 計算量
   * O(log N)
   */
  iterator upper_bound(const Key& key) const {
    auto it = max_le(key);
    return ++it;
  }

  /**
   * @brief `key` と等価な要素範囲を返す。
   * @param key 検索するキー。
   * @return `{lower_bound(key), upper_bound(key)}`。
   * ### 計算量
   * O(log N)
   */
  std::pair<iterator, iterator> equal_range(const Key& key) const {
    return {lower_bound(key), upper_bound(key)};
  }

  /**
   * @brief `key` より大きい最小の要素を指すイテレータを返す。
   * @details `upper_bound(key)` と同値。
   * @param key 検索するキー。
   * @return 該当要素が存在すればそのイテレータ、存在しなければ `end()`。
   * ### 計算量
   * O(log N)
   */
  iterator min_gt(const Key& key) const {
    return upper_bound(key);
  }

  /**
   * @brief `key` 以上の最小の要素を指すイテレータを返す。
   * @details `lower_bound(key)` と同値。
   * @param key 検索するキー。
   * @return 該当要素が存在すればそのイテレータ、存在しなければ `end()`。
   * ### 計算量
   * O(log N)
   */
  iterator min_ge(const Key& key) const {
    return lower_bound(key);
  }

  /**
   * @brief `key` と等価な要素を検索する。
   * @param key 検索するキー。
   * @return 見つかればその要素を指すイテレータ、見つからなければ `end()`。
   * ### 計算量
   * O(log N)
   */
  iterator find(const Key& key) const {
    auto it = max_le(key);
    if (it == bbegin()) return end();
    if (equivalent(*it, key)) return it;
    return end();
  }

  /**
   * @brief `key` と等価な要素数を返す。
   * @param key 数えるキー。
   * @return `key` と等価な要素数。
   * ### 計算量
   * O(log N)
   */
  size_type count(const Key& key) const {
    return static_cast<std::size_t>(upper_bound(key) - lower_bound(key));
  }

  /**
   * @brief `key` と等価な要素が存在するか判定する。
   * @param key 検索するキー。
   * @return 存在すれば `true`、そうでなければ `false`。
   * ### 計算量
   * O(log N)
   */
  bool contains(const Key& key) const {
    return find(key) != end();
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
    assert(index < size_);

    if (header_.root_->is_leaf())
      return {static_cast<Leaf*>(header_.root_.get()),
              static_cast<std::ptrdiff_t>(index)};
    auto [leaf, key_index] =
        static_cast<Internal*>(header_.root_.get())->nth(index);
    return {leaf, key_index};
  }

  /**
   * @brief 0-indexed で index 番目の要素を返す。
   * @param index 要素の順位。
   * @return index 番目の要素への const 参照。
   * @details
   * `index < size()` を満たす必要がある。
   *
   * ### 計算量
   * O(kNodeCapacity log N)
   */
  const Key& operator[](std::size_t index) const {
    return *nth(index);
  }

  /**
   * @brief 要素を挿入する。
   * @details `SetType::Set` では等価な要素が既に存在する場合は挿入しない。
   * `SetType::MultiSet` では常に1要素挿入する。更新により既存イテレータは
   * 無効化される場合がある。
   * @param key 挿入するキー。
   * @return 挿入位置のイテレータと、実際に挿入されたかを表す真偽値の組。
   * `MultiSet` では真偽値は常に `true`。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  std::pair<iterator, bool> insert(const Key& key) {
    auto it = lower_bound(key);
    Leaf* leaf = it.leaf_;
    std::ptrdiff_t index = it.key_index_;

    if (!kIsMultiSet)
      if (index < static_cast<std::ptrdiff_t>(leaf->count) &&
          equivalent(key, leaf->keys[index]))
        return {it, false};

    ++size_;

    Leaf* inserted_leaf = leaf;
    std::ptrdiff_t inserted_index = index;

    std::unique_ptr<Node> right = nullptr;

    if (leaf->insert(key, index)) {
      auto right_leaf = leaf->split();
      Leaf* right_leaf_ptr = right_leaf.get();

      if (index >= static_cast<std::ptrdiff_t>(kNodeHalf)) {
        inserted_leaf = right_leaf_ptr;
        inserted_index = index - static_cast<std::ptrdiff_t>(kNodeHalf);
      }
      right = std::move(right_leaf);
    }

    auto end_leaf = header_.last_leaf_;
    if (end_leaf->next != nullptr) end_leaf = end_leaf->next;
    header_.last_leaf_ = end_leaf;

    Internal* parent = leaf->parent;
    Node* left = leaf;

    while (parent != nullptr) {
      if (right != nullptr) {
        if (parent->insert(std::move(right), left->position + 1))
          right = parent->split();
        else
          right = nullptr;
      } else {
        ++parent->size;
      }

      left = parent;
      parent = left->parent;
    }
    if (right != nullptr) {
      auto new_root = std::make_unique<Internal>();
      new_root->insert(std::move(header_.root_), 0);
      new_root->insert(std::move(right), 1);
      header_.root_ = std::move(new_root);
    }
    return {{inserted_leaf, inserted_index}, true};
  }

  /**
   * @brief イテレータ範囲 `[first, last)` の要素を追加する。
   * @details 現在の要素は保持される。
   *
   * ### 計算量
   * M を範囲の要素数として
   * O(M NodeCapacity log_{NodeCapacity}(N + M))。
   */
  template <std::input_iterator InputIt>
  void insert(InputIt first, InputIt last) {
    std::vector<Key> values(first, last);
    for (const Key& key : values) {
      insert(key);
    }
  }

  /**
   * @brief `key` と等価な要素を削除する。
   * @param key 削除するキー。
   * @param single_erase `true` の場合は最初の1要素だけを削除する。
   * `false` の場合は等価な要素をすべて削除する。
   * @return 削除した要素数。
   * ### 計算量
   * `M` を削除要素数として O((M + 1) NodeCapacity log_{NodeCapacity} N)
   */
  size_type erase(const Key& key, bool single_erase = false) {
    std::size_t count = 0;

    iterator it = lower_bound(key);
    while (it != end() && equivalent(*it, key)) {
      it = erase(it);
      ++count;
      if (single_erase) break;
    }
    return count;
  }

  /**
   * @brief 指定した位置の要素を1つ削除する。
   * @param it 削除する要素を指すイテレータ。`end()` は指定できない。
   * @return 削除した要素の次を指すイテレータ。
   * @details 更新により `it` を含む既存イテレータは無効化される場合がある。
   * ### 計算量
   * O(NodeCapacity log_{NodeCapacity} N)
   */
  iterator erase(const iterator it) {
    assert(it != bbegin());
    assert(it != end());

    --size_;
    if (size_ == 0) {
      init();
      return begin();
    }

    Leaf* leaf = it.leaf_;
    std::size_t index = static_cast<std::size_t>(it.key_index_);

    // 次の iterator を先に保存しておく
    iterator result = it;
    if (it.key_index_ == static_cast<std::ptrdiff_t>(leaf->count) - 1) ++result;

    const bool result_is_end = result == end();
    bool need_erase = leaf->erase(index);

    auto end_leaf = header_.last_leaf_;

    if (need_erase) {
      if (leaf == header_.first_leaf_) header_.first_leaf_ = leaf->next;
      if (leaf == end_leaf) end_leaf = leaf->prev;
    }

    header_.last_leaf_ = end_leaf;

    Internal* parent = leaf->parent;
    Node* child = leaf;
    while (parent != nullptr) {
      if (need_erase)
        need_erase = parent->erase(child->position);
      else {
        --parent->size;
        parent->set_min_key();
      }
      child = parent;
      parent = child->parent;
    }
    cut_invalid_root();
    if (result_is_end) return end();
    return result;
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
    std::vector<Key> values(begin(), end());
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
};

template <class T = long long>
using BSet = Bplustree_Set<T>;
template <class T = long long>
using BMultiSet = Bplustree_Set<T, SetType::MultiSet>;

// compare_から型推論するためのtrait
template <class T>
struct BinaryCallableTraits;

template <class R, class C, class A, class B>
struct BinaryCallableTraits<R (C::*)(A, B) const> {
  using FirstArgumentType = A;
  using SecondArgumentType = B;
};

template <class R, class C, class A, class B>
struct BinaryCallableTraits<R (C::*)(A, B)> {
  using FirstArgumentType = A;
  using SecondArgumentType = B;
};

template <class R, class C, class A, class B>
struct BinaryCallableTraits<R (C::*)(A, B) const noexcept>
    : BinaryCallableTraits<R (C::*)(A, B) const> {};

template <class R, class C, class A, class B>
struct BinaryCallableTraits<R (C::*)(A, B) noexcept>
    : BinaryCallableTraits<R (C::*)(A, B)> {};

template <class Compare>
struct CompareKey {
 private:
  using Traits = BinaryCallableTraits<decltype(&Compare::operator())>;
  using First = std::remove_cvref_t<typename Traits::FirstArgumentType>;
  using Second = std::remove_cvref_t<typename Traits::SecondArgumentType>;

  static_assert(std::same_as<First, Second>);

 public:
  using type = First;
};

template <class Compare>
using CompareKeyType = typename CompareKey<std::decay_t<Compare>>::type;
/**
 * @brief 指定した比較器を持つ B+ tree set を生成する。
 * @tparam NodeCapacity Node の容量。
 * @param compare 比較器。2引数の非 generic lambda を想定する。
 * @return 指定した比較器を使用する B+ tree set。
 * @details
 * `compare` の2つの引数型から `Key` を自動推定する。
 * 引数が参照型や const 修飾されている場合、それらを除いた型を `Key` とする。
 *
 * generic lambda や、`operator()` がオーバーロードされた関数オブジェクトには
 * 対応しない。
 *
 * 比較器は B+ tree 内部に値として保持される。ただし比較器が外部オブジェクトを
 * 参照キャプチャしている場合、そのオブジェクトは B+ tree より長く生存する
 * 必要がある。
 *
 * また、比較器が参照する外部状態を変更することで既存要素間の順序関係が
 * 変化すると、B+ tree の順序不変条件が崩れ、検索結果などが不正になる。
 *
 * ### 計算量
 * O(1)
 */
template <std::size_t NodeCapacity = 64, class Compare>
auto make_bset(Compare&& compare) {
  using CompareType = std::decay_t<Compare>;
  using Key = CompareKeyType<CompareType>;

  return Bplustree_Set<Key, SetType::Set, CompareType, NodeCapacity>(
      std::forward<Compare>(compare));
}

/**
 * @brief 指定した比較器を持つ B+ tree multiset を生成する。
 * @tparam NodeCapacity Node の容量。
 * @param compare 比較器。2引数の非 generic lambda を想定する。
 * @return 指定した比較器を使用する B+ tree multiset。
 * @details
 * `compare` の2つの引数型から `Key` を自動推定する。
 * 引数が参照型や const 修飾されている場合、それらを除いた型を `Key` とする。
 *
 * generic lambda や、`operator()` がオーバーロードされた関数オブジェクトには
 * 対応しない。
 *
 * 比較器は B+ tree 内部に値として保持される。ただし比較器が外部オブジェクトを
 * 参照キャプチャしている場合、そのオブジェクトは B+ tree より長く生存する
 * 必要がある。
 *
 * また、比較器が参照する外部状態を変更することで既存要素間の順序関係が
 * 変化すると、B+ tree の順序不変条件が崩れ、検索結果などが不正になる。
 *
 * ### 計算量
 * O(1)
 */
template <std::size_t NodeCapacity = 64, class Compare>
auto make_bmultiset(Compare&& compare) {
  using CompareType = std::decay_t<Compare>;
  using Key = CompareKeyType<CompareType>;

  return Bplustree_Set<Key, SetType::MultiSet, CompareType, NodeCapacity>(
      std::forward<Compare>(compare));
}

/**
 * @brief 指定した比較器を持つ B+ tree set を範囲構築する。
 * @tparam NodeCapacity Node の容量。
 * @param first 範囲の先頭。
 * @param last 範囲の終端。
 * @param compare 比較器。2引数のlambda
 * を想定する。範囲から型推論可能なため、genericでも可。
 * @return 指定した比較器を使用し、範囲を要素として構築した B+ tree set。
 * @details
 * `InputIt` の value type から `Key` を自動推定する。
 * 比較器は B+ tree 内部に値として保持される。ただし比較器が外部オブジェクトを
 * 参照キャプチャしている場合、そのオブジェクトは B+ tree より長く生存する
 * 必要がある。
 *
 * また、比較器が参照する外部状態を変更することで既存要素間の順序関係が
 * 変化すると、B+ tree の順序不変条件が崩れ、検索結果などが不正になる。
 *
 * ### 計算量
 * N を範囲の要素数として、
 * ソート済みの場合 O(N)。
 * それ以外の場合 O(N log N)。
 */
template <std::size_t NodeCapacity = 64, std::input_iterator InputIt,
          class Compare>
auto make_bset(InputIt first, InputIt last, Compare&& compare) {
  using Key = std::iter_value_t<InputIt>;
  using CompareType = std::decay_t<Compare>;

  return Bplustree_Set<Key, SetType::Set, CompareType, NodeCapacity>(
      first, last, std::forward<Compare>(compare));
}

/**
 * @brief 指定した比較器を持つ B+ tree multiset を範囲構築する。
 * @tparam NodeCapacity Node の容量。
 *@param first 範囲の先頭。
 * @param last 範囲の終端。
 * @param compare 比較器。2引数のlambda
 * を想定する。範囲から型推論可能なため、genericでも可。
 * @return 指定した比較器を使用し、範囲を要素として構築した B+ tree multiset。
 * @details
 * `InputIt` の value type から `Key` を自動推定する。
 * 比較器は B+ tree 内部に値として保持される。ただし比較器が外部オブジェクトを
 * 参照キャプチャしている場合、そのオブジェクトは B+ tree より長く生存する
 * 必要がある。
 *
 * また、比較器が参照する外部状態を変更することで既存要素間の順序関係が
 * 変化すると、B+ tree の順序不変条件が崩れ、検索結果などが不正になる。
 *
 * ### 計算量
 * N を範囲の要素数として、
 * ソート済みの場合 O(N)。
 * それ以外の場合 O(N log N)。
 */
template <std::size_t NodeCapacity = 64, std::input_iterator InputIt,
          class Compare>
auto make_bmultiset(InputIt first, InputIt last, Compare&& compare) {
  using Key = std::iter_value_t<InputIt>;
  using CompareType = std::decay_t<Compare>;

  return Bplustree_Set<Key, SetType::MultiSet, CompareType, NodeCapacity>(
      first, last, std::forward<Compare>(compare));
}