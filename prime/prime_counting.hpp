#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

/**
 * @brief Lehmer 法で n 以下の素数の個数を求める。
 *
 * 素数表は count(n) に応じて遅延構築し、必要に応じて拡張する。
 * 引数は非負整数とする。同一インスタンスの同時呼び出しは非対応。
 */
struct PrimeCounting {
  std::vector<long long> primes;
  std::vector<long long> pi;

  /** @brief 軽量な初期状態を作る。 */
  PrimeCounting() : pi(2, 0) {
    init_phi();
  }

  /**
   * @brief 篩が構築済みの上限を返す。
   * ### 計算量
   * O(1)
   */
  long long limit() const {
    return (long long)pi.size() - 1;
  }

  /**
   * @brief n 以下の素数の個数を返す。
   * @param n 非負整数。実用上の計算時間・メモリに収まる範囲を想定。
   * @return n 以下の素数の個数
   * @details 必要な篩を自動構築し、過去の篩と一部の計算結果を再利用する。
   */
  long long count(long long n) {
    assert(n >= 0);
    if (n <= limit()) return pi[n];
    ensure(isqrt(n));
    return lehmer(n);
  }

  /**
   * @brief 篩を n 以上まで拡張する。必要な場合、上限は概ね2倍になる。
   * @param n 篩を構築する上限
   */
  void ensure(long long n) {
    assert(n >= 0);
    if (n <= limit()) return;
    long long target = limit();
    while (target < n) target *= 2;
    // 再構築方式：構築済みの範囲は再計算されるが、倍増により償却オーダーは変わらない。
    std::vector<bool> composite(target + 1, false);
    for (long long i = 2; i <= target / i; ++i) {
      if (composite[i]) continue;
      for (long long j = i * i; j <= target; j += i) composite[j] = true;
    }
    primes.clear();
    pi.assign(target + 1, 0);
    for (long long i = 2; i <= target; ++i) {
      if (!composite[i]) primes.push_back(i);
      pi[i] = pi[i - 1] + !composite[i];
    }
  }

 private:
  static constexpr long long kPhiPeriod = 30030;  // 2*3*5*7*11*13
  static constexpr long long kPhiSmall = 6;
  std::vector<std::vector<long long>> phi_table;
  std::unordered_map<long long, long long> cache;

  static long long isqrt(long long n) {
    long long x = (long long)std::sqrt((long double)n);
    while ((__int128)(x + 1) * (x + 1) <= n) ++x;
    while ((__int128)x * x > n) --x;
    return x;
  }

  static long long icbrt(long long n) {
    long long x = (long long)std::cbrt((long double)n);
    while ((__int128)(x + 1) * (x + 1) * (x + 1) <= n) ++x;
    while ((__int128)x * x * x > n) --x;
    return x;
  }

  static long long iroot4(long long n) {
    return isqrt(isqrt(n));
  }

  void init_phi() {
    const long long small_primes[kPhiSmall] = {2, 3, 5, 7, 11, 13};
    phi_table.assign(kPhiSmall + 1, std::vector<long long>(kPhiPeriod + 1));
    for (long long x = 0; x <= kPhiPeriod; ++x) phi_table[0][x] = x;
    for (long long a = 1; a <= kPhiSmall; ++a) {
      for (long long x = 0; x <= kPhiPeriod; ++x) {
        phi_table[a][x] =
            phi_table[a - 1][x] - phi_table[a - 1][x / small_primes[a - 1]];
      }
    }
  }

  long long phi(long long x, long long a) {
    if (a <= kPhiSmall) {
      return x / kPhiPeriod * phi_table[a][kPhiPeriod] +
             phi_table[a][x % kPhiPeriod];
    }
    if (x < primes[a - 1]) return 1;
    if (x <= limit() && primes[a - 1] > x / primes[a - 1]) {
      return pi[x] - a + 1;
    }
    return phi(x, a - 1) - phi(x / primes[a - 1], a - 1);
  }

  long long lehmer(long long x) {
    if (x <= limit()) return pi[x];
    auto it = cache.find(x);
    if (it != cache.end()) return it->second;

    long long a = lehmer(iroot4(x));
    long long b = lehmer(isqrt(x));
    long long c = lehmer(icbrt(x));
    long long result = phi(x, a) + (b + a - 2) * (b - a + 1) / 2;
    for (long long i = a; i < b; ++i) {
      long long w = x / primes[i];
      result -= lehmer(w);
      if (i < c) {
        long long bi = lehmer(isqrt(w));
        for (long long j = i; j < bi; ++j) {
          result -= lehmer(w / primes[j]) - j;
        }
      }
    }
    // 巨大な連続クエリでキャッシュが無制限に増えるのを防ぐ。
    if (cache.size() < 200000) cache.emplace(x, result);
    return result;
  }
};
