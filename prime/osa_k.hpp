#pragma once

#include <algorithm>
#include <cassert>
#include <limits>
#include <utility>
#include <vector>

/**
 * @brief 動的拡張可能な Osa-K 法
 *
 * 必要に応じて篩を2倍ずつ拡張し、最小素因数を保持する。
 */
struct OsaK {
  std::vector<long long> spf;
  std::vector<long long> primes;

  /**
   * @brief n 以下の篩を構築する。
   *
   * ### 計算量
   * O(n log log n)
   */
  explicit OsaK(long long n = 16) : spf(2, 0) {
    spf[1] = 1;
    ensure(n);
  }

  /**
   * @brief 篩が構築済みの最大値を返す。
   * @return 構築済みの最大値
   *
   * ### 計算量
   * O(1)
   */
  long long limit() {
    return (long long)spf.size() - 1;
  }

  /**
   * @brief n 以下まで篩を拡張する。
   *
   * すでに範囲内なら何もしない。
   * 必要な場合は現在の上限を2倍ずつ増やす。
   */
  void ensure(long long n) {
    assert(n >= 0);
    long long old = limit();
    if (n <= old) return;

    long long limit = old;
    while (limit < n) limit <<= 1;
    spf.resize(limit + 1, 0);

    // 新しい区間の合成数を篩う
    for (long long p = 2; p <= limit / p; ++p) {
      if (spf[p] == 0) spf[p] = p;
      if (spf[p] != p) continue;

      long long start = (old + 1) / p * p;
      if (start < old + 1) start += p;
      start = std::max(start, p * p);

      for (long long j = start; j <= limit; j += p) {
        if (spf[j] == 0) spf[j] = p;
      }
    }

    // 新しい素数を登録する
    for (long long i = std::max(2LL, old + 1); i <= limit; ++i) {
      if (spf[i] == 0) {
        spf[i] = i;
        primes.push_back(i);
      }
    }
  }

  /**
   * @brief n を素因数分解する。
   * @return (素因数, 指数) の昇順の配列
   *
   * ### 計算量
   * 篩の構築済み範囲内なら O(log n)
   */
  std::vector<std::pair<long long, long long>> factorize(long long n) {
    assert(n >= 1);
    ensure(n);
    std::vector<std::pair<long long, long long>> res;
    while (n > 1) {
      long long p = spf[n];
      long long cnt = 0;
      do {
        n /= p;
        ++cnt;
      } while (n > 1 && spf[n] == p);
      res.emplace_back(p, cnt);
    }
    return res;
  }

  /**
   * @brief n の正の約数を列挙する。
   * @return 約数の配列（昇順とは限らない）
   *
   * ### 計算量
   * 篩の構築済み範囲内なら O(log n + d(n))
   */
  std::vector<long long> divisors(long long n) {
    auto pf = factorize(n);
    std::vector<long long> res = {1};
    for (auto [p, e] : pf) {
      long long sz = res.size();
      long long power = 1;
      for (long long j = 1; j <= e; ++j) {
        power *= p;
        for (long long i = 0; i < sz; ++i) {
          res.push_back(res[i] * power);
        }
      }
    }
    return res;
  }

  /**
   * @brief n が素数か判定する。
   *
   * ### 計算量
   * 篩の構築済み範囲内なら O(1)
   */
  bool is_prime(long long n) {
    assert(n >= 0);
    ensure(n);
    return n >= 2 && spf[n] == n;
  }
};
