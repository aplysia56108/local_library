#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <utility>

// B+ tree の土台となる、順序を持つ列構造。
// 実データは leaf にのみ保持し、internal node は探索用の separator と
// 子ポインタだけを持つ。公開 API は Node 操作を使って順に実装する。
template <class Key, class Compare = std::less<Key>,
          std::size_t NodeCapMax = 63>
class Btree_Vector {
 private:
  static_assert(NodeCapMax >= 3);
  static_assert(std::default_initializable<Key>);

  // leaf の最大キー数、および internal node の最大 separator 数である。
  static constexpr std::size_t kMinKeys = (NodeCapMax - 1) / 2;
  static constexpr std::size_t kMaxKeys = NodeCapMax;
  static constexpr std::size_t kMaxChildren = NodeCapMax + 1;

  enum class Node_Kind { kLeaf, kInternal };

  struct Node {
    Node* parent = nullptr;
    std::size_t position = 0;
    std::size_t subtree_size = 0;
    Node_Kind kind;

    explicit Node(Node_Kind kind) : kind(kind) {}
    bool is_leaf() const { return kind == Node_Kind::kLeaf; }
  };

  struct Leaf_Node final : Node {
    // keys[0, key_count) が昇順の実データである。
    std::array<Key, kMaxKeys> keys{};
    std::size_t key_count = 0;
    Leaf_Node* prev = nullptr;
    Leaf_Node* next = nullptr;

    Leaf_Node() : Node(Node_Kind::kLeaf) {}

    explicit Leaf_Node(Key key) : Node(Node_Kind::kLeaf), key_count(1) {
      keys[0] = std::move(key);
      this->subtree_size = 1;
    }

    void insert_non_full(const Key& key, std::size_t key_index) {
      assert(key_index <= key_count);
      assert(key_count < kMaxKeys);
      for (std::size_t i = key_count; i > key_index; --i) {
        keys[i] = std::move(keys[i - 1]);
      }
      keys[key_index] = key;
      ++key_count;
      ++this->subtree_size;
    }

    // 満杯 leaf への挿入と分割をまとめて行う。B+ tree では separator を
    // 親へコピーするため、中央値に対応する実データは leaf から消えない。
    Leaf_Node* split_and_insert(const Key& key, std::size_t key_index) {
      assert(key_count == kMaxKeys);
      assert(key_index <= key_count);

      std::array<Key, kMaxKeys + 1> all_keys{};
      for (std::size_t i = 0; i < key_count; ++i) {
        all_keys[i] = std::move(keys[i]);
      }
      for (std::size_t i = key_count; i > key_index; --i) {
        all_keys[i] = std::move(all_keys[i - 1]);
      }
      all_keys[key_index] = key;

      const std::size_t left_count = (kMaxKeys + 1) / 2;
      const std::size_t right_count = kMaxKeys + 1 - left_count;
      Leaf_Node* right = new Leaf_Node();
      right->parent = this->parent;
      right->position = this->position + 1;
      right->key_count = right_count;
      right->subtree_size = right_count;

      for (std::size_t i = 0; i < left_count; ++i) {
        keys[i] = std::move(all_keys[i]);
      }
      for (std::size_t i = 0; i < right_count; ++i) {
        right->keys[i] = std::move(all_keys[left_count + i]);
      }
      key_count = left_count;
      this->subtree_size = left_count;

      right->next = next;
      if (next != nullptr) next->prev = right;
      right->prev = this;
      next = right;
      return right;
    }

    void erase(std::size_t key_index) {
      assert(key_index < key_count);
      for (std::size_t i = key_index + 1; i < key_count; ++i) {
        keys[i - 1] = std::move(keys[i]);
      }
      --key_count;
      --this->subtree_size;
    }
  };

  struct Internal_Node final : Node {
    // separators[i] == minimum_key(children[i + 1]) である。
    std::array<Key, kMaxKeys> separators{};
    std::array<Node*, kMaxChildren> children{};
    std::size_t child_count = 0;

    Internal_Node() : Node(Node_Kind::kInternal) {}

    void sync_metadata() {
      assert(child_count > 0);
      this->subtree_size = 0;
      for (std::size_t i = 0; i < child_count; ++i) {
        assert(children[i] != nullptr);
        children[i]->parent = this;
        children[i]->position = i;
        this->subtree_size += children[i]->subtree_size;
        if (i > 0) separators[i - 1] = minimum_key_of(children[i]);
      }
    }

    void insert_child_non_full(Node* child, std::size_t child_index) {
      assert(child != nullptr);
      assert(child_index <= child_count);
      assert(child_count < kMaxChildren);
      for (std::size_t i = child_count; i > child_index; --i) {
        children[i] = children[i - 1];
      }
      children[child_index] = child;
      ++child_count;
      sync_metadata();
    }

    // 満杯 internal node に child を入れてから分割する。separator は child
    // から再構築できるため、親へ move する Key は存在しない。
    Internal_Node* split_and_insert(Node* child, std::size_t child_index) {
      assert(child != nullptr);
      assert(child_count == kMaxChildren);
      assert(child_index <= child_count);

      std::array<Node*, kMaxChildren + 1> all_children{};
      for (std::size_t i = 0; i < child_count; ++i) {
        all_children[i] = children[i];
      }
      for (std::size_t i = child_count; i > child_index; --i) {
        all_children[i] = all_children[i - 1];
      }
      all_children[child_index] = child;

      const std::size_t total_children = kMaxChildren + 1;
      const std::size_t left_count = total_children / 2;
      const std::size_t right_count = total_children - left_count;
      Internal_Node* right = new Internal_Node();
      right->parent = this->parent;
      right->position = this->position + 1;
      right->child_count = right_count;

      for (std::size_t i = 0; i < left_count; ++i) {
        children[i] = all_children[i];
      }
      for (std::size_t i = left_count; i < kMaxChildren; ++i) {
        children[i] = nullptr;
      }
      child_count = left_count;
      for (std::size_t i = 0; i < right_count; ++i) {
        right->children[i] = all_children[left_count + i];
      }

      sync_metadata();
      right->sync_metadata();
      return right;
    }

   private:
    static const Key& minimum_key_of(const Node* node) {
      assert(node != nullptr);
      while (!node->is_leaf()) {
        const auto* internal = static_cast<const Internal_Node*>(node);
        assert(internal->child_count > 0);
        node = internal->children[0];
      }
      const auto* leaf = static_cast<const Leaf_Node*>(node);
      assert(leaf->key_count > 0);
      return leaf->keys[0];
    }
  };

  struct Header {
    Node* root = nullptr;
    Leaf_Node* first = nullptr;
    Leaf_Node* last = nullptr;
  } header_;

  static void delete_subtree(Node* node) {
    if (node == nullptr) return;
    if (node->is_leaf()) {
      delete static_cast<Leaf_Node*>(node);
      return;
    }
    auto* internal = static_cast<Internal_Node*>(node);
    for (std::size_t i = 0; i < internal->child_count; ++i) {
      delete_subtree(internal->children[i]);
    }
    delete internal;
  }

 public:
  Btree_Vector() = default;
  Btree_Vector(const Btree_Vector&) = delete;
  Btree_Vector& operator=(const Btree_Vector&) = delete;
  ~Btree_Vector() { clear(); }

  void clear() {
    delete_subtree(header_.root);
    header_ = {};
  }
};
