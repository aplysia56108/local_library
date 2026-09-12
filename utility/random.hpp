#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

// ==========================================
// 1. 乱数生成器 (Random Number Generator)
// ==========================================

struct RandomNumberGenerator {
  std::mt19937_64 engine;  // 64bit 高速乱数エンジン

  RandomNumberGenerator(long long seed = -1) {
    if (seed == -1) {
      engine = std::mt19937_64(
          std::chrono::steady_clock::now().time_since_epoch().count());
    } else {
      engine = std::mt19937_64(seed);
    }
  }

  // 整数乱数 [a, b)
  long long operator()(long long a, long long b) {
    if (a >= b) return a;
    std::uniform_int_distribution<long long> dist(a, b - 1);
    return dist(engine);
  }

  // 整数乱数 [0, b)
  long long operator()(long long b) {
    return (*this)(0, b);
  }

  // 64bit 非負整数乱数 [0, 2^60)
  long long operator()() {
    return (*this)(0, 1LL << 60);
  }

  // 実数乱数 [0.0, a)
  double operator[](double a) {
    std::uniform_real_distribution<double> dist(0.0, a);
    return dist(engine);
  }

  // 正規分布 (平均 mean, 標準偏差 sigma)
  double normal_dist(double sigma, double mean = 0.0) {
    std::normal_distribution<double> dist(mean, sigma);
    return dist(engine);
  }
};
inline RandomNumberGenerator rnd;

struct Timer {
  std::chrono::steady_clock::time_point start_time;

  Timer() : start_time(std::chrono::steady_clock::now()) {}

  // リセット
  void reset() {
    start_time = std::chrono::steady_clock::now();
  }

  // 経過時間（秒）を取得
  double elapsed_sec() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - start_time).count();
  }

  // 経過時間（ミリ秒）を取得
  double elapsed_ms() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(now - start_time).count();
  }
};
inline Timer default_timer;

inline double now_time() {
  return default_timer.elapsed_sec();
}

// ==========================================
// 3. シャッフル・サンプリング
// ==========================================

/**
 * @brief 配列をランダムにシャッフル
 */
template <class T>
void rnd_shuffle(std::vector<T>& v) {
  std::shuffle(v.begin(), v.end(), rnd.engine);
}

/**
 * @brief コンテナからランダムに n 個の要素を重複なしで抽出
 */
template <class Container>
Container sample(const Container& v, size_t n) {
  Container res;
  std::sample(v.begin(), v.end(), std::back_inserter(res), n, rnd.engine);
  return res;
}