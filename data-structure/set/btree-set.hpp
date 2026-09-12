#pragma once

// i番目がO(log)で取れるsetの構想
// iteratorのdiffや(+n)がlogで取れるようにする

// まずBtreeで挿入可能な列の構造を作り、それを利用してset,
// multisetなどを実装する

// パフォーマンスが高いとうれしい。
// まずはインターフェースを書いてほしい
#include <array>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <concepts>
#include <functional>
#include <optional>
#include <utility>

template <class Key, class Compare = std::less<Key>,
          std::size_t NodeCapMax = 63>
class Btree_Set {
 private:
  static_assert(NodeCapMax >= 3);
  static_assert(std::default_initializable<Key>);

  // 非根ノードは kMinKeys 以上 kMaxKeys 以下のキーを持つ。
  static constexpr std::size_t kMinKeys = (NodeCapMax - 1) / 2;
  static constexpr std::size_t kMaxKeys = NodeCapMax;
  static constexpr std::size_t kMaxChildren = NodeCapMax + 1;

  struct Node {
    // keys[0, key_count) だけが有効なキーである。
    std::array<Key, kMaxKeys> keys{};
    // 葉では children はすべて nullptr。内部ノードでは children の
    // [0, key_count + 1) が有効で、キー数より常に 1 多い。
    std::array<Node*, kMaxChildren> children{};
    std::size_t key_count = 0;
    std::size_t subtree_size = 0;
    Node* parent = nullptr;
    std::size_t position = 0;
    bool is_leaf = true;

    Node() = default;

    explicit Node(Key key) : key_count(1), subtree_size(1) {
      keys[0] = std::move(key);
    }

    struct SplitResult {
      Key separator;
      Node* right;
    };

    // children が指す部分木から、このノードの部分木サイズを再計算する。
    void recalculate_subtree_size() {
      subtree_size = key_count;
      if (!is_leaf) {
        for (std::size_t i = 0; i <= key_count; ++i) {
          subtree_size += children[i]->subtree_size;
        }
      }
    }

    // key_count < kMaxKeys のノードに key を key_index 番目として入れる。
    // 内部ノードでは、key の右側の子 right_child も同時に追加する。
    // このノードの subtree_size は再計算するが、祖先の更新は呼び出し側が行う。
    void insert_non_full(const Key& key, Node* right_child,
                         std::size_t key_index) {
      assert(key_index <= key_count);
      assert(key_count < kMaxKeys);
      assert(is_leaf == (right_child == nullptr));

      for (std::size_t i = key_count; i > key_index; --i) {
        keys[i] = std::move(keys[i - 1]);
      }
      keys[key_index] = key;

      if (!is_leaf) {
        for (std::size_t i = key_count + 1; i > key_index + 1; --i) {
          children[i] = children[i - 1];
        }
        children[key_index + 1] = right_child;
        right_child->parent = this;

        // 子の挿入位置より右にある子は添字が一つずれる。
        for (std::size_t i = key_index + 1; i <= key_count + 1; ++i) {
          children[i]->position = i;
        }
      }

      ++key_count;
      ++subtree_size;
    }

