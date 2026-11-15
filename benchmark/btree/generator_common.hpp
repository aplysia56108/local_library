#pragma once

#include <bits/stdc++.h>
#include <cstdint>

/**
 * @brief B+tree ベンチマークの独立した入力・正解生成器。
 * @details 固定seedで入力を作成し、std::vector や独立した Fenwick tree
 * で期待値を求める。ライブラリ本体には依存しない。
 */

namespace benchmark_generator {

using ll = long long;
#if defined(BENCH_OP_MAX_RIGHT) || defined(BENCH_OP_MIN_LEFT)
#define BENCH_BOUNDARY_SEARCH
#endif
constexpr ll Q = 100'000;
constexpr ll MOD = 998'244'353;

struct Rng {
  std::uint32_t state = 0x9e3779b9u;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
};

inline ll set_key(ll i) { return (i * 48'271) % 1'000'000'007; }
inline ll vector_value(ll i) { return (i * 48'271 + 9'176) % MOD; }
inline ll lazy_value(ll i) { return (i * 809 + 17) % 10'009; }
inline std::pair<ll, ll> affine_value(ll i) {
  return {1 + (i * 17 + 5) % 3, (i * 1'009 + 19) % 10'001};
}
inline std::pair<ll, ll> affine_op(std::pair<ll, ll> a,
                                    std::pair<ll, ll> b) {
  return {a.first * b.first % MOD,
          (a.second + a.first * b.second) % MOD};
}

/** @brief 動的順位選択のための独立した参照実装。 */
class Fenwick {
  ll n_;
  std::vector<ll> bit_;

 public:
  explicit Fenwick(ll n) : n_(n), bit_(n + 1, 0) {
    for (ll i = 0; i < n; ++i) add(i, 1);
  }
  void add(ll position, ll delta) {
    for (ll i = position + 1; i <= n_; i += i & -i) bit_[i] += delta;
  }
  ll kth(ll order) const {
    ll index = 0;
    ll step = 1;
    while (step * 2 <= n_) step *= 2;
    for (; step; step /= 2) {
      if (index + step <= n_ && bit_[index + step] <= order) {
        order -= bit_[index + step];
        index += step;
      }
    }
    return index;
  }
};

inline std::pair<ll, ll> make_range(Rng& rng, ll n, ll i) {
  const ll l = rng.next() % n;
  const ll limit = std::min(n - l, i % 1000 == 0 ? n : 32LL);
  return {l, l + 1 + rng.next() % limit};
}

#if defined(BENCH_BOUNDARY_SEARCH)
using Value = ll;
inline Value value_at(ll i) { return lazy_value(i); }
#elif defined(BENCH_TREE_SEGTREE)
using Value = std::pair<ll, ll>;
inline Value value_at(ll i) { return affine_value(i); }
#elif defined(BENCH_TREE_SET)
using Value = ll;
inline Value value_at(ll i) { return set_key(i); }
#elif defined(BENCH_TREE_LAZY)
using Value = ll;
inline Value value_at(ll i) { return lazy_value(i); }
#elif defined(BENCH_TREE_VECTOR)
using Value = ll;
inline Value value_at(ll i) { return vector_value(i); }
#else
#error "Unknown B+tree benchmark type"
#endif

inline void print(std::ostream& os, ll value) { os << value << '\n'; }
inline void print(std::ostream& os, std::pair<ll, ll> value) {
  os << value.first << ' ' << value.second << '\n';
}
}  // namespace benchmark_generator

/**
 * @brief 100,000回の操作を入力に出力し、独立した方法で正解を作る。
 *
 * ### 計算量
 * 区間操作は O(Q * 平均区間長)、挿入・削除は O(Q log Q)、
 * setの全列挙は O(N log N)。いずれも計測対象外。
 */
signed main() {
  using namespace benchmark_generator;
#if defined(BENCH_OP_INSERT)
  constexpr ll N = 0;
#elif defined(BENCH_OP_ERASE)
  constexpr ll N = 2 * Q;
#elif defined(BENCH_OP_RANGE_ERASE)
  constexpr ll N = 5 * Q;
#else
  constexpr ll N = Q;
#endif

  std::ofstream input("in.txt"), output("out.txt");
  if (!input || !output) return 1;
  input << N << ' ' << Q << '\n';
  std::vector<Value> initial;
  initial.reserve(N);
  for (ll i = 0; i < N; ++i) {
    initial.push_back(value_at(i));
    print(input, initial.back());
  }
  Rng rng;

#if defined(BENCH_OP_INSERT)
#if defined(BENCH_TREE_SET)
  std::vector<ll> answer;
  answer.reserve(Q);
  for (ll i = 0; i < Q; ++i) {
    ll key = set_key(i);
    input << key << '\n';
    answer.push_back(key);
  }
  std::sort(answer.begin(), answer.end());
  for (ll x : answer) print(output, x);
#else
  std::vector<ll> positions(Q);
  for (ll i = 0; i < Q; ++i) {
    positions[i] = rng.next() % (i + 1);
    input << positions[i] << ' ';
    print(input, value_at(i));
  }
  // 逆順に空スロットを確定させることで std::vector::insert と同じ
  // 最終配列を O(Q log Q) で作る。
  Fenwick free_slots(Q);
  std::vector<Value> answer(Q);
  for (ll i = Q; i-- > 0;) {
    ll slot = free_slots.kth(positions[i]);
    answer[slot] = value_at(i);
    free_slots.add(slot, -1);
  }
  for (const auto& x : answer) print(output, x);
#endif

#elif defined(BENCH_OP_ERASE)
#if defined(BENCH_TREE_SET)
  std::vector<char> removed(N, false);
  for (ll i = 0; i < Q; ++i) {
    ll original_index = (i * 131'071) % N;
    input << set_key(original_index) << '\n';
    removed[original_index] = true;
  }
  std::vector<ll> answer;
  answer.reserve(N - Q);
  for (ll i = 0; i < N; ++i)
    if (!removed[i]) answer.push_back(set_key(i));
  std::sort(answer.begin(), answer.end());
  for (ll x : answer) print(output, x);
#else
  Fenwick alive(N);
  std::vector<char> removed(N, false);
  for (ll i = 0; i < Q; ++i) {
    ll index = rng.next() % (N - i);
    input << index << '\n';
    ll original_index = alive.kth(index);
    removed[original_index] = true;
    alive.add(original_index, -1);
  }
  for (ll i = 0; i < N; ++i)
    if (!removed[i]) print(output, initial[i]);
#endif

#elif defined(BENCH_OP_RANGE_ERASE)
  Fenwick alive(N);
  std::vector<char> removed(N, false);
  ll remaining = N;
  for (ll i = 0; i < Q; ++i) {
    // Short but nontrivial contiguous erases; always keep enough live elements.
    const ll count = std::min(remaining - (Q - i - 1),
                              1 + static_cast<ll>(rng.next() % 6));
    const ll left = rng.next() % (remaining - count + 1);
    input << left << ' ' << left + count << '\n';
    for (ll j = 0; j < count; ++j) {
      const ll original_index = alive.kth(left);
      removed[original_index] = true;
      alive.add(original_index, -1);
    }
    remaining -= count;
  }
  for (ll i = 0; i < N; ++i)
    if (!removed[i]) print(output, initial[i]);

#elif defined(BENCH_OP_SPLIT_MERGE)
  for (ll i = 0; i < Q; ++i) {
    // Include both empty sides and near-edge splits, not just central splits.
    ll index;
    if (i % 1024 == 0) index = 0;
    else if (i % 1024 == 1) index = N;
    else if (i % 1024 == 2) index = 1;
    else if (i % 1024 == 3) index = N - 1;
    else index = rng.next() % (N + 1);
    input << index << '\n';
  }
  for (const auto& x : initial) print(output, x);

#elif defined(BENCH_OP_LOWER_BOUND)
  std::sort(initial.begin(), initial.end());
  for (ll i = 0; i < Q; ++i) {
    // Include exact keys, misses between keys, and out-of-range searches.
    const ll key = i % 4 == 0
                       ? set_key(rng.next() % N)
                       : static_cast<ll>(rng.next() % 1'000'000'009u);
    input << key << '\n';
    const auto it = std::lower_bound(initial.begin(), initial.end(), key);
    print(output, it == initial.end() ? -1LL : *it);
  }

#elif defined(BENCH_BOUNDARY_SEARCH)
  std::vector<ll> prefix(N + 1, 0);
  for (ll i = 0; i < N; ++i) prefix[i + 1] = prefix[i] + initial[i];
  for (ll i = 0; i < Q; ++i) {
    const ll position = rng.next() % (N + 1);
    const ll limit = i % 1000 == 0 ? 1'000'000'000'000LL
                                   : static_cast<ll>(rng.next() % 1'000'000);
    input << position << ' ' << limit << '\n';
#if defined(BENCH_OP_MAX_RIGHT)
    const auto it = std::upper_bound(prefix.begin() + position, prefix.end(),
                                     prefix[position] + limit);
    print(output, static_cast<ll>(it - prefix.begin()) - 1);
#else
    const auto it = std::lower_bound(prefix.begin(), prefix.begin() + position + 1,
                                     prefix[position] - limit);
    print(output, static_cast<ll>(it - prefix.begin()));
#endif
  }

#elif defined(BENCH_OP_ITERATE)
#if defined(BENCH_TREE_SET)
  std::sort(initial.begin(), initial.end());
#endif
  for (const auto& x : initial) print(output, x);

#elif defined(BENCH_OP_ACCESS)
  if constexpr (std::is_same_v<Value, ll>) {
#if defined(BENCH_TREE_SET)
    std::sort(initial.begin(), initial.end());
#endif
  }
  for (const auto& x : initial) print(output, x);

#elif defined(BENCH_OP_REVERSE)
  for (ll i = 0; i < Q; ++i) {
    auto [l, r] = make_range(rng, N, i);
    input << l << ' ' << r << '\n';
    std::reverse(initial.begin() + l, initial.begin() + r);
  }
  for (const auto& x : initial) print(output, x);

#elif defined(BENCH_OP_PROD)
#if defined(BENCH_TREE_LAZY)
  std::vector<ll> prefix(N + 1);
  for (ll i = 0; i < N; ++i)
    prefix[i + 1] = (prefix[i] + initial[i]) % MOD;
#endif
  for (ll i = 0; i < Q; ++i) {
    auto [l, r] = make_range(rng, N, i);
    input << l << ' ' << r << '\n';
#if defined(BENCH_TREE_SEGTREE)
    Value answer{1, 0};
    for (ll j = l; j < r; ++j) answer = affine_op(answer, initial[j]);
    print(output, answer);
#elif defined(BENCH_TREE_LAZY)
    ll answer = (prefix[r] - prefix[l] + MOD) % MOD;
    print(output, answer);
#else
#error "prod only supports segment trees"
#endif
  }

#elif defined(BENCH_OP_APPLY)
  for (ll i = 0; i < Q; ++i) {
    auto [l, r] = make_range(rng, N, i);
    const ll a = rng.next() % 3;
    const ll b = rng.next() % 1009;
    input << l << ' ' << r << ' ' << a << ' ' << b << '\n';
    for (ll j = l; j < r; ++j) initial[j] = (a * initial[j] + b) % MOD;
  }
  for (const auto& x : initial) print(output, x);
#else
#error "Unknown benchmark operation"
#endif
}
