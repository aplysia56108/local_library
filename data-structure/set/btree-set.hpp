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
          std::size_t MinDegree = 32>
class Btree_Set {
 private:
  static_assert(MinDegree >= 2);
  static_assert(std::default_initializable<Key>);

  // 非根ノードは kMinKeys 以上 kMaxKeys 以下のキーを持つ。
  static constexpr std::size_t kMinKeys = MinDegree - 1;
  static constexpr std::size_t kMaxKeys = 2 * MinDegree - 1;
  static constexpr std::size_t kMaxChildren = 2 * MinDegree;

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

      // 2 * MinDegree 個のキーから、左に MinDegree - 1 個、右に
      // MinDegree 個を残し、境界の 1 個を親へ昇格させる。
      const std::size_t median_index = MinDegree - 1;
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

    void erase(const std::size_t i) {};

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
    if (key_index < node->key_count &&
        equivalent(node->keys[key_index], key)) {
      return {false, std::nullopt};
    }

    if (node->is_leaf) {
      if (node->key_count < kMaxKeys) {
        node->insert_non_full(key, nullptr, key_index);
        return {true, std::nullopt};
      }
      return {true, node->split_and_insert(key, nullptr, key_index)};
    }

    InsertResult child_result = insert_recursive(node->children[key_index], key);
    if (!child_result.inserted) {
      return {false, std::nullopt};
    }
    if (!child_result.split) {
      ++node->subtree_size;
      return {true, std::nullopt};
    }

    const auto& child_split = *child_result.split;
    if (node->key_count < kMaxKeys) {
      node->insert_non_full(child_split.separator, child_split.right, key_index);
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

  Node* root_ = nullptr;
  std::size_t size_ = 0;
  Compare compare_{};

 public:
  class iterator {
   public:
    iterator() = default;

   private:
    Node* node_ = nullptr;
    std::size_t key_index_ = 0;

    iterator(Node* node, std::size_t key_index)
        : node_(node), key_index_(key_index) {}

    friend class Btree_Set;
  };

  class const_iterator {
   public:
    const_iterator() = default;

   private:
    const Node* node_ = nullptr;
    std::size_t key_index_ = 0;

    const_iterator(const Node* node, std::size_t key_index)
        : node_(node), key_index_(key_index) {}

    friend class Btree_Set;
  };

  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;

  Btree_Set() = default;
  explicit Btree_Set(Compare compare) : compare_(std::move(compare)) {}
  Btree_Set(const Btree_Set&) = delete;
  Btree_Set& operator=(const Btree_Set&) = delete;
  ~Btree_Set() { clear(); }

  bool empty() const { return size_ == 0; }
  size_type size() const { return size_; }
  void clear() {
    delete_subtree(root_);
    root_ = nullptr;
    size_ = 0;
  }

  std::pair<iterator, bool> insert(const Key& key) {
    if (root_ == nullptr) {
      root_ = new Node(key);
      ++size_;
      return {iterator{root_, 0}, true};
    }

    InsertResult result = insert_recursive(root_, key);
    if (!result.inserted) {
      const auto [node, key_index] = locate(key);
      return {iterator{node, key_index}, false};
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
    return {iterator{node, key_index}, true};
  }
  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args);

  iterator erase(const_iterator pos);
  size_type erase(const Key& key);

  iterator find(const Key& key);
  bool contains(const Key& key) const;
  iterator lower_bound(const Key& key);
  iterator upper_bound(const Key& key);
  std::pair<iterator, iterator> equal_range(const Key& key);

  iterator begin();
  iterator end();
  iterator nth(size_type index);  // index == size() なら end()
  size_type order_of_key(const Key& key) const;
  size_type index_of(const_iterator it) const;
};
