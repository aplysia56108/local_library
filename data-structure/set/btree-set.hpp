#pragma once

// i番目がO(log)で取れるsetの構想
// iteratorのdiffや(+n)がlogで取れるようにする

// まずBtreeで挿入可能な列の構造を作り、それを利用してset,
// multisetなどを実装する

// パフォーマンスが高いとうれしい。
// まずはインターフェースを書いてほしい
#include <array>
#include <cassert>
#include <cstddef>
#include <concepts>
#include <functional>
#include <utility>

template <class Key, class Compare = std::less<Key>,
          std::size_t MinDegree = 32>
class Btree_Set {
 private:
  static_assert(MinDegree >= 2);
  static_assert(std::default_initializable<Key>);

  // 非根ノードは MinDegree - 1 個以上のキーを持つ。
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
      recalculate_subtree_size();
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

 public:
  class iterator {
   private:
    Node* node_ = nullptr;
    std::size_t key_index_ = 0;

    friend class Btree_Set;
  };

  class const_iterator {
   private:
    const Node* node_ = nullptr;
    std::size_t key_index_ = 0;

    friend class Btree_Set;
  };

  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;

  Btree_Set() = default;
  bool empty() const;
  size_type size() const;
  void clear();

  std::pair<iterator, bool> insert(const Key& key);
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
