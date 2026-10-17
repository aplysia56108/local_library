
#include <bits/stdc++.h>

#include "utility/io.hpp"

template <class T>
std::string to_string_via_io(const T& val) {
  std::ostringstream oss;
  oss << val;
  return oss.str();
}

/** @brief 基本型の出力を検証する */
void test_basic_io() {
  assert(to_string_via_io(123LL) == "123");
  assert(to_string_via_io(-123LL) == "-123");
  assert(to_string_via_io(0LL) == "0");
  assert(to_string_via_io(3.14) == "3.14");
  assert(to_string_via_io('a') == "a");
  assert(to_string_via_io(true) == "1");

  assert(to_string_via_io(std::string("abc")) == "abc");
  assert(to_string_via_io(std::string_view("abc")) == "abc");
  assert(to_string_via_io("abc") == "abc");
  assert(to_string_via_io(std::string("")) == "");

  std::cout << "[PASS] basic I/O\n";
}

/** @brief ACL modint の出力を検証する */
void test_modint_io() {
  atcoder::modint998244353 a = 100;
  assert(to_string_via_io(a) == "100");

  a = 998244354LL;
  assert(to_string_via_io(a) == "1");

  using Mint = atcoder::dynamic_modint<0>;
  Mint::set_mod(1000000007);

  Mint b = 123;
  assert(to_string_via_io(b) == "123");

  std::vector<Mint> v = {1, 2, 3};
  assert(to_string_via_io(v) == "1 2 3");

  std::cout << "[PASS] modint I/O\n";
}

/** @brief 128bit 整数の出力を検証する */
void test_int128_io() {
  __int128_t x = 1234567890123456789LL;
  assert(to_string_via_io(x) == "1234567890123456789");

  x *= 100;
  assert(to_string_via_io(x) == "123456789012345678900");

  x = -x;
  assert(to_string_via_io(x) == "-123456789012345678900");

  std::cout << "[PASS] int128 I/O\n";
}

/** @brief pair・tuple の再帰出力を検証する */
void test_pair_tuple_io() {
  std::pair<long long, std::string> p = {1, "apple"};
  assert(to_string_via_io(p) == "1 apple");

  auto t = std::make_tuple(10LL, "test", 3.14);
  assert(to_string_via_io(t) == "10 test 3.14");

  auto empty = std::make_tuple();
  assert(to_string_via_io(empty) == "");

  std::pair<long long, std::pair<long long, long long>> nested = {1, {2, 3}};
  assert(to_string_via_io(nested) == "1 2 3");

  std::cout << "[PASS] pair & tuple I/O\n";
}

/** @brief 標準コンテナの出力を検証する */
void test_containers_io() {
  std::vector<long long> v = {1, 2, 3};
  assert(to_string_via_io(v) == "1 2 3");

  std::array<long long, 3> a = {1, 2, 3};
  assert(to_string_via_io(a) == "1 2 3");

  std::deque<long long> dq = {1, 2, 3};
  assert(to_string_via_io(dq) == "1 2 3");

  std::set<long long> st = {3, 1, 2};
  assert(to_string_via_io(st) == "1 2 3");

  std::multiset<long long> mst = {2, 1, 2};
  assert(to_string_via_io(mst) == "1 2 2");

  std::map<long long, std::string> mp = {{1, "one"}, {2, "two"}};
  assert(to_string_via_io(mp) == "1 one\n2 two");

  std::cout << "[PASS] containers I/O\n";
}

/** @brief コンテナアダプターの出力を検証する */
void test_adapter_io() {
  std::queue<long long> q;
  q.push(10);
  q.push(20);
  assert(to_string_via_io(q) == "10 20");
  assert(q.size() == 2);

  std::stack<long long> sk;
  sk.push(10);
  sk.push(20);
  assert(to_string_via_io(sk) == "20 10");
  assert(sk.size() == 2);

  std::priority_queue<long long> pq;
  pq.push(3);
  pq.push(1);
  pq.push(2);
  assert(to_string_via_io(pq) == "3 2 1");
  assert(pq.size() == 3);

  std::priority_queue<long long, std::vector<long long>,
                      std::greater<long long>>
      min_pq;
  min_pq.push(3);
  min_pq.push(1);
  min_pq.push(2);
  assert(to_string_via_io(min_pq) == "1 2 3");

  std::cout << "[PASS] adapter I/O\n";
}

/** @brief Range の汎用出力を検証する */
void test_range_io() {
  std::vector<long long> v = {1, 2, 3, 4};

  std::span<const long long> sp(v);
  assert(to_string_via_io(sp) == "1 2 3 4");

  std::span<const long long> empty;
  assert(to_string_via_io(empty) == "");

  auto sub = std::ranges::subrange(v.begin() + 1, v.end());
  assert(to_string_via_io(sub) == "2 3 4");

  auto view = v | std::views::take(2);
  // const 参照から走査できる View を検証
  if constexpr (type_check::IsPrintableRange<decltype(view)>) {
    assert(to_string_via_io(view) == "1 2");
  }

  std::cout << "[PASS] range I/O\n";
}

