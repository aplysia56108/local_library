#include <iostream>
#include "data-structure/btree/btree-vector.hpp"

using Btree = Btree_Base<int, std::less<int>, 4>;
using Leaf = Btree::Leaf;
using Internal = Btree::Internal;
using Node = Btree::Node;

int kMaxKeys = Btree::kNodeCapacity;

static void test_node() {
  // ============================================================
  // Leaf: constructor
  // ============================================================
  {
    Leaf leaf;

    assert(leaf.is_leaf());
    assert(leaf.parent == nullptr);
    assert(leaf.size == 0);
    assert(leaf.count == 0);
    assert(leaf.min_key == nullptr);
    assert(leaf.prev == nullptr);
    assert(leaf.next == nullptr);
  }

  {
    Leaf leaf(10);

    assert(leaf.is_leaf());
    assert(leaf.parent == nullptr);
    assert(leaf.size == 1);
    assert(leaf.count == 1);
    assert(leaf.keys[0] == 10);
    assert(leaf.min_key == &leaf.keys[0]);
    assert(*leaf.min_key == 10);
    assert(leaf.prev == nullptr);
    assert(leaf.next == nullptr);
  }

  // ============================================================
  // Leaf: insert
  // ============================================================
  {
    Leaf leaf;

    leaf.insert(20, 0);
    assert(leaf.count == 1);
    assert(leaf.size == 1);
    assert(leaf.keys[0] == 20);
    assert(*leaf.min_key == 20);

    // 先頭
    leaf.insert(10, 0);
    assert(leaf.count == 2);
    assert(leaf.size == 2);
    assert(leaf.keys[0] == 10);
    assert(leaf.keys[1] == 20);
    assert(*leaf.min_key == 10);

    // 末尾
    leaf.insert(40, 2);
    assert(leaf.count == 3);
    assert(leaf.keys[0] == 10);
    assert(leaf.keys[1] == 20);
    assert(leaf.keys[2] == 40);
    assert(*leaf.min_key == 10);

    // 中央
    leaf.insert(30, 2);
    assert(leaf.count == 4);
    assert(leaf.size == 4);
    assert(leaf.keys[0] == 10);
    assert(leaf.keys[1] == 20);
    assert(leaf.keys[2] == 30);
    assert(leaf.keys[3] == 40);
    assert(*leaf.min_key == 10);
  }

  // ============================================================
  // Leaf: split
  // ============================================================
  {
    Leaf left;
    left.insert(10, 0);
    left.insert(20, 1);
    left.insert(30, 2);
    left.insert(40, 3);

    std::unique_ptr<Leaf> right(left.split());

    assert(left.count == 2);
    assert(left.size == 2);
    assert(left.keys[0] == 10);
    assert(left.keys[1] == 20);
    assert(*left.min_key == 10);

    assert(right->count == 2);
    assert(right->size == 2);
    assert(right->keys[0] == 30);
    assert(right->keys[1] == 40);
    assert(*right->min_key == 30);

    assert(left.next == right.get());
    assert(right->prev == &left);
    assert(right->next == nullptr);
  }

  // ============================================================
  // Leaf: split with existing next
  // ============================================================
  {
    Leaf left;
    Leaf old_next(100);

    left.next = &old_next;
    old_next.prev = &left;

    left.insert(10, 0);
    left.insert(20, 1);
    left.insert(30, 2);
    left.insert(40, 3);

    std::unique_ptr<Leaf> right(left.split());

    assert(left.next == right.get());
    assert(right->prev == &left);

    assert(right->next == &old_next);
    assert(old_next.prev == right.get());
  }

  // ============================================================
  // Leaf: erase
  // ============================================================
  {
    Leaf leaf;
    leaf.insert(10, 0);
    leaf.insert(20, 1);
    leaf.insert(30, 2);
    // leaf.insert(40, 3);

    // 中央
    bool empty = leaf.erase(1);

    assert(!empty);
    assert(leaf.count == 2);
    assert(leaf.size == 2);
    assert(leaf.keys[0] == 10);
    assert(leaf.keys[1] == 30);
    // assert(leaf.keys[2] == 40);
    assert(*leaf.min_key == 10);

    leaf.insert(20, 1);
    // 末尾
    empty = leaf.erase(2);

    assert(!empty);
    assert(leaf.count == 2);
    assert(leaf.size == 2);
    assert(leaf.keys[0] == 10);
    assert(leaf.keys[1] == 20);
    assert(*leaf.min_key == 10);
    leaf.insert(30, 2);

    // 先頭
    empty = leaf.erase(0);

    assert(!empty);
    assert(leaf.count == 2);
    assert(leaf.size == 2);
    assert(leaf.keys[0] == 20);
    assert(leaf.keys[1] == 30);
    assert(*leaf.min_key == 20);
  }

  // ============================================================
  // Leaf: erase last element and detach
  // ============================================================
  {
    Leaf prev(10);
    Leaf leaf(20);
    Leaf next(30);

    prev.next = &leaf;
    leaf.prev = &prev;
    leaf.next = &next;
    next.prev = &leaf;

    bool empty = leaf.erase(0);

    assert(empty);
    assert(leaf.count == 0);
    assert(leaf.size == 0);
    assert(leaf.min_key == nullptr);

    assert(prev.next == &next);
    assert(next.prev == &prev);
  }

  // ============================================================
  // Internal: empty
  // ============================================================
  {
    Internal node;

    assert(!node.is_leaf());
    assert(node.parent == nullptr);
    assert(node.count == 0);
    assert(node.size == 0);
    assert(node.min_key == nullptr);

    for (std::size_t i = 0; i < kMaxKeys; ++i) {
      assert(node.children[i] == nullptr);
    }
  }

  // ============================================================
  // Internal: insert
  // ============================================================
  {
    Internal node;

    auto a = std::make_unique<Leaf>(10);
    auto b = std::make_unique<Leaf>(20);
    auto c = std::make_unique<Leaf>(30);

    Leaf* a_ptr = a.get();
    Leaf* b_ptr = b.get();
    Leaf* c_ptr = c.get();

    node.insert(std::move(a), 0);
    node.insert(std::move(c), 1);

    // 中央へ挿入
    node.insert(std::move(b), 1);

    assert(node.count == 3);
    assert(node.size == 3);

    assert(node.children[0].get() == a_ptr);
    assert(node.children[1].get() == b_ptr);
    assert(node.children[2].get() == c_ptr);

    assert(node.children[0]->parent == &node);
    assert(node.children[1]->parent == &node);
    assert(node.children[2]->parent == &node);

    assert(node.children[0]->position == 0);
    assert(node.children[1]->position == 1);
    assert(node.children[2]->position == 2);

    assert(node.min_key == a_ptr->min_key);
    assert(*node.min_key == 10);

    assert(node.children[3] == nullptr);
  }

  // ============================================================
  // Internal: size = subtree size sum
  // ============================================================
  {
    Internal node;

    auto a = std::make_unique<Leaf>();
    a->insert(1, 0);
    a->insert(2, 1);

    auto b = std::make_unique<Leaf>();
    b->insert(3, 0);
    b->insert(4, 1);
    b->insert(5, 2);

    node.insert(std::move(b), 0);
    node.insert(std::move(a), 0);

    assert(node.count == 2);
    assert(node.size == 5);
    assert(*node.min_key == 1);
  }

  // ============================================================
  // Internal: split
  // ============================================================
  {
    Internal left;

    left.insert(std::make_unique<Leaf>(10), 0);
    left.insert(std::make_unique<Leaf>(20), 1);
    left.insert(std::make_unique<Leaf>(30), 2);
    left.insert(std::make_unique<Leaf>(40), 3);

    std::unique_ptr<Internal> right(left.split());

    assert(left.count == 2);
    assert(left.size == 2);
    assert(*left.min_key == 10);

    assert(right->count == 2);
    assert(right->size == 2);
    assert(*right->min_key == 30);

    assert(*left.children[0]->min_key == 10);
    assert(*left.children[1]->min_key == 20);

    assert(*right->children[0]->min_key == 30);
    assert(*right->children[1]->min_key == 40);

    for (std::size_t i = 0; i < left.count; ++i) {
      assert(left.children[i]->parent == &left);
      assert(left.children[i]->position == i);
    }

    for (std::size_t i = 0; i < right->count; ++i) {
      assert(right->children[i]->parent == right.get());
      assert(right->children[i]->position == i);
    }

    // split 元の後半は move 済み
    assert(left.children[2] == nullptr);
    assert(left.children[3] == nullptr);
  }

  // ============================================================
  // Internal: erase middle
  // ============================================================
  {
    Internal node;

    node.insert(std::make_unique<Leaf>(10), 0);
    node.insert(std::make_unique<Leaf>(20), 1);
    node.insert(std::make_unique<Leaf>(30), 2);

    bool empty = node.erase(1);

    assert(!empty);
    assert(node.count == 2);
    assert(node.size == 2);

    assert(*node.children[0]->min_key == 10);
    assert(*node.children[1]->min_key == 30);

    assert(node.children[0]->position == 0);
    assert(node.children[1]->position == 1);

    assert(node.children[2] == nullptr);
    assert(*node.min_key == 10);
  }

  // ============================================================
  // Internal: erase last
  // ============================================================
  {
    Internal node;

    node.insert(std::make_unique<Leaf>(10), 0);
    node.insert(std::make_unique<Leaf>(20), 1);

    bool empty = node.erase(1);

    assert(!empty);
    assert(node.count == 1);
    assert(node.size == 1);
    assert(*node.children[0]->min_key == 10);
    assert(node.children[1] == nullptr);
    assert(*node.min_key == 10);

    empty = node.erase(0);

    assert(empty);
    assert(node.count == 0);
    assert(node.size == 0);
    assert(node.children[0] == nullptr);
    assert(node.min_key == nullptr);
  }

  std::cout << "Node tests passed.\n";
}

int main() {
  test_node();
}
