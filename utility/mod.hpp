#pragma once

#include <atcoder/all>
#include <cassert>
#include <iostream>
#include <vector>

// ==========================================
// 1. Binomial (二項係数ライブラリ)
// ==========================================

/**
 * @brief 動的拡張対応 二項係数・階乗テーブル
 * @tparam T 計算に用いる型 (long long または atcoder::modint / dynamic_modint)
 */
template <class T = long long>
struct Binomial {
  long long
      p;  // モジュロ (T が modint の場合は使用されないが汎用性のために保持)
  std::vector<T> fac, finv, inv;

  Binomial(long long p_ = 998244353) : p(p_) {}

  // テーブルを指定サイズまで動的に拡張
  void extend(int n) {
    if (n < 0) return;

    // 初回呼び出し時に 0! の基本テーブルを作成
    if (fac.empty()) {
      fac = {T(1), T(1)};
      finv = {T(1), T(1)};
      inv = {T(0), T(1)};
    }
    int old_size = fac.size();
    if (old_size > n) return;

    fac.resize(n + 1);
    finv.resize(n + 1);
    inv.resize(n + 1);

    for (int i = old_size; i <= n; i++) {
      if constexpr (std::is_integral_v<T>) {
        fac[i] = fac[i - 1] * i % p;
        inv[i] = p - inv[p % i] * (p / i) % p;
        finv[i] = finv[i - 1] * inv[i] % p;
      } else {
        // atcoder::modint 等の型の場合
        fac[i] = fac[i - 1] * i;
        inv[i] = T(1) / i;
        finv[i] = finv[i - 1] * inv[i];
      }
    }
  }

  // 階乗 n!
  T fact(int n) {
    if (n < 0) return 0;
    extend(n);
    return fac[n];
  }

  // 階乗の逆元 1/n!
  T fact_inv(int n) {
    if (n < 0) return 0;
    extend(n);
    return finv[n];
  }

  // 元の逆元 1/n
  T val_inv(int n) {
    if (n <= 0) return 0;
    extend(n);
    return inv[n];
  }

  // 組合せ nCk
  T C(int n, int k) {
    if (n < k || n < 0 || k < 0) return 0;
    extend(n);
    if constexpr (std::is_integral_v<T>) {
      return fac[n] * (finv[k] * finv[n - k] % p) % p;
    } else {
      return fac[n] * finv[k] * finv[n - k];
    }
  }

  // 順列 nPk
  T P(int n, int k) {
    if (n < k || n < 0 || k < 0) return 0;
    extend(n);
    if constexpr (std::is_integral_v<T>) {
      return fac[n] * finv[n - k] % p;
    } else {
      return fac[n] * finv[n - k];
    }
  }

  // 重複組合せ nHk = (n+k-1)Ck
  T H(int n, int k) {
    if (n < 0 || k < 0) return 0;
    if (n == 0 && k == 0) return 1;
    return C(n + k - 1, k);
  }

  // 糖衣構文 (従来との互換用)
  T operator()(int n, int k) {
    return C(n, k);
  }
  T operator[](int n) {
    return fact(n);
  }
};

// ==========================================
// 2. ModPowTable (累乗テーブル)
// ==========================================

/**
 * @brief 底 x の累乗（x^i）をテーブル化して高速取得
 * @tparam T 計算に用いる型 (long long または atcoder::modint / dynamic_modint)
 * @note T に atcoder::modint998244353 や atcoder::modint (dynamic_modint)
 * を指定しても正常に動作します。 i > 2e6 の巨大な指数が渡された場合は、自動的に
 * .pow(i) による計算に切り替わります。
 */
template <class T = long long>
struct ModPowTable {
  long long x, m;
  std::vector<T> d;

  ModPowTable(long long x_, long long m_ = 998244353) : x(x_), m(m_) {}

  T operator[](long long i) {
    if (i < 0) return T(0);

    // 巨大な指数は直接計算
    if (i > 2e6) {
      if constexpr (std::is_integral_v<T>) {
        return atcoder::pow_mod(x, i, m);
      } else {
        T base = x;
        return base.pow(i);  // 一時オブジェクトではなく明示的な変数から呼ぶ
      }
    }

    // 初回アクセス時に 0 項目 (x^0 = 1) を生成
    if (d.empty()) {
      d.push_back(T(1));
    }

    // テーブルの動的拡張
    while ((long long)d.size() <= i) {
      if constexpr (std::is_integral_v<T>) {
        d.push_back(d.back() * x % m);
      } else {
        d.push_back(d.back() * T(x));  // 明示的に T 型にキャストして乗算
      }
    }
    return d[i];
  }
};