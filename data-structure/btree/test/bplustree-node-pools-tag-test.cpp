#include <bits/stdc++.h>

#include "data-structure/btree/components/bplustree-node-pools.hpp"

using namespace std;

namespace {
struct MockLeaf {
  long long value = 0;
  explicit MockLeaf(long long v) : value(v) {}
};

struct MockBottom {
  std::size_t size = 0;
  std::uint8_t count = 0;
  std::array<std::uint32_t, 4> children{};
};

struct MockUpper {
  std::size_t size = 0;
  std::uint8_t count = 0;
  std::uint8_t level = 0;
  std::array<std::uint32_t, 4> children{};
  explicit MockUpper(std::uint8_t lvl) : level(lvl) {}
};

struct MockTraits {
  using Leaf = MockLeaf;
  using BottomNode = MockBottom;
  using UpperNode = MockUpper;
};
}  // namespace

/** @brief タグ付き ID が独立 Pool で重複しても区別できることを検証する。 */
signed main() {
  using Pools = Bplustree::NodePools<MockTraits>;
  const auto leaf = Pools::make_leaf(42);
  const auto bottom = Pools::make_bottom();
  const auto upper = Pools::make_upper(1);

  assert(Pools::is_bottom(bottom));
  assert(!Pools::is_bottom(upper));
  assert(bottom != upper);
  assert(Pools::get<1>(bottom).count == 0);
  assert(Pools::get<2>(upper).level == 1);
  assert(Pools::get<Pools::NodeType::Leaf>(leaf).value == 42);

  Pools::get<1>(bottom).children[0] = leaf;
  Pools::get<1>(bottom).count = 1;
  Pools::get<1>(bottom).size = 1;
  Pools::get<2>(upper).children[0] = bottom;
  Pools::get<2>(upper).count = 1;
  Pools::get<2>(upper).size = 1;

  bool hit_bottom = false, hit_upper = false;
  Pools::with_internal(bottom, [&](auto& node) {
    hit_bottom = is_same_v<remove_cvref_t<decltype(node)>, MockBottom>;
  });
  Pools::with_internal(upper, [&](auto& node) {
    hit_upper = is_same_v<remove_cvref_t<decltype(node)>, MockUpper>;
  });
  assert(hit_bottom && hit_upper);
  assert(Pools::id(Pools::get<1>(bottom)) == bottom);
  assert(Pools::id(Pools::get<2>(upper)) == upper);

  Pools::recycle(upper);
  assert(Pools::leaves().size() == 0);
  assert(Pools::bottoms().size() == 0);
  assert(Pools::uppers().size() == 0);

  cerr << "bplustree-node-pools-tag-test passed!" << endl;
}
