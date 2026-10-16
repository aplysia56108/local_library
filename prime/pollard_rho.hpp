#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

#include "prime/miller_rabin.hpp"

/** @brief Pollard's Rho 法による64bit整数の素因数分解。 */
struct PollardRho {
  using u64 = std::uint64_t;
  using u128 = __uint128_t;

  static u64 mul_mod(u64 a, u64 b, u64 mod) {
    return MillerRabin::mul_mod(a, b, mod);
  }

  static u64 add_mod(u64 a, u64 b, u64 mod) {
    return static_cast<u64>((static_cast<u128>(a) + b) % mod);
  }

  static u64 random(u64 lo, u64 hi) {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    return std::uniform_int_distribution<u64>(lo, hi)(rng);
  }

  // n は奇数の合成数。
  static u64 find_factor(u64 n) {
    if (n % 3 == 0) return 3;
    while (true) {
      u64 y = random(1, n - 1);
      u64 c = random(1, n - 1);
      auto f = [&](u64 x) {
        return add_mod(mul_mod(x, x, n), c, n);
      };
      u64 r = 1, q = 1, g = 1, x = 0, ys = 0;
      constexpr u64 m = 128;
      while (g == 1) {
        x = y;
        for (u64 i = 0; i < r; ++i) y = f(y);
        u64 k = 0;
        while (k < r && g == 1) {
          ys = y;
          q = 1;
          u64 len = std::min(m, r - k);
          for (u64 i = 0; i < len; ++i) {
            y = f(y);
            u64 diff = x > y ? x - y : y - x;
            q = mul_mod(q, diff, n);
          }
          g = std::gcd(q, n);
          k += len;
        }
        r *= 2;
      }
      if (g == n) {
        do {
          ys = f(ys);
          u64 diff = x > ys ? x - ys : ys - x;
          g = std::gcd(diff, n);
        } while (g == 1);
      }
      if (g != n) return g;
    }
  }

  static void factorize_impl(u64 n, std::vector<long long>& res) {
    if (n == 1) return;
    if (MillerRabin::is_prime(static_cast<long long>(n))) {
      res.push_back(static_cast<long long>(n));
      return;
    }
    if (n % 2 == 0) {
      res.push_back(2);
      factorize_impl(n / 2, res);
      return;
    }
    u64 d = find_factor(n);
    factorize_impl(d, res);
    factorize_impl(n / d, res);
  }

  /**
   * @brief n を素因数分解する。
   * @param n 正の整数
   * @return (素因数, 指数) の昇順配列
   *
   * ### 計算量
   * 乱択アルゴリズム。実行時間は入力と乱数に依存する。
   */
  static std::vector<std::pair<long long, long long>> factorize(long long n) {
    assert(n >= 1);
    std::vector<long long> factors;
    factorize_impl(static_cast<u64>(n), factors);
    std::sort(factors.begin(), factors.end());
    std::vector<std::pair<long long, long long>> res;
    for (long long p : factors) {
      if (res.empty() || res.back().first != p)
        res.emplace_back(p, 1);
      else
        ++res.back().second;
    }
    return res;
  }

  /**
   * @brief n の正の約数を列挙する。
   * @param n 正の整数
   * @return 正の約数（昇順とは限らない）
   */
  static std::vector<long long> divisors(long long n) {
    auto pf = factorize(n);
    std::vector<long long> res = {1};
    for (auto [p, e] : pf) {
      long long sz = static_cast<long long>(res.size());
      long long power = 1;
      for (long long j = 1; j <= e; ++j) {
        power *= p;
        for (long long i = 0; i < sz; ++i) res.push_back(res[i] * power);
      }
    }
    return res;
  }
};