    // 満杯のノードへキーを挿入し、中央値を親へ渡すために分割する。
    // 呼び出し後、this は左ノード、返り値 right は右ノードになる。
    // 返り値 separator を親へ挿入する責務と、祖先の subtree_size 更新は
    // 呼び出し側が持つ。
    SplitResult split_and_insert(const Key& key, Node* right_child,
                                 std::size_t key_index) {
      assert(key_count == kMaxKeys);
      assert(key_index <= key_count);
      assert(is_leaf == (right_child == nullptr));

      // あふれた 1 個を含むキー列と子列を一時領域で作る。
      std::array<Key, kMaxKeys + 1> all_keys{};
      for (std::size_t i = 0; i < key_count; ++i) {
        all_keys[i] = std::move(keys[i]);
      }
      for (std::size_t i = key_count; i > key_index; --i) {
        all_keys[i] = std::move(all_keys[i - 1]);
      }
      all_keys[key_index] = key;

      std::array<Node*, kMaxChildren + 1> all_children{};
      if (!is_leaf) {
        for (std::size_t i = 0; i <= key_count; ++i) {
          all_children[i] = children[i];
        }
        for (std::size_t i = key_count + 1; i > key_index + 1; --i) {
          all_children[i] = all_children[i - 1];
        }
        all_children[key_index + 1] = right_child;
      }

      // kMaxKeys + 1 個のキーから中央値を親へ昇格させる。
      // 左右のノードには少なくとも kMinKeys 個のキーが残る。
      const std::size_t median_index = kMaxKeys / 2;
      const std::size_t left_key_count = median_index;
      const std::size_t right_key_count = kMaxKeys - median_index;

      Node* right = new Node();
      right->is_leaf = is_leaf;
      right->parent = parent;
      right->position = position + 1;
      right->key_count = right_key_count;

      for (std::size_t i = 0; i < left_key_count; ++i) {
        keys[i] = std::move(all_keys[i]);
      }
      for (std::size_t i = 0; i < right_key_count; ++i) {
        right->keys[i] = std::move(all_keys[median_index + 1 + i]);
      }
      key_count = left_key_count;

      if (!is_leaf) {
        for (std::size_t i = 0; i <= left_key_count; ++i) {
          children[i] = all_children[i];
          children[i]->parent = this;
          children[i]->position = i;
        }
        for (std::size_t i = left_key_count + 1; i < kMaxChildren; ++i) {
          children[i] = nullptr;
        }

        for (std::size_t i = 0; i <= right_key_count; ++i) {
          right->children[i] = all_children[median_index + 1 + i];
          right->children[i]->parent = right;
          right->children[i]->position = i;
        }
      }

      recalculate_subtree_size();
      right->recalculate_subtree_size();
      return {std::move(all_keys[median_index]), right};
    }

    void erase_from_leaf(std::size_t key_index) {
      assert(is_leaf);
      assert(key_index < key_count);

      for (std::size_t i = key_index + 1; i < key_count; ++i) {
        keys[i - 1] = std::move(keys[i]);
      }
      --key_count;
      --subtree_size;
    }

    void update_child_positions() {
      if (!is_leaf) {
        for (std::size_t i = 0; i <= key_count; ++i) {
          children[i]->parent = this;
          children[i]->position = i;
        }
      }
    }

    std::pair<Node*, std::size_t> nth(std::size_t i) const;
  };

  struct InsertResult {
    bool inserted;
    std::optional<typename Node::SplitResult> split;
  };

  // node 内で key 以上となる最初のキーの添字を返す。
  std::size_t lower_bound_index(const Node* node, const Key& key) const {
    return std::lower_bound(node->keys.begin(),
                            node->keys.begin() + node->key_count, key,
                            compare_) -
           node->keys.begin();
  }

  bool equivalent(const Key& lhs, const Key& rhs) const {
    return !compare_(lhs, rhs) && !compare_(rhs, lhs);
  }

  // key を葉まで挿入し、分割した場合だけ親へ渡す情報を返す。
  InsertResult insert_recursive(Node* node, const Key& key) {
    const std::size_t key_index = lower_bound_index(node, key);
    if (key_index < node->key_count && equivalent(node->keys[key_index], key)) {
      return {false, std::nullopt};
    }

    if (node->is_leaf) {
      if (node->key_count < kMaxKeys) {
        node->insert_non_full(key, nullptr, key_index);
        return {true, std::nullopt};
      }
      return {true, node->split_and_insert(key, nullptr, key_index)};
    }

    InsertResult child_result =
        insert_recursive(node->children[key_index], key);
    if (!child_result.inserted) {
      return {false, std::nullopt};
    }
    if (!child_result.split) {
      ++node->subtree_size;
      return {true, std::nullopt};
    }

    const auto& child_split = *child_result.split;
    if (node->key_count < kMaxKeys) {
      node->insert_non_full(child_split.separator, child_split.right,
                            key_index);
      return {true, std::nullopt};
    }
    return {true, node->split_and_insert(child_split.separator,
                                         child_split.right, key_index)};
  }

