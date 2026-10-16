#pragma once

#include <cassert>
#include <utility>
#include <vector>

#include "prime/osa_k.hpp"
#include "prime/pollard_rho.hpp"
#include "prime/miller_rabin.hpp"

/** @brief Osa-K と Pollard's Rho を自動で使い分ける。 */
struct Prime {
  static constexpr long long kThreshold = 1000000;
  OsaK osa;

  explicit Prime(long long n = 16) : osa(n) {}

  /** @brief 素数判定。小さい整数は篩を自動拡張する。 */
  bool is_prime(long long n) {
    if (n < 2) return false;
    if (n <= osa.limit() || n <= kThreshold) return osa.is_prime(n);
    return MillerRabin::is_prime(n);
  }

  /** @brief 素因数分解結果を昇順で返す。 */
  std::vector<std::pair<long long, long long>> factorize(long long n) {
    assert(n >= 1);
    if (n <= osa.limit() || n <= kThreshold) return osa.factorize(n);
    return PollardRho::factorize(n);
  }

  /** @brief 約数を列挙する（昇順とは限らない）。 */
  std::vector<long long> divisors(long long n) {
    assert(n >= 1);
    if (n <= osa.limit() || n <= kThreshold) return osa.divisors(n);
    return PollardRho::divisors(n);
  }
};
