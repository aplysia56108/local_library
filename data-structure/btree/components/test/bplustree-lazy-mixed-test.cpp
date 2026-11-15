#include <bits/stdc++.h>
#include <atcoder/modint>

#include "data-structure/btree/bplustree-lazysegtree.hpp"

/**
 * @brief mixed_edges の先頭 5,000 操作を再現し、lazy と構造変更の交互処理を検証。
 * @details 固定seed・独立した std::vector を使う。
 * ### 計算量
 * 参照実装の O(NQ) を許す短い機能テスト。性能計測は行わない。
 */
namespace {
using ll = long long;
using mint = atcoder::modint998244353;
constexpr ll N = 100'000;
constexpr ll Q = 5'000;
constexpr std::string_view pattern = "IIIIIEEEDRRSGGQQQAAA";

struct Rng {
  std::uint32_t state = 0x9e3779b9u;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
};

ll position(Rng& rng, ll count, ll& anchor) {
  assert(count > 0);
  const ll span = std::min(64LL, count);
  const bool left = rng.next() % 2 == 0;
  const ll offset = rng.next() % span;
  const ll p = left ? offset : count - 1 - offset;
  anchor = p;
  return p;
}

std::pair<ll, ll> make_range(Rng& rng, ll size, ll& anchor, ll step) {
  const ll length = step % 200 == 0
                        ? std::max(1LL, size / 4)
                        : 1 + rng.next() % std::min(32LL, size);
  const ll left = position(rng, size - length + 1, anchor);
  return {left, left + length};
}

template <std::size_t Capacity>
void mixed_edges_test() {
  std::vector<mint> reference;
  reference.reserve(N + Q);
  for (ll i = 0; i < N; ++i) reference.push_back((i * 809 + 17) % 10'009);
  AffineSumLazySegtree<mint, Capacity> tree(reference.begin(), reference.end());

  Rng rng;
  ll anchor = N / 2;
  ll next_id = N;
  std::string operations(pattern);

  for (ll block = 0; block < Q / static_cast<ll>(pattern.size()); ++block) {
    for (ll j = static_cast<ll>(operations.size()) - 1; j > 0; --j)
      std::swap(operations[j], operations[rng.next() % (j + 1)]);
    for (ll j = 0; j < static_cast<ll>(operations.size()); ++j) {
      const ll step = block * static_cast<ll>(operations.size()) + j;
      const char op = operations[j];

      if (op == 'I') {
        const ll pos = position(rng, reference.size() + 1, anchor);
        const mint value = (next_id++ * 809 + 17) % 10'009;
        reference.insert(reference.begin() + pos, value);
        tree.insert(pos, value);
      } else if (op == 'E') {
        const ll pos = position(rng, reference.size(), anchor);
        reference.erase(reference.begin() + pos);
        tree.erase(pos);
      } else if (op == 'D') {
        const ll length = 1 + rng.next() % std::min(3LL, static_cast<ll>(reference.size()));
        const ll left = position(rng, reference.size() - length + 1, anchor);
        reference.erase(reference.begin() + left, reference.begin() + left + length);
        tree.erase(left, left + length);
      } else if (op == 'R') {
        const auto [left, right] = make_range(rng, reference.size(), anchor, step);
        std::reverse(reference.begin() + left, reference.begin() + right);
        tree.reverse(left, right);
      } else if (op == 'S') {
        const ll pos = position(rng, reference.size() + 1, anchor);
        auto right = tree.split(pos);
        tree.merge(right);
      } else if (op == 'G') {
        const ll pos = position(rng, reference.size(), anchor);
        if (reference[pos] != tree[pos]) {
          std::cerr << "mixed_edges access mismatch: capacity=" << Capacity
                    << ", operation=" << step << ", position=" << pos << '\n';
          std::abort();
        }
      } else if (op == 'Q') {
        const auto [left, right] = make_range(rng, reference.size(), anchor, step);
        mint expected = 0;
        for (ll k = left; k < right; ++k) expected += reference[k];
        const auto actual = tree.prod(left, right);
        if (expected != actual) {
          std::cerr << "mixed_edges prod mismatch: capacity=" << Capacity
                    << ", operation=" << step << ", range=[" << left
                    << ',' << right << "), expected=" << expected.val()
                    << ", actual=" << actual.val() << '\n';
          std::abort();
        }
      } else if (op == 'A') {
        const auto [left, right] = make_range(rng, reference.size(), anchor, step);
        const ll a = rng.next() % 3;
        const ll b = rng.next() % 1009;
        for (ll k = left; k < right; ++k) reference[k] = a * reference[k] + b;
        tree.apply(left, right, {mint(a), mint(b)});
      }
      assert(tree.size() == reference.size());
    }
  }

  for (ll i = 0; i < static_cast<ll>(reference.size()); ++i)
    assert(tree[i] == reference[i]);
  std::cerr << "mixed_edges lazy test passed! capacity=" << Capacity << '\n';
}
}  // namespace

signed main() {
  mixed_edges_test<8>();
  mixed_edges_test<16>();
  std::cerr << "All mixed_edges lazy regression tests passed!\n";
}
