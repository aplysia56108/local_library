#pragma once

#include <cstdint>
#include <initializer_list>

/** @brief 64bit 非負整数の決定的素数判定。 */
struct MillerRabin {
  using u64 = std::uint64_t;
  using u128 = __uint128_t;

  static u64 mul_mod(u64 a, u64 b, u64 mod) {
    return static_cast<u64>(static_cast<u128>(a) * b % mod);
  }

  static u64 pow_mod(u64 a, u64 e, u64 mod) {
    u64 res = 1;
    while (e > 0) {
      if (e & 1) res = mul_mod(res, a, mod);
      a = mul_mod(a, a, mod);
      e >>= 1;
    }
    return res;
  }

  /**
   * @brief n が素数か判定する。
   * @param n 判定対象
   * @return 素数なら true
   *
   * ### 計算量
   * O(log n) 回の剰余乗算
   */
  static bool is_prime(long long n) {
    if (n < 2) return false;
    u64 x = static_cast<u64>(n);
    for (u64 p : {2ULL, 3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL, 23ULL,
                  29ULL, 31ULL, 37ULL}) {
      if (x % p == 0) return x == p;
    }
    u64 d = x - 1;
    long long s = 0;
    while ((d & 1) == 0) {
      d >>= 1;
      ++s;
    }
    for (u64 a : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL,
                  1795265022ULL}) {
      a %= x;
      if (a == 0) continue;
      u64 y = pow_mod(a, d, x);
      if (y == 1 || y == x - 1) continue;
      bool passed = false;
      for (long long r = 1; r < s; ++r) {
        y = mul_mod(y, y, x);
        if (y == x - 1) {
          passed = true;
          break;
        }
      }
      if (!passed) return false;
    }
    return true;
  }
};
