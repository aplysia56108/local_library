#include <bits/stdc++.h>

#include <cassert>

// テスト対象の入出力ヘッダー
#include "utility/random.hpp"

void test_rng_and_utils() {
  // 1. 乱数範囲テスト
  for (int i = 0; i < 1000; i++) {
    long long val = rnd(10, 20);
    assert(10 <= val && val < 20);

    double dval = rnd[5.5];
    assert(0.0 <= dval && dval <= 5.5);
  }

  // 3. シャッフル＆サンプリングテスト
  std::vector<int> v = {1, 2, 3, 4, 5};
  auto sampled = sample(v, 3);
  assert(sampled.size() == 3);

  rnd_shuffle(v);
  assert(v.size() == 5);

  // 4. タイマー計測のテスト (少し待機して計測)
  double t1 = now_time();
  sleep(1.0);
  assert(now_time() - t1 > 1.0);
  assert(now_time() - t1 < 1.05);
  Timer timer;
  sleep(1.0);
  assert(timer.elapsed_sec() > 1.0);
  assert(timer.elapsed_ms() > 1000);
  assert(timer.elapsed_sec() < 1.05);
  assert(timer.elapsed_ms() < 1050);

  timer.reset();
  sleep(1.0);
  assert(timer.elapsed_sec() > 1.0);
  assert(timer.elapsed_ms() > 1000);
  assert(timer.elapsed_sec() < 1.05);
  assert(timer.elapsed_ms() < 1050);

  std::cout << "All RNG and Utility tests passed successfully!" << std::endl;
}

int main() {
  test_rng_and_utils();
  return 0;
}