#pragma once

#include <bits/stdc++.h>
#include <cstdint>

/**
 * @brief 混合操作とアクセス分布の入力・正解を B+tree に依存せずに生成する。
 * @details Set は std::set、その他は std::vector を参照実装とする。
 * ### 計算量
 * std::vector の挿入・削除・反転は O(N)、set は O(log N)。
 * 計算はすべてベンチマークの計測対象外。
 */
namespace mixed_generator {
using ll = long long;
constexpr ll N = 100'000;
constexpr ll Q = 100'000;
constexpr ll MOD = 998'244'353;

#if defined(BENCH_TREE_SET)
constexpr std::string_view pattern = "IIIIIIEEEEEELLLLLLLL";
#elif defined(BENCH_TREE_VECTOR)
constexpr std::string_view pattern = "IIIIIIEEEEDDRRRRSGGG";
#elif defined(BENCH_TREE_SEGTREE)
constexpr std::string_view pattern = "IIIIIEEEDRRRSGGGQQQQ";
#elif defined(BENCH_TREE_LAZY)
constexpr std::string_view pattern = "IIIIIEEEDRRSGGQQQAAA";
#else
#error "Unknown benchmark tree"
#endif
static_assert(pattern.size() == 20);

struct Rng {
  std::uint32_t state = 0x9e3779b9u;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
};

inline ll set_key(ll id) { return (id * 48'271 + 9'176) % 1'000'000'007; }
inline ll vector_value(ll id) { return (id * 48'271 + 9'176) % MOD; }
inline ll lazy_value(ll id) { return (id * 809 + 17) % 10'009; }
inline std::pair<ll, ll> affine_value(ll id) {
  return {1 + (id * 17 + 5) % 3, (id * 1'009 + 19) % 10'001};
}
inline std::pair<ll, ll> affine_op(std::pair<ll, ll> a,
                                    std::pair<ll, ll> b) {
  return {a.first * b.first % MOD,
          (a.second + a.first * b.second) % MOD};
}

#if defined(BENCH_TREE_SEGTREE)
using Value = std::pair<ll, ll>;
inline Value value_at(ll id) { return affine_value(id); }
#elif defined(BENCH_TREE_SET)
using Value = ll;
inline Value value_at(ll id) { return set_key(id); }
#elif defined(BENCH_TREE_VECTOR)
using Value = ll;
inline Value value_at(ll id) { return vector_value(id); }
#else
using Value = ll;
inline Value value_at(ll id) { return lazy_value(id); }
#endif

inline void print(std::ostream& os, ll value) { os << value << '\n'; }
inline void print(std::ostream& os, std::pair<ll, ll> value) {
  os << value.first << ' ' << value.second << '\n';
}

// mode=0: uniform, mode=1: either edge, mode=2: near the previous position.
inline ll position(Rng& rng, ll count, ll& anchor) {
  assert(count > 0);
  ll p;
#if BENCH_DISTRIBUTION == 0
  p = rng.next() % count;
#elif BENCH_DISTRIBUTION == 1
  const ll span = std::min(64LL, count);
  const bool left = rng.next() % 2 == 0;
  const ll offset = rng.next() % span;
  p = left ? offset : count - 1 - offset;
#elif BENCH_DISTRIBUTION == 2
  const ll delta = static_cast<ll>(rng.next() % 129) - 64;
  p = std::clamp(anchor + delta, 0LL, count - 1);
#else
#error "Unknown distribution"
#endif
  anchor = p;
  return p;
}

inline std::pair<ll, ll> range(Rng& rng, ll size, ll& anchor,
                               ll step, bool allow_long) {
  assert(size > 0);
  const ll length = allow_long && step % 200 == 0
                        ? std::max(1LL, size / 4)
                        : 1 + rng.next() % std::min(32LL, size);
  const ll left = position(rng, size - length + 1, anchor);
  return {left, left + length};
}

#if defined(BENCH_TREE_SET)
struct SetReference {
  std::set<ll> sorted;
  std::vector<ll> alive;
  std::unordered_map<ll, ll> slot;
  ll anchor = set_key(0);

  void insert(ll key) {
    assert(sorted.insert(key).second);
    slot.emplace(key, static_cast<ll>(alive.size()));
    alive.push_back(key);
  }
  void erase(ll key) {
    assert(sorted.erase(key) == 1);
    const ll i = slot.at(key);
    const ll last = alive.back();
    alive[i] = last;
    slot[last] = i;
    alive.pop_back();
    slot.erase(key);
  }
  ll choose_existing(Rng& rng) {
    assert(!alive.empty());
    ll key;
#if BENCH_DISTRIBUTION == 0
    key = alive[rng.next() % alive.size()];
#elif BENCH_DISTRIBUTION == 1
    key = rng.next() % 2 ? *sorted.begin() : *sorted.rbegin();
#else
    auto it = sorted.lower_bound(anchor);
    if (it == sorted.end()) it = std::prev(sorted.end());
    ll steps = rng.next() % 16;
    while (steps-- && std::next(it) != sorted.end()) ++it;
    key = *it;
#endif
    anchor = key;
    return key;
  }
  ll choose_lookup(Rng& rng) {
    ll key;
#if BENCH_DISTRIBUTION == 0
    const ll mode = rng.next() % 4;
    if (mode == 0) key = alive[rng.next() % alive.size()];
    else if (mode == 1) key = 1'000'000'008LL;
    else if (mode == 2) key = -1;
    else key = set_key(rng.next() % (N + Q)) + 1;
#elif BENCH_DISTRIBUTION == 1
    key = (rng.next() % 2 ? *sorted.begin() : *sorted.rbegin())
          + static_cast<ll>(rng.next() % 17) - 8;
#else
    key = anchor + static_cast<ll>(rng.next() % 513) - 256;
#endif
    anchor = key;
    return key;
  }
};
#endif
}  // namespace mixed_generator

signed main() {
  using namespace mixed_generator;
  std::ofstream input("in.txt"), output("out.txt");
  if (!input || !output) return 1;
  input << N << ' ' << Q << '\n';
  Rng rng;
  ll next_id = N;
#if defined(BENCH_TREE_SET)
  SetReference ref;
  ref.alive.reserve(N + Q);
  ref.slot.reserve(N + Q);
  for (ll i = 0; i < N; ++i) {
    const ll key = set_key(i);
    ref.insert(key);
    print(input, key);
  }
#else
  std::vector<Value> ref;
  ref.reserve(N + Q);
  for (ll i = 0; i < N; ++i) {
    const auto value = value_at(i);
    ref.push_back(value);
    print(input, value);
  }
  ll anchor = N / 2;
#endif

  std::string ops(pattern);
  for (ll block = 0; block < Q / static_cast<ll>(pattern.size()); ++block) {
    // Shuffle each fixed-ratio block, avoiding a repeated batch of identical ops.
    for (ll j = static_cast<ll>(ops.size()) - 1; j > 0; --j)
      std::swap(ops[j], ops[rng.next() % (j + 1)]);
    for (ll j = 0; j < static_cast<ll>(ops.size()); ++j) {
#if !defined(BENCH_TREE_SET)
      const ll step = block * static_cast<ll>(ops.size()) + j;
#endif
      const char op = ops[j];
      if (op == 'I') {
#if defined(BENCH_TREE_SET)
        const ll key = set_key(next_id++);
        input << "I " << key << '\n';
        ref.insert(key);
#else
        const ll pos = position(rng, static_cast<ll>(ref.size()) + 1, anchor);
        const Value value = value_at(next_id++);
        input << "I " << pos << ' ';
        print(input, value);
        ref.insert(ref.begin() + pos, value);
#endif
      } else if (op == 'E') {
#if defined(BENCH_TREE_SET)
        const ll key = ref.choose_existing(rng);
        input << "E " << key << '\n';
        ref.erase(key);
#else
        const ll pos = position(rng, static_cast<ll>(ref.size()), anchor);
        input << "E " << pos << '\n';
        ref.erase(ref.begin() + pos);
#endif
      } else if (op == 'L') {
#if defined(BENCH_TREE_SET)
        const ll key = ref.choose_lookup(rng);
        input << "L " << key << '\n';
        const auto it = ref.sorted.lower_bound(key);
        print(output, it == ref.sorted.end() ? -1LL : *it);
#else
        return 2;
#endif
      } else if (op == 'G') {
#if !defined(BENCH_TREE_SET)
        const ll pos = position(rng, static_cast<ll>(ref.size()), anchor);
        input << "G " << pos << '\n';
        print(output, ref[pos]);
#else
        return 2;
#endif
      } else if (op == 'D') {
#if !defined(BENCH_TREE_SET)
        const ll length = 1 + rng.next() % std::min(3LL, static_cast<ll>(ref.size()));
        const ll left = position(rng, static_cast<ll>(ref.size()) - length + 1, anchor);
        input << "D " << left << ' ' << left + length << '\n';
        ref.erase(ref.begin() + left, ref.begin() + left + length);
#else
        return 2;
#endif
      } else if (op == 'R') {
#if !defined(BENCH_TREE_SET)
        const auto [left, right] = range(rng, static_cast<ll>(ref.size()), anchor, step, true);
        input << "R " << left << ' ' << right << '\n';
        std::reverse(ref.begin() + left, ref.begin() + right);
#else
        return 2;
#endif
      } else if (op == 'S') {
#if !defined(BENCH_TREE_SET)
        const ll pos = position(rng, static_cast<ll>(ref.size()) + 1, anchor);
        input << "S " << pos << '\n';
#else
        return 2;
#endif
      } else if (op == 'Q') {
#if defined(BENCH_TREE_SEGTREE) || defined(BENCH_TREE_LAZY)
        const auto [left, right] = range(rng, static_cast<ll>(ref.size()), anchor, step, true);
        input << "Q " << left << ' ' << right << '\n';
#if defined(BENCH_TREE_SEGTREE)
        Value value{1, 0};
        for (ll i = left; i < right; ++i) value = affine_op(value, ref[i]);
        print(output, value);
#else
        ll sum = 0;
        for (ll i = left; i < right; ++i) sum = (sum + ref[i]) % MOD;
        print(output, sum);
#endif
#else
        return 2;
#endif
      } else if (op == 'A') {
#if defined(BENCH_TREE_LAZY)
        const auto [left, right] = range(rng, static_cast<ll>(ref.size()), anchor, step, true);
        const ll a = rng.next() % 3;
        const ll b = rng.next() % 1009;
        input << "A " << left << ' ' << right << ' ' << a << ' ' << b << '\n';
        for (ll i = left; i < right; ++i) ref[i] = (a * ref[i] + b) % MOD;
#else
        return 2;
#endif
      } else {
        return 2;
      }
    }
  }
#if defined(BENCH_TREE_SET)
  for (const ll value : ref.sorted) print(output, value);
#else
  for (const auto& value : ref) print(output, value);
#endif
}