/** @brief ネストしたコンテナの出力を検証する */
void test_nested_io() {
  std::vector<std::vector<long long>> vv = {{1, 2}, {3, 4}};
  assert(to_string_via_io(vv) == "1 2\n3 4");

  std::vector<std::vector<long long>> irregular = {{1, 2, 3}, {}, {4}};
  assert(to_string_via_io(irregular) == "1 2 3\n\n4");

  std::vector<std::pair<long long, long long>> vp = {{1, 2}, {3, 4}};
  assert(to_string_via_io(vp) == "1 2 3 4");

  std::vector<std::string> vs = {"abc", "def"};
  assert(to_string_via_io(vs) == "abc def");

  std::vector<std::vector<std::string>> vvs = {{"abc", "def"}, {"ghi", "jkl"}};
  assert(to_string_via_io(vvs) == "abc def\nghi jkl");

  std::cout << "[PASS] nested I/O\n";
}

/** @brief optional・variant の出力を検証する */
void test_optional_variant_io() {
  std::optional<long long> opt = 123;
  assert(to_string_via_io(opt) == "123");

  std::optional<std::string> str = "hello";
  assert(to_string_via_io(str) == "hello");

  std::variant<long long, std::string> var = "hello";
  assert(to_string_via_io(var) == "hello");

  var = 456LL;
  assert(to_string_via_io(var) == "456");

  std::vector<std::optional<long long>> v = {1, 2, 3};
  assert(to_string_via_io(v) == "1 2 3");

  std::cout << "[PASS] optional & variant I/O\n";
}

/** @brief io_via_val による出力を検証する */
void test_io_via_val() {
  struct Value {
    using io_via_val = void;
    long long x;

    long long val() const {
      return x;
    }
  };

  Value x{123};
  assert(to_string_via_io(x) == "123");

  std::vector<Value> v = {{1}, {2}, {3}};
  assert(to_string_via_io(v) == "1 2 3");

  struct CharValue {
    using io_via_val = void;
    char c;

    char val() const {
      return c;
    }
  };

  std::vector<CharValue> chars = {{'a'}, {'b'}, {'c'}};
  assert(to_string_via_io(chars) == "a b c");

  struct StringValue {
    using io_via_val = void;
    long long x, y;

    std::string val() const {
      return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
    }
  };

  assert(to_string_via_io(StringValue{1, 2}) == "(1,2)");

  std::cout << "[PASS] io_via_val I/O\n";
}

/** @brief 自作Rangeの出力を検証する */
void test_custom_range_io() {
  struct CustomRange {
    std::vector<long long> values;

    auto begin() const {
      return values.begin();
    }
    auto end() const {
      return values.end();
    }
  };

  CustomRange r{{10, 20, 30}};
  assert(to_string_via_io(r) == "10 20 30");

  std::cout << "[PASS] custom range I/O\n";
}

/** @brief コピー不可Rangeの出力を検証する */
void test_noncopyable_range_io() {
  struct NonCopyableRange {
    std::vector<long long> values;

    NonCopyableRange(std::initializer_list<long long> v) : values(v) {}

    NonCopyableRange(const NonCopyableRange&) = delete;
    NonCopyableRange& operator=(const NonCopyableRange&) = delete;

    auto begin() const {
      return values.begin();
    }
    auto end() const {
      return values.end();
    }
  };

  NonCopyableRange r{10, 20, 30};
  assert(to_string_via_io(r) == "10 20 30");

  std::cout << "[PASS] noncopyable range I/O\n";
}

/** @brief 空コンテナの出力を検証する */
void test_empty_containers() {
  assert(to_string_via_io(std::vector<long long>{}) == "");
  assert(to_string_via_io(std::set<long long>{}) == "");
  assert(to_string_via_io(std::map<long long, long long>{}) == "");
  assert(to_string_via_io(std::queue<long long>{}) == "");
  assert(to_string_via_io(std::stack<long long>{}) == "");
  assert(to_string_via_io(std::priority_queue<long long>{}) == "");

  std::vector<std::vector<long long>> vv = {{}, {}};
  assert(to_string_via_io(vv) == "\n");

  std::cout << "[PASS] empty containers\n";
}

/** @brief out・out_without_endl の出力を検証する */
void test_out_utility() {
  std::ostringstream oss;
  std::streambuf* old_cout = std::cout.rdbuf(oss.rdbuf());

  out(1, "hello", 3.14);
  out();
  out(std::vector<long long>{1, 2, 3});
  out_without_endl("abc");
  out_without_endl(1, 2, 3);

  std::cout.rdbuf(old_cout);

  assert(oss.str() ==
         "1 hello 3.14\n"
         "\n"
         "1 2 3\n"
         "abc1 2 3");

  std::cout << "[PASS] out utility\n";
}

int main() {
  test_basic_io();
  test_modint_io();
  test_int128_io();
  test_pair_tuple_io();
  test_containers_io();
  test_adapter_io();
  test_range_io();
  test_nested_io();
  test_optional_variant_io();
  test_io_via_val();
  test_custom_range_io();
  test_noncopyable_range_io();
  test_empty_containers();
  test_out_utility();

  std::cout << "\nAll I/O tests passed successfully!\n";
}
