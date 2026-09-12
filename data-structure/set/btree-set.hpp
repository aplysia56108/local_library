#pragma once

// i番目がO(log)で取れるsetの構想
// iteratorのdiffや(+n)がlogで取れるようにする

// まずBtreeで挿入可能な列の構造を作り、それを利用してset,
// multisetなどを実装する

// パフォーマンスが高いとうれしい。
// まずはインターフェースを書いてほしい
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

template <class Key, class Compare = std::less<Key>,
          std::size_t NodeCapacity = 64>
class Btree_Set {
 private:
  struct Node {
    std::vector<Key> keys;
    std::vector<Node*> children;
    std::vector<std::size_t> child_sizes;
    Node* parent;
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
