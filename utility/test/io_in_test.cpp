#include <bits/stdc++.h>

#include "utility/io.hpp"

/**
 * @brief Reader を使って入力結果を検証する。
 * @tparam T 入力対象の型。
 * @param input 入力文字列。
 * @param expected 期待する値。
 */
template <class T>
void test_read(const std::string& input, const T& expected) {
  std::istringstream iss(input);
  io_impl::Reader reader(iss);

  T actual{};
  reader.read(actual);

  assert(!iss.fail());
  assert(actual == expected);
}

/** @brief 基本型・文字列の入力を検証する。 */
void test_basic_input() {
  test_read<long long>("123", 123);
  test_read<long long>("-456", -456);
  test_read<long long>("0", 0);
  test_read<double>("3.5", 3.5);
  test_read<char>("a", 'a');
  test_read<bool>("1", true);
  test_read<bool>("0", false);
  test_read<std::string>("hello", "hello");

  std::cout << "[PASS] basic input\n";
}

/** @brief pair・tuple の再帰入力を検証する。 */
void test_pair_tuple_input() {
  test_read<std::pair<long long, std::string>>("10 apple", {10, "apple"});

  test_read<std::tuple<long long, char, double>>("20 x 3.5", {20, 'x', 3.5});

  test_read<std::pair<long long, std::pair<long long, long long>>>("1 2 3",
                                                                   {1, {2, 3}});

  test_read<std::tuple<>>("", {});

  std::cout << "[PASS] pair & tuple input\n";
}

/** @brief vector・array の入力を検証する。 */
void test_container_input() {
  std::istringstream iss("1 2 3 4 5 6 7 8 9");
  io_impl::Reader reader(iss);

  std::vector<long long> v(3);
  std::array<long long, 3> a{};
  std::vector<long long> w(3);

  reader.read(v);
  reader.read(a);
  reader.read(w);

  assert((v == std::vector<long long>{1, 2, 3}));
  assert((a == std::array<long long, 3>{4, 5, 6}));
  assert((w == std::vector<long long>{7, 8, 9}));
  assert(!iss.fail());

  std::cout << "[PASS] container input\n";
}

/** @brief ネストしたコンテナの再帰入力を検証する。 */
void test_nested_input() {
  std::istringstream iss("1 2 3 4 5 6 7 8");
  io_impl::Reader reader(iss);

  std::vector<std::vector<long long>> grid(2, std::vector<long long>(3));
  std::array<std::pair<long long, long long>, 1> pairs{};

  reader.read(grid);
  reader.read(pairs);

  assert((grid == std::vector<std::vector<long long>>{{1, 2, 3}, {4, 5, 6}}));
  assert((pairs[0] == std::pair<long long, long long>{7, 8}));
  assert(!iss.fail());

  std::cout << "[PASS] nested input\n";
}

/** @brief 空コンテナの入力を検証する。 */
void test_empty_input() {
  std::istringstream iss("42");
  io_impl::Reader reader(iss);

  std::vector<long long> empty_vector;
  std::array<long long, 0> empty_array{};
  long long x = 0;

  reader.read(empty_vector);
  reader.read(empty_array);
  reader.read(x);

  assert(empty_vector.empty());
  assert(x == 42);
  assert(!iss.fail());

  std::cout << "[PASS] empty input\n";
}

/** @brief ACL modint の入力を検証する。 */
void test_modint_input() {
  using Mint = atcoder::modint998244353;
  test_read<Mint>("998244354", Mint(1));
  test_read<Mint>("-1", Mint(998244352));

  using DynamicMint = atcoder::dynamic_modint<0>;
  DynamicMint::set_mod(1000000007);

  test_read<DynamicMint>("1000000008", DynamicMint(1));
  test_read<DynamicMint>("-1", DynamicMint(1000000006));

  std::istringstream iss("1 2 3");
  io_impl::Reader reader(iss);
  std::vector<Mint> v(3);
  reader.read(v);

  assert((v == std::vector<Mint>{1, 2, 3}));

  std::cout << "[PASS] modint input\n";
}

/** @brief 128bit整数の境界値・符号を検証する。 */
void test_int128_input() {
  using i128 = __int128_t;
  using u128 = __uint128_t;

  constexpr i128 imin = std::numeric_limits<i128>::min();
  constexpr i128 imax = std::numeric_limits<i128>::max();
  constexpr u128 umax = std::numeric_limits<u128>::max();

  test_read<i128>("0", i128(0));
  test_read<i128>("+123", i128(123));
  test_read<i128>("-123", i128(-123));
  test_read<i128>("-0", i128(0));

  test_read<i128>("-170141183460469231731687303715884105728", imin);
  test_read<i128>("170141183460469231731687303715884105727", imax);

  test_read<u128>("0", u128(0));
  test_read<u128>("+123", u128(123));
  test_read<u128>("340282366920938463463374607431768211455", umax);

  std::cout << "[PASS] int128 input\n";
}

/** @brief 不正な128bit整数入力でfailbitが立つことを検証する。 */
void test_int128_invalid_input() {
  using i128 = __int128_t;
  using u128 = __uint128_t;

  auto check_fail = []<class T>(const std::string& input) {
    std::istringstream iss(input);
    io_impl::Reader reader(iss);

    T value = 123;
    reader.read(value);

    assert(iss.fail());
    assert(value == T(123));
  };

  check_fail.operator()<i128>("170141183460469231731687303715884105728");
  check_fail.operator()<i128>("-170141183460469231731687303715884105729");
  check_fail.operator()<u128>("340282366920938463463374607431768211456");
  check_fail.operator()<u128>("-1");
  check_fail.operator()<i128>("12x34");
  check_fail.operator()<i128>("+");
  check_fail.operator()<i128>("-");

  std::cout << "[PASS] invalid int128 input\n";
}

/** @brief operator>> の入力を検証する。 */
void test_stream_operator_input() {
  std::istringstream iss("10 20 30 40 50 60");

  std::pair<long long, long long> p;
  std::vector<long long> v(2);
  std::array<long long, 2> a{};

  iss >> p >> v >> a;

  assert((p == std::pair<long long, long long>{10, 20}));
  assert((v == std::vector<long long>{30, 40}));
  assert((a == std::array<long long, 2>{50, 60}));
  assert(!iss.fail());

  std::cout << "[PASS] operator>> input\n";
}

int main() {
  test_basic_input();
  test_pair_tuple_input();
  test_container_input();
  test_nested_input();
  test_empty_input();
  test_modint_input();
  test_int128_input();
  test_int128_invalid_input();
  test_stream_operator_input();

  std::cout << "\nAll input tests passed successfully!\n";
}
