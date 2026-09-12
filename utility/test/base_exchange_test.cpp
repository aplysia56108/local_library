#include "utility/base_exchange.hpp"

#include <bits/stdc++.h>

void test_to_base10() {
  assert(to_base10("1010", 2) == 10);
  assert(to_base10("1F", 16) == 31);
  assert(to_base10("0", 10) == 0);
  assert(to_base10("777", 8) == 511);
  assert(to_base10("Z", 36) == 35);
  std::cout << "[PASS] to_base10 tests" << std::endl;
}

void test_bin() {
  assert(bin(0) == "0");
  assert(bin(0, 4) == "0000");
  assert(bin(10) == "1010");
  assert(bin(5, 8) == "00000101");
  assert(bin(15, 3) == "1111");  // パディング幅より大きい場合はそのまま展開
  std::cout << "[PASS] bin tests" << std::endl;
}

void test_to_baseB() {
  assert(to_baseB(10, 2) == "1010");
  assert(to_baseB(31, 16) == "1F");
  assert(to_baseB(0, 10) == "0");
  assert(to_baseB(-10, 2) == "-1010");
  assert(to_baseB(511LL, 8) == "777");
  assert(to_baseB(35, 36) == "Z");
  std::cout << "[PASS] to_baseB tests" << std::endl;
}

void test_roundtrip() {
  // 相互変換（10進数 -> B進数 -> 10進数）の一貫性テスト
  std::vector<long long> values = {0, 1, 42, 1000, 1000000LL};
  for (auto val : values) {
    for (int b = 2; b <= 36; ++b) {
      std::string b_str = to_baseB(val, b);
      long long restored = to_base10(b_str, b);
      assert(val == restored);
    }
  }
  std::cout << "[PASS] roundtrip (to_baseB <-> to_base10) tests" << std::endl;
}

int main() {
  test_to_base10();
  test_bin();
  test_to_baseB();
  test_roundtrip();

  std::cout << "\nAll math_utils tests passed successfully!" << std::endl;
  return 0;
}