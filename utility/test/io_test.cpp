#include <bits/stdc++.h>

// テスト対象の入出力ヘッダー
#include "utility/io.hpp"

// --- helper: ostream への出力を文字列として取得する関数 ---
template <typename T>
std::string to_string_via_io(const T& val) {
  std::ostringstream oss;
  oss << val;
  return oss.str();
}

void test_modint_io() {
  atcoder::modint998244353 a = 100;
  assert(to_string_via_io(a) == "100");

  std::istringstream iss("998244354");
  atcoder::modint998244353 b;
  iss >> b;
  assert(b.val() == 1);

  std::cout << "[PASS] modint I/O tests" << std::endl;
}

void test_int128_io() {
  __int128_t x = 1234567890123456789LL;
  assert(to_string_via_io(x) == "1234567890123456789");

  std::istringstream iss("-9876543210987654321");
  __int128_t y;
  iss >> y;
  assert(to_string_via_io(y) == "-9876543210987654321");

  std::cout << "[PASS] __int128_t I/O tests" << std::endl;
}

void test_pair_tuple_io() {
  std::pair<int, std::string> p = {1, "apple"};
  assert(to_string_via_io(p) == "1 apple");

  auto t = std::make_tuple(10, "test", 3.14);
  assert(to_string_via_io(t) == "10 test 3.14");

  std::cout << "[PASS] pair & tuple I/O tests" << std::endl;
}

void test_containers_io() {
  // vector
  std::vector<int> v = {1, 2, 3};
  assert(to_string_via_io(v) == "1 2 3");

  // set
  std::set<int> st = {3, 1, 2};
  assert(to_string_via_io(st) == "1 2 3 ");

  // map
  std::map<int, std::string> mp = {{1, "one"}, {2, "two"}};
  assert(to_string_via_io(mp) == "[{1->one}, {2->two}]");

  // queue / stack
  std::queue<int> q;
  q.push(10);
  q.push(20);
  assert(to_string_via_io(q) == "10 20 ");

  std::stack<int> sk;
  sk.push(10);
  sk.push(20);
  assert(to_string_via_io(sk) == "20 10 ");

  std::cout << "[PASS] containers I/O tests" << std::endl;
}

void test_out_utility() {
  // std::cout のバッファを一時横取りして out() 関数の動作をテスト
  std::ostringstream oss;
  std::streambuf* old_cout = std::cout.rdbuf(oss.rdbuf());

  out(1, "hello", 3.14);

  std::cout.rdbuf(old_cout);  // 元に戻す
  assert(oss.str() == "1 hello 3.14\n");

  std::cout << "[PASS] out() utility tests" << std::endl;
}

int main() {
  test_modint_io();
  test_int128_io();
  test_pair_tuple_io();
  test_containers_io();
  test_out_utility();

  std::cout << "\nAll I/O tests passed successfully!" << std::endl;
  return 0;
}