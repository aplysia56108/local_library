// Include the iterator first to check that it provides its own dependencies.
#include "data-structure/btree/components/bplustree-iterator.hpp"
#include "data-structure/btree/components/bplustree-node.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <numeric>
#include <vector>

using Traits = Bplustree::Traits<int, 4>;
using Node = Traits::Node;
using Leaf = Traits::Leaf;
using Internal = Traits::Internal;
using It = Iterator<Node, Leaf, Internal, int, false>;
using ConstIt = Iterator<Node, Leaf, Internal, int, true>;
static_assert(std::random_access_iterator<It>);
static_assert(std::random_access_iterator<ConstIt>);
static_assert(std::convertible_to<It, ConstIt>);
static_assert(!std::convertible_to<ConstIt, It>);
static_assert(std::same_as<std::iter_reference_t<ConstIt>, const int&>);

void test_tree(int size) {
  std::vector<std::unique_ptr<Node>> level;
  for (int i = 0; i < size || level.empty();) {
    auto leaf = std::make_unique<Leaf>();
    for (int j = 0; j < 3 && i < size; ++j) leaf->insert(i++, j);
    level.push_back(std::move(leaf));
  }
  while (level.size() > 1) {
    std::vector<std::unique_ptr<Node>> next;
    for (std::size_t i = 0; i < level.size();) {
      auto parent = std::make_unique<Internal>();
      for (int j = 0; j < 3 && i < level.size(); ++j)
        parent->insert(std::move(level[i++]), j);
      next.push_back(std::move(parent));
    }
    level = std::move(next);
  }
  auto& root = level[0];
  auto [first, first_index] = root->locate(0);
  auto [last, last_index] = root->locate(size);
  It begin(first, first_index), end(last, last_index), before(first, -1);
  assert(end - begin == size && begin - end == -size);
  assert(begin - before == 1 && before - begin == -1);
  assert(before + 1 == begin && end - size == begin);
  assert(begin + size == end);
  assert(before < begin && begin <= end);
  ConstIt cbegin = begin, cend = end;
  assert(cbegin == begin && begin == cbegin);
  assert(cend - begin == size && end - cbegin == size);
  assert(cbegin <= end && begin <= cend);

  for (int i = -1; i <= size; ++i) {
    It pos = begin + i;
    assert(pos - begin == i && pos - before == i + 1);
    if (i >= 0 && i < size) {
      assert(*pos == i && begin[i] == i);
      ConstIt cpos = pos;
      assert(*cpos == i && cpos.operator->() == pos.operator->());
      auto old = pos++;
      assert(old == begin + i && pos == begin + i + 1);
      old = pos--;
      assert(old == begin + i + 1 && pos == begin + i);
    }
    for (int j = -1; j <= size; ++j) {
      auto other = begin + j;
      assert(pos - other == i - j);
      assert((pos < other) == (i < j));
      auto moved = pos;
      moved += j - i;
      assert(moved == other);
      moved -= j - i;
      assert(moved == pos);
    }
  }
  std::vector<int> expected(size);
  std::iota(expected.begin(), expected.end(), 0);
  assert(std::vector<int>(begin, end) == expected);
  std::reverse(begin, end);
  std::reverse(expected.begin(), expected.end());
  assert(std::equal(begin, end, expected.begin(), expected.end()));
  std::sort(begin, end);
  std::iota(expected.begin(), expected.end(), 0);
  assert(std::equal(cbegin, cend, expected.begin(), expected.end()));
}

int main() {
  assert(It{} == It{} && ConstIt{} == ConstIt{});
  for (int size : {0, 1, 2, 3, 4, 9, 10, 27, 28, 80}) test_tree(size);
  assert(Node::live_nodes == 0);
}