  std::pair<Node*, std::size_t> locate(const Key& key) const {
    Node* node = root_;
    while (node != nullptr) {
      const std::size_t key_index = lower_bound_index(node, key);
      if (key_index < node->key_count &&
          equivalent(node->keys[key_index], key)) {
        return {node, key_index};
      }
      if (node->is_leaf) {
        break;
      }
      node = node->children[key_index];
    }
    return {nullptr, 0};
  }

  std::pair<Node*, std::size_t> lower_bound_locate(const Key& key) const {
    Node* node = root_;
    Node* candidate_node = nullptr;
    std::size_t candidate_index = 0;

    while (node != nullptr) {
      const std::size_t key_index = lower_bound_index(node, key);
      if (key_index < node->key_count) {
        candidate_node = node;
        candidate_index = key_index;
      }
      if (node->is_leaf) {
        break;
      }
      node = node->children[key_index];
    }
    return {candidate_node, candidate_index};
  }

  std::pair<Node*, std::size_t> upper_bound_locate(const Key& key) const {
    Node* node = root_;
    Node* candidate_node = nullptr;
    std::size_t candidate_index = 0;

    while (node != nullptr) {
      std::size_t low = 0;
      std::size_t high = node->key_count;
      while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (compare_(key, node->keys[mid])) {
          high = mid;
        } else {
          low = mid + 1;
        }
      }
      if (low < node->key_count) {
        candidate_node = node;
        candidate_index = low;
      }
      if (node->is_leaf) {
        break;
      }
      node = node->children[low];
    }
    return {candidate_node, candidate_index};
  }

  std::pair<Node*, std::size_t> nth_locate(std::size_t index) const {
    if (index >= size_) {
      return {nullptr, 0};
    }

    Node* node = root_;
    while (true) {
      for (std::size_t i = 0; i <= node->key_count; ++i) {
        if (!node->is_leaf) {
          const std::size_t child_size = node->children[i]->subtree_size;
          if (index < child_size) {
            node = node->children[i];
            goto next_node;
          }
          index -= child_size;
        }
        if (i == node->key_count) {
          break;
        }
        if (index == 0) {
          return {node, i};
        }
        --index;
      }
      assert(false);

    next_node:
      continue;
    }
  }

  std::size_t index_of_node(const Node* node, std::size_t key_index) const {
    assert(node != nullptr);
    assert(key_index < node->key_count);

    std::size_t index = key_index;
    if (!node->is_leaf) {
      for (std::size_t i = 0; i <= key_index; ++i) {
        index += node->children[i]->subtree_size;
      }
    }

    while (node->parent != nullptr) {
      const Node* parent = node->parent;
      for (std::size_t i = 0; i < node->position; ++i) {
        index += parent->children[i]->subtree_size + 1;
      }
      node = parent;
    }
    return index;
  }

  std::size_t index_of_position(const Node* node, std::size_t key_index) const {
    return node == nullptr ? size_ : index_of_node(node, key_index);
  }

  static void delete_subtree(Node* node) {
    if (node == nullptr) {
      return;
    }
    if (!node->is_leaf) {
      for (std::size_t i = 0; i <= node->key_count; ++i) {
        delete_subtree(node->children[i]);
      }
    }
    delete node;
  }

  void borrow_from_left(Node* parent, std::size_t child_index) {
    Node* left = parent->children[child_index - 1];
    Node* child = parent->children[child_index];
    assert(left->key_count > kMinKeys);
    assert(left->is_leaf == child->is_leaf);

    for (std::size_t i = child->key_count; i > 0; --i) {
      child->keys[i] = std::move(child->keys[i - 1]);
    }
    if (!child->is_leaf) {
      for (std::size_t i = child->key_count + 1; i > 0; --i) {
        child->children[i] = child->children[i - 1];
      }
      child->children[0] = left->children[left->key_count];
      left->children[left->key_count] = nullptr;
    }

    child->keys[0] = std::move(parent->keys[child_index - 1]);
    parent->keys[child_index - 1] = std::move(left->keys[left->key_count - 1]);
    --left->key_count;
    ++child->key_count;

    left->update_child_positions();
    child->update_child_positions();
    left->recalculate_subtree_size();
    child->recalculate_subtree_size();
    parent->recalculate_subtree_size();
  }

  void borrow_from_right(Node* parent, std::size_t child_index) {
    Node* child = parent->children[child_index];
    Node* right = parent->children[child_index + 1];
    assert(right->key_count > kMinKeys);
    assert(right->is_leaf == child->is_leaf);

    child->keys[child->key_count] = std::move(parent->keys[child_index]);
    parent->keys[child_index] = std::move(right->keys[0]);
    for (std::size_t i = 1; i < right->key_count; ++i) {
      right->keys[i - 1] = std::move(right->keys[i]);
    }

    if (!child->is_leaf) {
      child->children[child->key_count + 1] = right->children[0];
      for (std::size_t i = 1; i <= right->key_count; ++i) {
        right->children[i - 1] = right->children[i];
      }
      right->children[right->key_count] = nullptr;
    }

    ++child->key_count;
    --right->key_count;
    child->update_child_positions();
    right->update_child_positions();
    child->recalculate_subtree_size();
    right->recalculate_subtree_size();
    parent->recalculate_subtree_size();
  }

  // children[left_index] とその右兄弟を併合し、左側のノードを返す。
  Node* merge_children(Node* parent, std::size_t left_index) {
    Node* left = parent->children[left_index];
    Node* right = parent->children[left_index + 1];
    assert(left->is_leaf == right->is_leaf);
    assert(left->key_count + 1 + right->key_count <= kMaxKeys);

    const std::size_t left_key_count = left->key_count;
    left->keys[left_key_count] = std::move(parent->keys[left_index]);
    for (std::size_t i = 0; i < right->key_count; ++i) {
      left->keys[left_key_count + 1 + i] = std::move(right->keys[i]);
    }

    if (!left->is_leaf) {
      for (std::size_t i = 0; i <= right->key_count; ++i) {
        left->children[left_key_count + 1 + i] = right->children[i];
      }
    }
    left->key_count += 1 + right->key_count;

    for (std::size_t i = left_index + 1; i < parent->key_count; ++i) {
      parent->keys[i - 1] = std::move(parent->keys[i]);
    }
    for (std::size_t i = left_index + 2; i <= parent->key_count; ++i) {
      parent->children[i - 1] = parent->children[i];
    }
    parent->children[parent->key_count] = nullptr;
    --parent->key_count;

    left->update_child_positions();
    parent->update_child_positions();
    left->recalculate_subtree_size();
    parent->recalculate_subtree_size();
    delete right;
    return left;
  }

  // child_index の子に降りる前に、削除後も下限を満たせるよう調整する。
  std::size_t prepare_child_for_erase(Node* parent, std::size_t child_index) {
    Node* child = parent->children[child_index];
    if (child->key_count > kMinKeys) {
      return child_index;
    }
    if (child_index > 0 &&
        parent->children[child_index - 1]->key_count > kMinKeys) {
      borrow_from_left(parent, child_index);
      return child_index;
    }
    if (child_index < parent->key_count &&
        parent->children[child_index + 1]->key_count > kMinKeys) {
      borrow_from_right(parent, child_index);
      return child_index;
    }
    if (child_index < parent->key_count) {
      merge_children(parent, child_index);
      return child_index;
    }
    merge_children(parent, child_index - 1);
    return child_index - 1;
  }

  Key maximum_key(Node* node) const {
    while (!node->is_leaf) {
      node = node->children[node->key_count];
    }
    return node->keys[node->key_count - 1];
  }

  Key minimum_key(Node* node) const {
    while (!node->is_leaf) {
      node = node->children[0];
    }
    return node->keys[0];
  }

  bool erase_recursive(Node* node, const Key& key) {
    std::size_t key_index = lower_bound_index(node, key);
    if (key_index < node->key_count && equivalent(node->keys[key_index], key)) {
      if (node->is_leaf) {
        node->erase_from_leaf(key_index);
        return true;
      }

      Node* left = node->children[key_index];
      Node* right = node->children[key_index + 1];
      if (left->key_count > kMinKeys) {
        const Key predecessor = maximum_key(left);
        node->keys[key_index] = predecessor;
        const bool erased = erase_recursive(left, predecessor);
        node->recalculate_subtree_size();
        return erased;
      }
      if (right->key_count > kMinKeys) {
        const Key successor = minimum_key(right);
        node->keys[key_index] = successor;
        const bool erased = erase_recursive(right, successor);
        node->recalculate_subtree_size();
        return erased;
      }

      Node* merged = merge_children(node, key_index);
      const bool erased = erase_recursive(merged, key);
      node->recalculate_subtree_size();
      return erased;
    }

    if (node->is_leaf) {
      return false;
    }

    key_index = prepare_child_for_erase(node, key_index);
    const bool erased = erase_recursive(node->children[key_index], key);
    node->recalculate_subtree_size();
    return erased;
  }

  Node* root_ = nullptr;
  std::size_t size_ = 0;
  Compare compare_{};

 public:
  class iterator {
   public:
    iterator() = default;

    const Key& operator*() const {
      assert(node_ != nullptr);
      return node_->keys[key_index_];
    }

    const Key* operator->() const {
      return &operator*();
    }

    iterator& operator++() {
      assert(node_ != nullptr);
      if (!node_->is_leaf) {
        node_ = node_->children[key_index_ + 1];
        while (!node_->is_leaf) {
          node_ = node_->children[0];
        }
        key_index_ = 0;
        return *this;
      }

      if (key_index_ + 1 < node_->key_count) {
        ++key_index_;
        return *this;
      }

      while (node_->parent != nullptr &&
             node_->position == node_->parent->key_count) {
        node_ = node_->parent;
      }
      if (node_->parent == nullptr) {
        node_ = nullptr;
        key_index_ = 0;
      } else {
        key_index_ = node_->position;
        node_ = node_->parent;
      }
      return *this;
    }

    iterator operator++(signed) {
      iterator result = *this;
      ++*this;
      return result;
    }

    iterator& operator--() {
      assert(owner_ != nullptr);
      if (node_ == nullptr) {
        node_ = owner_->root_;
        assert(node_ != nullptr);
        while (!node_->is_leaf) {
          node_ = node_->children[node_->key_count];
        }
        key_index_ = node_->key_count - 1;
        return *this;
      }
      if (!node_->is_leaf) {
        node_ = node_->children[key_index_];
        while (!node_->is_leaf) {
          node_ = node_->children[node_->key_count];
        }
        key_index_ = node_->key_count - 1;
        return *this;
      }

      if (key_index_ > 0) {
        --key_index_;
        return *this;
      }

      while (node_->parent != nullptr && node_->position == 0) {
        node_ = node_->parent;
      }
      assert(node_->parent != nullptr);
      key_index_ = node_->position - 1;
      node_ = node_->parent;
      return *this;
    }

    iterator operator--(signed) {
      iterator result = *this;
      --*this;
      return result;
    }

    iterator& operator+=(std::ptrdiff_t offset) {
      assert(owner_ != nullptr);
      const std::size_t index = owner_->index_of_position(node_, key_index_);
      if (offset >= 0) {
        assert(static_cast<std::size_t>(offset) <= owner_->size_ - index);
        const auto [node, key_index] =
            owner_->nth_locate(index + static_cast<std::size_t>(offset));
        *this = iterator{owner_, node, key_index};
      } else {
        const std::size_t distance =
            static_cast<std::size_t>(-(offset + 1)) + 1;
        assert(distance <= index);
        const auto [node, key_index] = owner_->nth_locate(index - distance);
        *this = iterator{owner_, node, key_index};
      }
      return *this;
    }

    iterator& operator-=(std::ptrdiff_t offset) {
      assert(owner_ != nullptr);
      const std::size_t index = owner_->index_of_position(node_, key_index_);
      if (offset >= 0) {
        assert(static_cast<std::size_t>(offset) <= index);
        const auto [node, key_index] =
            owner_->nth_locate(index - static_cast<std::size_t>(offset));
        *this = iterator{owner_, node, key_index};
      } else {
        const std::size_t distance =
            static_cast<std::size_t>(-(offset + 1)) + 1;
        assert(distance <= owner_->size_ - index);
        const auto [node, key_index] = owner_->nth_locate(index + distance);
        *this = iterator{owner_, node, key_index};
      }
      return *this;
    }

    friend iterator operator+(iterator it, std::ptrdiff_t offset) {
      return it += offset;
    }

    friend iterator operator+(std::ptrdiff_t offset, iterator it) {
      return it += offset;
    }

    friend iterator operator-(iterator it, std::ptrdiff_t offset) {
      return it -= offset;
    }

    friend std::ptrdiff_t operator-(const iterator& lhs, const iterator& rhs) {
      assert(lhs.owner_ == rhs.owner_);
      assert(lhs.owner_ != nullptr);
      return static_cast<std::ptrdiff_t>(
                 lhs.owner_->index_of_position(lhs.node_, lhs.key_index_)) -
             static_cast<std::ptrdiff_t>(
                 rhs.owner_->index_of_position(rhs.node_, rhs.key_index_));
    }

    friend bool operator==(const iterator&, const iterator&) = default;

   private:
    const Btree_Set* owner_ = nullptr;
    Node* node_ = nullptr;
    std::size_t key_index_ = 0;

    iterator(const Btree_Set* owner, Node* node, std::size_t key_index)
        : owner_(owner), node_(node), key_index_(key_index) {}

    friend class Btree_Set;
    friend class const_iterator;
  };

  class const_iterator {
   public:
    const_iterator() = default;
    const_iterator(const iterator& other)
        : owner_(other.owner_),
          node_(other.node_),
          key_index_(other.key_index_) {}

    const Key& operator*() const {
      assert(node_ != nullptr);
      return node_->keys[key_index_];
    }

    const Key* operator->() const {
      return &operator*();
    }

    const_iterator& operator++() {
      assert(node_ != nullptr);
      if (!node_->is_leaf) {
        node_ = node_->children[key_index_ + 1];
        while (!node_->is_leaf) node_ = node_->children[0];
        key_index_ = 0;
      } else if (key_index_ + 1 < node_->key_count) {
        ++key_index_;
      } else {
        while (node_->parent != nullptr &&
               node_->position == node_->parent->key_count) {
          node_ = node_->parent;
        }
        if (node_->parent == nullptr) {
          node_ = nullptr;
          key_index_ = 0;
        } else {
          key_index_ = node_->position;
          node_ = node_->parent;
        }
      }
      return *this;
    }

    const_iterator operator++(signed) {
      const_iterator result = *this;
      ++*this;
      return result;
    }

    const_iterator& operator--() {
      assert(owner_ != nullptr);
      if (node_ == nullptr) {
        node_ = owner_->root_;
        assert(node_ != nullptr);
        while (!node_->is_leaf) node_ = node_->children[node_->key_count];
        key_index_ = node_->key_count - 1;
      } else if (!node_->is_leaf) {
        node_ = node_->children[key_index_];
        while (!node_->is_leaf) node_ = node_->children[node_->key_count];
        key_index_ = node_->key_count - 1;
      } else if (key_index_ > 0) {
        --key_index_;
      } else {
        while (node_->parent != nullptr && node_->position == 0) {
          node_ = node_->parent;
        }
        assert(node_->parent != nullptr);
        key_index_ = node_->position - 1;
        node_ = node_->parent;
      }
      return *this;
    }

    const_iterator operator--(signed) {
      const_iterator result = *this;
      --*this;
      return result;
    }

    const_iterator& operator+=(std::ptrdiff_t offset) {
      assert(owner_ != nullptr);
      const std::size_t index = owner_->index_of_position(node_, key_index_);
      if (offset >= 0) {
        assert(static_cast<std::size_t>(offset) <= owner_->size_ - index);
        *this = owner_->nth(index + static_cast<std::size_t>(offset));
      } else {
        const std::size_t distance =
            static_cast<std::size_t>(-(offset + 1)) + 1;
        assert(distance <= index);
        *this = owner_->nth(index - distance);
      }
      return *this;
    }

    const_iterator& operator-=(std::ptrdiff_t offset) {
      assert(owner_ != nullptr);
      const std::size_t index = owner_->index_of_position(node_, key_index_);
      if (offset >= 0) {
        assert(static_cast<std::size_t>(offset) <= index);
        *this = owner_->nth(index - static_cast<std::size_t>(offset));
      } else {
        const std::size_t distance =
            static_cast<std::size_t>(-(offset + 1)) + 1;
        assert(distance <= owner_->size_ - index);
        *this = owner_->nth(index + distance);
      }
      return *this;
    }

    friend const_iterator operator+(const_iterator it, std::ptrdiff_t offset) {
      return it += offset;
    }

    friend const_iterator operator+(std::ptrdiff_t offset, const_iterator it) {
      return it += offset;
    }

    friend const_iterator operator-(const_iterator it, std::ptrdiff_t offset) {
      return it -= offset;
    }

    friend std::ptrdiff_t operator-(const const_iterator& lhs,
                                    const const_iterator& rhs) {
      assert(lhs.owner_ == rhs.owner_);
      assert(lhs.owner_ != nullptr);
      return static_cast<std::ptrdiff_t>(
                 lhs.owner_->index_of_position(lhs.node_, lhs.key_index_)) -
             static_cast<std::ptrdiff_t>(
                 rhs.owner_->index_of_position(rhs.node_, rhs.key_index_));
    }

    friend bool operator==(const const_iterator&,
                           const const_iterator&) = default;

   private:
    const Btree_Set* owner_ = nullptr;
    const Node* node_ = nullptr;
    std::size_t key_index_ = 0;

    const_iterator(const Btree_Set* owner, const Node* node,
                   std::size_t key_index)
        : owner_(owner), node_(node), key_index_(key_index) {}

    friend class Btree_Set;
  };

  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;

  Btree_Set() = default;
  explicit Btree_Set(Compare compare) : compare_(std::move(compare)) {}
  Btree_Set(const Btree_Set&) = delete;
  Btree_Set& operator=(const Btree_Set&) = delete;
  ~Btree_Set() {
    clear();
  }

  bool empty() const {
    return size_ == 0;
  }
  size_type size() const {
    return size_;
  }
  void clear() {
    delete_subtree(root_);
    root_ = nullptr;
    size_ = 0;
  }

  std::pair<iterator, bool> insert(const Key& key) {
    if (root_ == nullptr) {
      root_ = new Node(key);
      ++size_;
      return {iterator{this, root_, 0}, true};
    }

    InsertResult result = insert_recursive(root_, key);
    if (!result.inserted) {
      const auto [node, key_index] = locate(key);
      return {iterator{this, node, key_index}, false};
    }

    ++size_;
    if (result.split) {
      Node* new_root = new Node();
      new_root->is_leaf = false;
      new_root->children[0] = root_;
      root_->parent = new_root;
      root_->position = 0;
      new_root->insert_non_full(result.split->separator, result.split->right,
                                0);
      new_root->recalculate_subtree_size();
      root_ = new_root;
    }

    const auto [node, key_index] = locate(key);
    return {iterator{this, node, key_index}, true};
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return insert(Key(std::forward<Args>(args)...));
  }

  iterator erase(const_iterator pos) {
    assert(pos.owner_ == this);
    assert(pos.node_ != nullptr);
    const std::size_t next_index = index_of_node(pos.node_, pos.key_index_);
    const Key key = pos.node_->keys[pos.key_index_];
    assert(erase(key) == 1);
    return nth(next_index);
  }
  size_type erase(const Key& key) {
    if (root_ == nullptr || !erase_recursive(root_, key)) {
      return 0;
    }
    --size_;

    if (root_->key_count == 0) {
      if (root_->is_leaf) {
        delete root_;
        root_ = nullptr;
      } else {
        Node* new_root = root_->children[0];
        new_root->parent = nullptr;
        new_root->position = 0;
        root_->children[0] = nullptr;
        delete root_;
        root_ = new_root;
      }
    }
    return 1;
  }

  iterator find(const Key& key) {
    const auto [node, key_index] = locate(key);
    return iterator{this, node, key_index};
  }

  const_iterator find(const Key& key) const {
    const auto [node, key_index] = locate(key);
    return const_iterator{this, node, key_index};
  }

  bool contains(const Key& key) const {
    return locate(key).first != nullptr;
  }

  iterator lower_bound(const Key& key) {
    const auto [node, key_index] = lower_bound_locate(key);
    return iterator{this, node, key_index};
  }

  const_iterator lower_bound(const Key& key) const {
    const auto [node, key_index] = lower_bound_locate(key);
    return const_iterator{this, node, key_index};
  }

  iterator upper_bound(const Key& key) {
    const auto [node, key_index] = upper_bound_locate(key);
    return iterator{this, node, key_index};
  }

  const_iterator upper_bound(const Key& key) const {
    const auto [node, key_index] = upper_bound_locate(key);
    return const_iterator{this, node, key_index};
  }

  std::pair<iterator, iterator> equal_range(const Key& key) {
    return {lower_bound(key), upper_bound(key)};
  }

  std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
    return {lower_bound(key), upper_bound(key)};
  }

  iterator begin() {
    if (root_ == nullptr) {
      return end();
    }
    Node* node = root_;
    while (!node->is_leaf) {
      node = node->children[0];
    }
    return iterator{this, node, 0};
  }

  iterator end() {
    return iterator{this, nullptr, 0};
  }

  const_iterator begin() const {
    return cbegin();
  }
  const_iterator end() const {
    return cend();
  }

  const_iterator cbegin() const {
    if (root_ == nullptr) return cend();
    const Node* node = root_;
    while (!node->is_leaf) node = node->children[0];
    return const_iterator{this, node, 0};
  }

  const_iterator cend() const {
    return const_iterator{this, nullptr, 0};
  }

  iterator nth(size_type index) {
    const auto [node, key_index] = nth_locate(index);
    return iterator{this, node, key_index};
  }

  const_iterator nth(size_type index) const {
    const auto [node, key_index] = nth_locate(index);
    return const_iterator{this, node, key_index};
  }

  size_type order_of_key(const Key& key) const {
    Node* node = root_;
    size_type result = 0;
    while (node != nullptr) {
      const std::size_t key_index = lower_bound_index(node, key);
      for (std::size_t i = 0; i < key_index; ++i) {
        if (!node->is_leaf) {
          result += node->children[i]->subtree_size;
        }
        ++result;
      }
      if (key_index < node->key_count &&
          equivalent(node->keys[key_index], key)) {
        if (!node->is_leaf) {
          result += node->children[key_index]->subtree_size;
        }
        return result;
      }
      if (node->is_leaf) {
        return result;
      }
      node = node->children[key_index];
    }
    return result;
  }

  size_type index_of(const_iterator it) const {
    assert(it.owner_ == this);
    return index_of_node(it.node_, it.key_index_);
  }
};
